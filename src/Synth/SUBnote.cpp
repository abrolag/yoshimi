/*

    SUBnote.cpp - The "subtractive" synthesizer

    Original ZynAddSubFX author Nasca Octavian Paul
    Copyright (C) 2002-2009 Nasca Octavian Paul
    Copyright 2009-2011, Alan Calvert
    Copyright 2014-2019, Will Godfrey & others

    This file is part of yoshimi, which is free software: you can redistribute
    it and/or modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    yoshimi is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.   See the GNU General Public License (version 2 or
    later) for more details.

    You should have received a copy of the GNU General Public License along with
    yoshimi; if not, write to the Free Software Foundation, Inc., 51 Franklin
    Street, Fifth Floor, Boston, MA  02110-1301, USA.

    This file is derivative of ZynAddSubFX original code
    Modified January 2019
*/

#include <cmath>
#include <fftw3.h>
#include <iostream>

#include "Params/SUBnoteParameters.h"
#include "Params/Controller.h"
#include "Synth/Envelope.h"
#include "DSP/Filter.h"
#include "Misc/SynthEngine.h"
#include "Misc/SynthHelper.h"
#include "Synth/SUBnote.h"

using synth::velF;
using synth::getDetune;
using synth::interpolateAmplitude;
using synth::aboveAmplitudeThreshold;


SUBnote::SUBnote(SUBnoteParameters *parameters, Controller *ctl_, float freq,
                 float velocity, int portamento_, int midinote, bool besilent, SynthEngine *_synth) :
    pars(parameters),
    GlobalFilterL(NULL),
    GlobalFilterR(NULL),
    GlobalFilterEnvelope(NULL),
    portamento(portamento_),
    ctl(ctl_),
    log_0_01(logf(0.01f)),
    log_0_001(logf(0.001f)),
    log_0_0001(logf(0.0001f)),
    log_0_00001(logf(0.00001f)),
    subNoteChange(parameters),
    synth(_synth),
    filterStep(0)
{
    ready = 0;

    // Initialise some legato-specific vars
    Legato.msg = LM_Norm;
    Legato.fade.length = int(synth->samplerate_f * 0.005f); // 0.005 seems ok.
    if (Legato.fade.length < 1)
        Legato.fade.length = 1;// (if something's fishy)
    Legato.fade.step = (1.0f / Legato.fade.length);
    Legato.decounter = -10;
    Legato.param.freq = freq;
    Legato.param.vel = velocity;
    Legato.param.portamento = portamento_;
    Legato.param.midinote = midinote;
    Legato.silent = besilent;

    NoteEnabled = true;

    numstages = pars->Pnumstages;
    stereo = pars->Pstereo;
    start = pars->Pstart;
    firsttick = 1;

    float t = synth->numRandom();
    randpanL = cosf(t * HALFPI);
    randpanR = cosf((1.0f - t) * HALFPI);

    numharmonics = 0;
    lfilter = NULL;
    rfilter = NULL;

    setBaseFreq();

    oldpitchwheel = 0;
    oldbandwidth = 64;

    if (pars->Pfixedfreq == 0)
        initparameters(basefreq);
    else
        initparameters(basefreq / 440.0f * freq);

    computeNoteParameters();
    computecurrentparameters();

    oldamplitude = newamplitude;
    ready = 1;
}


// SUBlegatonote: This function is (mostly) a copy of SUBnote(...) and
// initparameters(...) stuck together with some lines removed so that
// it only alter the already playing note (to perform legato). It is
// possible I left stuff that is not required for this.
void SUBnote::SUBlegatonote(float freq, float velocity,
                            int portamento_, int midinote, bool externcall)
{
    // Manage legato stuff
    if (externcall)
        Legato.msg = LM_Norm;
    if (Legato.msg != LM_CatchUp)
    {
        Legato.lastfreq = Legato.param.freq;
        Legato.param.freq = freq;
        Legato.param.vel = velocity;
        Legato.param.portamento = portamento_;
        Legato.param.midinote = midinote;
        if (Legato.msg == LM_Norm)
        {
            if (Legato.silent)
            {
                Legato.fade.m = 0.0f;
                Legato.msg = LM_FadeIn;
            }
            else
            {
                Legato.fade.m = 1.0;
                Legato.msg = LM_FadeOut;
                return;
            }
        }
        if (Legato.msg == LM_ToNorm)
            Legato.msg = LM_Norm;
    }

    portamento = portamento_;

    setBaseFreq();

    oldpitchwheel = 0;
    oldbandwidth = 64;

    computeNoteParameters();

    oldamplitude = newamplitude;

    // End of the SUBlegatonote function.
}


SUBnote::~SUBnote()
{
    if (NoteEnabled)
        KillNote();
}


// Kill the note
void SUBnote::KillNote(void)
{
    if (NoteEnabled)
    {
        delete [] lfilter;
        lfilter = NULL;
        if (stereo)
            delete [] rfilter;
        rfilter = NULL;
        delete AmpEnvelope;
        if (FreqEnvelope != NULL)
            delete FreqEnvelope;
        if (BandWidthEnvelope != NULL)
            delete BandWidthEnvelope;
        NoteEnabled = false;
    }
}

int SUBnote::createNewFilters()
{
    bool alreadyEnabled[MAX_SUB_HARMONICS];
    memset(alreadyEnabled, 0, sizeof(alreadyEnabled));
    for (int p = 0; p < numharmonics; ++p)
        alreadyEnabled[pos[p]] = true;

    // select only harmonics that desire to compute
    int origNumHarmonics = numharmonics;
    for (int n = 0; n < MAX_SUB_HARMONICS; ++n)
    {
        if (pars->Phmag[n] == 0 || alreadyEnabled[n])
            continue;
        if (n * basefreq > synth->halfsamplerate_f)
            break; // remove the freqs above the Nyquist freq
        pos[numharmonics++] = n;
        alreadyEnabled[n] = true;
    }

    if (numharmonics == origNumHarmonics)
        return 0;

    bpfilter *newFilter = new bpfilter[numstages * numharmonics];
    if (lfilter != NULL)
    {
        memcpy(newFilter, lfilter, numstages * origNumHarmonics * sizeof(bpfilter));
        delete [] lfilter;
    }
    lfilter = newFilter;
    if (stereo != 0)
    {
        newFilter = new bpfilter[numstages * numharmonics];
        if (rfilter != NULL)
        {
            memcpy(newFilter, rfilter, numstages * origNumHarmonics * sizeof(bpfilter));
            delete [] rfilter;
        }
        rfilter = newFilter;
    }

    return numharmonics - origNumHarmonics;
}

void SUBnote::setBaseFreq()
{
    if (pars->Pfixedfreq == 0)
        basefreq = Legato.param.freq;
    else
    {
        basefreq = 440.0f;
        int fixedfreqET = pars->PfixedfreqET;
        if (fixedfreqET)
        {   // if the frequency varies according the keyboard note
            float tmp =
                (Legato.param.midinote - 69.0f) / 12.0f * powf(2.0f, (((fixedfreqET - 1) / 63.0f) - 1.0f));
            if (fixedfreqET <= 64)
                basefreq *= powf(2.0f, tmp);
            else
                basefreq *= powf(3.0f, tmp);
        }
    }

    float detune = getDetune(pars->PDetuneType, pars->PCoarseDetune, pars->PDetune);
    basefreq *= powf(2.0f, detune / 1200.0f); // detune
//    basefreq*=ctl->pitchwheel.relfreq;//pitch wheel
}

void SUBnote::computeNoteParameters()
{
    volume = powf(0.1f, 3.0f * (1.0f - pars->PVolume / 96.0f)); // -60 dB .. 0 dB
    volume *= velF(Legato.param.vel, pars->PAmpVelocityScaleFunction);

    int BendAdj = pars->PBendAdjust - 64;
    if (BendAdj % 24 == 0)
        BendAdjust = BendAdj / 24;
    else
        BendAdjust = BendAdj / 24.0f;
    float offset_val = (pars->POffsetHz - 64)/64.0f;
    OffsetHz = 15.0f*(offset_val * sqrtf(fabsf(offset_val)));

    // global filter
    GlobalFilterCenterPitch =
        pars->GlobalFilter->getfreq()
        + // center freq
          (pars->PGlobalFilterVelocityScale / 127.0f * 6.0f)
        * // velocity sensing
          (velF(Legato.param.vel, pars->PGlobalFilterVelocityScaleFunction) - 1);

    updatefilterbank();

    if (pars->PGlobalFilterEnabled != 0)
    {
        globalfiltercenterq = pars->GlobalFilter->getq();
        GlobalFilterFreqTracking = pars->GlobalFilter->getfreqtracking(basefreq);
    }
}

// Compute the filters coefficients
void SUBnote::computefiltercoefs(bpfilter &filter, float freq, float bw, float gain)
{
    if (freq > synth->halfsamplerate_f - 200.0f)
    {
        freq = synth->halfsamplerate_f - 200.0f;
    }

    float omega = TWOPI * freq / synth->samplerate_f;
    float sn = sinf(omega);
    float cs = cosf(omega);
    float alpha = sn * sinhf(LOG_2 / 2.0f * bw * omega / sn);

    if (alpha > 1)
        alpha = 1;
    if (alpha > bw)
        alpha = bw;

    filter.b0 = alpha / (1.0f + alpha) * filter.amp * gain;
    filter.b2 = -alpha / (1.0f + alpha) * filter.amp * gain;
    filter.a1 = -2.0f * cs / (1.0f + alpha);
    filter.a2 = (1.0f - alpha) / (1.0f + alpha);
}


// Initialise the filters
void SUBnote::initfilters(int startIndex)
{
    for (int n = startIndex; n < numharmonics; ++n)
    {
        float hgain = getHgain(n);

        for (int nph = 0; nph < numstages; ++nph)
        {
            initfilter(lfilter[nph + n * numstages], hgain);
            if (stereo)
                initfilter(rfilter[nph + n * numstages], hgain);
        }
    }
}

void SUBnote::initfilter(bpfilter &filter, float mag)
{
    filter.xn1 = 0.0f;
    filter.xn2 = 0.0f;

    if (start == 0)
    {
        filter.yn1 = 0.0f;
        filter.yn2 = 0.0f;
    }
    else
    {
        float a = 0.1f * mag; // empirically
        float p = synth->numRandom() * TWOPI;
        if (start == 1)
            a *= synth->numRandom();
        filter.yn1 = a * cosf(p);
        filter.yn2 = a * cosf(p + filter.freq * TWOPI / synth->samplerate_f);

        // correct the error of computation the start amplitude
        // at very high frequencies
        if (filter.freq > synth->samplerate_f * 0.96f)
        {
            filter.yn1 = 0.0f;
            filter.yn2 = 0.0f;
        }
    }
}


// Do the filtering
inline void SubFilterA(const float coeff[4], float &src, float work[4])
{
    work[3] = src*coeff[0]+work[1]*coeff[1]+work[2]*coeff[2]+work[3]*coeff[3];
    work[1] = src;
    src     = work[3];
}


inline void SubFilterB(const float coeff[4], float &src, float work[4])
{
    work[2] = src*coeff[0]+work[0]*coeff[1]+work[3]*coeff[2]+work[2]*coeff[3];
    work[0] = src;
    src     = work[2];
}


// ported from zynaddsubfx V 2.4.4
//This dance is designed to minimize unneeded memory operations which can result
//in quite a bit of wasted time
void SUBnote::filter(bpfilter &filter, float *smps)
{
    if(synth->getIsLV2Plugin()){
        filterVarRun(filter, smps);
        return;
    }

    int remainder = synth->sent_buffersize % 8;
    int blocksize = synth->sent_buffersize - remainder;
    float coeff[4] = {filter.b0, filter.b2,  -filter.a1, -filter.a2};
    float work[4]  = {filter.xn1, filter.xn2, filter.yn1, filter.yn2};

    for(int i = 0; i < blocksize; i += 8)
    {
        SubFilterA(coeff, smps[i + 0], work);
        SubFilterB(coeff, smps[i + 1], work);
        SubFilterA(coeff, smps[i + 2], work);
        SubFilterB(coeff, smps[i + 3], work);
        SubFilterA(coeff, smps[i + 4], work);
        SubFilterB(coeff, smps[i + 5], work);
        SubFilterA(coeff, smps[i + 6], work);
        SubFilterB(coeff, smps[i + 7], work);
    }
    if (remainder > 0)
    {
        for(int i = blocksize; i < blocksize + remainder ; i += 2)
        {
            SubFilterA(coeff, smps[i + 0], work);
            SubFilterB(coeff, smps[i + 1], work);
        }
    }
    filter.xn1 = work[0];
    filter.xn2 = work[1];
    filter.yn1 = work[2];
    filter.yn2 = work[3];
}


//Andrew Deryabin: support for variable-length runs
//currently only for lv2 plugin
void SUBnote::filterVarRun(SUBnote::bpfilter &filter, float *smps)
{
    float tmpout;
    int runLength = synth->sent_buffersize;
    int i = 0;
    if(runLength >= 8){
        float coeff[4] = {filter.b0, filter.b2,  -filter.a1, -filter.a2};
        float work[4]  = {filter.xn1, filter.xn2, filter.yn1, filter.yn2};
        while(runLength >= 8){
            SubFilterA(coeff, smps[i + 0], work);
            SubFilterB(coeff, smps[i + 1], work);
            SubFilterA(coeff, smps[i + 2], work);
            SubFilterB(coeff, smps[i + 3], work);
            SubFilterA(coeff, smps[i + 4], work);
            SubFilterB(coeff, smps[i + 5], work);
            SubFilterA(coeff, smps[i + 6], work);
            SubFilterB(coeff, smps[i + 7], work);
            i += 8;
            runLength -= 8;
        }
        filter.xn1 = work[0];
        filter.xn2 = work[1];
        filter.yn1 = work[2];
        filter.yn2 = work[3];
    }

    for(; i < synth->sent_buffersize; ++i){
        tmpout=smps[i] * filter.b0 + filter.b2 * filter.xn2
               -filter.a1 * filter.yn1 - filter.a2 * filter.yn2;
        filter.xn2=filter.xn1;
        filter.xn1=smps[i];
        filter.yn2=filter.yn1;
        filter.yn1=tmpout;
        smps[i]=tmpout;
    }

}


// Init Parameters
void SUBnote::initparameters(float freq)
{
    AmpEnvelope = new Envelope(pars->AmpEnvelope, freq, synth);
    if (pars->PFreqEnvelopeEnabled != 0)
        FreqEnvelope = new Envelope(pars->FreqEnvelope, freq, synth);
    else
        FreqEnvelope = NULL;
    if (pars->PBandWidthEnvelopeEnabled != 0)
        BandWidthEnvelope = new Envelope(pars->BandWidthEnvelope, freq, synth);
    else
        BandWidthEnvelope = NULL;
    if (pars->PGlobalFilterEnabled != 0)
    {
        GlobalFilterL = new Filter(pars->GlobalFilter, synth);
        if (stereo != 0)
            GlobalFilterR = new Filter(pars->GlobalFilter, synth);
        GlobalFilterEnvelope = new Envelope(pars->GlobalFilterEnvelope, freq, synth);
    }
}
//end of port


// Compute how much to reduce amplitude near nyquist or subaudible frequencies.
float SUBnote::computerolloff(float freq)
{
    const float lower_limit = 10.0f;
    const float lower_width = 10.0f;
    const float upper_width = 200.0f;
    float upper_limit = synth->samplerate / 2.0f;

    if (freq > lower_limit + lower_width &&
            freq < upper_limit - upper_width)
        return 1.0f;
    if (freq <= lower_limit || freq >= upper_limit)
        return 0.0f;
    if (freq <= lower_limit + lower_width)
        return (1.0f - cosf(M_PI * (freq - lower_limit) / lower_width)) / 2.0f;
    return (1.0f - cosf(M_PI * (freq - upper_limit) / upper_width)) / 2.0f;
}

void SUBnote::computeallfiltercoefs()
{
    float envfreq = 1.0f;
    float envbw = 1.0f;
    float gain = 1.0f;

    if (FreqEnvelope != NULL)
    {
        envfreq = FreqEnvelope->envout() / 1200;
        envfreq = powf(2.0f, envfreq);
    }

    envfreq *= powf(ctl->pitchwheel.relfreq, BendAdjust); // pitch wheel

    if (portamento != 0)
    {   // portamento is used
        envfreq *= ctl->portamento.freqrap;
        if (ctl->portamento.used == 0)
        {   // the portamento has finished
            portamento = 0; // this note is no longer "portamented"
        }
    }

    if (BandWidthEnvelope != NULL)
    {
        envbw = BandWidthEnvelope->envout();
        envbw = powf(2.0f, envbw);
    }
    envbw *= ctl->bandwidth.relbw; // bandwidth controller

    float tmpgain = 1.0f / sqrtf(envbw * envfreq);

    for (int n = 0; n < numharmonics; ++n)
    {
        for (int nph = 0; nph < numstages; ++nph)
        {
            if (nph == 0)
                gain = tmpgain;
            else
                gain = 1.0f;
            computefiltercoefs( lfilter[nph + n * numstages],
                                lfilter[nph + n *numstages].freq * envfreq,
                                lfilter[nph + n * numstages].bw * envbw, gain);
        }
    }
    if (stereo)
        for (int n = 0; n < numharmonics; ++n)
        {
            for (int nph = 0; nph < numstages; ++nph)
            {
                if (nph == 0)
                    gain = tmpgain;
                else
                    gain = 1.0f;
                computefiltercoefs( rfilter[nph + n * numstages],
                                    rfilter[nph + n * numstages].freq * envfreq,
                                    rfilter[nph + n * numstages].bw * envbw, gain);
            }
        }
    oldbandwidth = ctl->bandwidth.data;
    oldpitchwheel = ctl->pitchwheel.data;
}

// Compute Parameters of SUBnote for each tick
void SUBnote::computecurrentparameters(void)
{
    // disabled till we know what we are doing!
    /*for (int n = 0; n < MAX_SUB_HARMONICS; ++n)
    {
        int changed = pars->PfilterChanged[n];
        if (changed)
        {
            if (changed == 6) // magnitude
                ;
            else if (changed == 7) // bandwidth
                ;
            cout << "Filter changed " << changed << endl;
            pars->PfilterChanged[n] = 0;
        }
    }*/
    if (FreqEnvelope != NULL
        || BandWidthEnvelope != NULL
        || oldpitchwheel != ctl->pitchwheel.data
        || oldbandwidth != ctl->bandwidth.data
        || portamento != 0)
        computeallfiltercoefs();
    newamplitude = volume * AmpEnvelope->envout_dB() * 2.0f;

    // Filter
    if (GlobalFilterL != NULL)
    {
        float globalfilterpitch = GlobalFilterCenterPitch + GlobalFilterEnvelope->envout();
        float filterfreq = globalfilterpitch + ctl->filtercutoff.relfreq + GlobalFilterFreqTracking;
        filterfreq = GlobalFilterL->getrealfreq(filterfreq);

        GlobalFilterL->setfreq_and_q(filterfreq, globalfiltercenterq * ctl->filterq.relq);
        if (GlobalFilterR != NULL)
            GlobalFilterR->setfreq_and_q(filterfreq, globalfiltercenterq * ctl->filterq.relq);
    }
}


// Note Output
int SUBnote::noteout(float *outl, float *outr)
{
    tmpsmp = synth->getRuntime().genTmp1;
    tmprnd = synth->getRuntime().genTmp2;
    memset(outl, 0, synth->sent_bufferbytes);
    memset(outr, 0, synth->sent_bufferbytes);
    if (!NoteEnabled)
        return 0;

    if (subNoteChange.checkUpdated())
    {
        setBaseFreq();
        computeNoteParameters();
    }

    // left channel
    for (int i = 0; i < synth->sent_buffersize; ++i)
        tmprnd[i] = synth->numRandom() * 2.0f - 1.0f;
    for (int n = 0; n < numharmonics; ++n)
    {
        float rolloff = overtone_rolloff[n];
        memcpy(tmpsmp, tmprnd, synth->sent_bufferbytes);
        for (int nph = 0; nph < numstages; ++nph)
            filter(lfilter[nph + n * numstages], tmpsmp);
        for (int i = 0; i < synth->sent_buffersize; ++i)
            outl[i] += tmpsmp[i] * rolloff;
    }

    if (GlobalFilterL != NULL)
        GlobalFilterL->filterout(outl);

    // right channel
    if (stereo)
    {
        for (int i = 0; i < synth->sent_buffersize; ++i)
            tmprnd[i] = synth->numRandom() * 2.0f - 1.0f;
        for (int n = 0; n < numharmonics; ++n)
        {
            float rolloff = overtone_rolloff[n];
            memcpy(tmpsmp, tmprnd, synth->sent_bufferbytes);
            for (int nph = 0; nph < numstages; ++nph)
                filter(rfilter[nph + n * numstages], tmpsmp);
            for (int i = 0; i < synth->sent_buffersize; ++i)
                outr[i] += tmpsmp[i] * rolloff;
        }
        if (GlobalFilterR != NULL)
            GlobalFilterR->filterout(outr);
    }
    else
        memcpy(outr, outl, synth->sent_bufferbytes);

    if (firsttick)
    {
        int n = 10;
        if (n > synth->sent_buffersize)
            n = synth->sent_buffersize;
        for (int i = 0; i < n; ++i)
        {
            float ampfadein = 0.5f - 0.5f * cosf((float)i / (float)n * PI);
            outl[i] *= ampfadein;
            outr[i] *= ampfadein;
        }
        firsttick = 0;
    }


    float pangainL = pars->pangainL; // assume non random pan
    float pangainR = pars->pangainR;
    if (pars->randomPan())
    {
        pangainL = randpanL;
        pangainR = randpanR;
    }

    if (aboveAmplitudeThreshold(oldamplitude, newamplitude))
    {
        // Amplitude interpolation
        for (int i = 0; i < synth->sent_buffersize; ++i)
        {
            float tmpvol = interpolateAmplitude(oldamplitude, newamplitude, i,
                                                synth->sent_buffersize);
            outl[i] *= tmpvol * pangainL;
            outr[i] *= tmpvol * pangainR;
        }
    }
    else
    {
        for (int i = 0; i < synth->sent_buffersize; ++i)
        {
            outl[i] *= newamplitude * pangainL;
            outr[i] *= newamplitude * pangainR;
        }
    }
    oldamplitude = newamplitude;
    computecurrentparameters();

    // Apply legato-specific sound signal modifications
    if (Legato.silent)
    {   // Silencer
        if (Legato.msg != LM_FadeIn)
        {
            memset(outl, 0, synth->sent_bufferbytes);
            memset(outr, 0, synth->sent_bufferbytes);
        }
    }
    switch (Legato.msg)
    {
        case LM_CatchUp : // Continue the catch-up...
            if (Legato.decounter == -10)
                Legato.decounter = Legato.fade.length;
            for (int i = 0; i < synth->sent_buffersize; ++i)
            {   // Yea, could be done without the loop...
                Legato.decounter--;
                if (Legato.decounter < 1)
                {
                    synth->part[synth->legatoPart]->legatoFading &= 5;
                    // Catching-up done, we can finally set
                    // the note to the actual parameters.
                    Legato.decounter = -10;
                    Legato.msg = LM_ToNorm;
                    SUBlegatonote(Legato.param.freq, Legato.param.vel,
                                  Legato.param.portamento, Legato.param.midinote,
                                  false);
                    break;
                }
            }
            break;

        case LM_FadeIn : // Fade-in
            if (Legato.decounter == -10)
                Legato.decounter = Legato.fade.length;
            Legato.silent = false;
            for (int i = 0; i < synth->sent_buffersize; ++i)
            {
                Legato.decounter--;
                if (Legato.decounter < 1)
                {
                    Legato.decounter = -10;
                    Legato.msg = LM_Norm;
                    break;
                }
                Legato.fade.m += Legato.fade.step;
                outl[i] *= Legato.fade.m;
                outr[i] *= Legato.fade.m;
            }
            break;

        case LM_FadeOut : // Fade-out, then set the catch-up
            if (Legato.decounter == -10)
                Legato.decounter = Legato.fade.length;
            for (int i = 0; i < synth->sent_buffersize; ++i)
            {
                Legato.decounter--;
                if (Legato.decounter < 1)
                {
                    for (int j = i; j < synth->sent_buffersize; ++j)
                        outl[j] = outr[j] = 0.0f;
                    Legato.decounter = -10;
                    Legato.silent = true;
                    // Fading-out done, now set the catch-up :
                    Legato.decounter = Legato.fade.length;
                    Legato.msg = LM_CatchUp;
                    // This freq should make this now silent note to catch-up
                    // (or should I say resync ?) with the heard note for the same
                    // length it stayed at the previous freq during the fadeout.
                    float catchupfreq =
                        Legato.param.freq * (Legato.param.freq / Legato.lastfreq);
                    SUBlegatonote(catchupfreq, Legato.param.vel,
                                 Legato.param.portamento, Legato.param.midinote,
                                 false);
                    break;
                }
                Legato.fade.m -= Legato.fade.step;
                outl[i] *= Legato.fade.m;
                outr[i] *= Legato.fade.m;
            }
            break;

        default :
            break;
    }

    // Check if the note needs to be computed more
    if (AmpEnvelope->finished() != 0)
    {
        for (int i = 0; i < synth->sent_buffersize; ++i)
        {   // fade-out
            float tmp = 1.0f - (float)i / synth->sent_buffersize_f;
            outl[i] *= tmp;
            outr[i] *= tmp;
        }
        KillNote();
    }
    return 1;
}


// Release Key (Note Off)
void SUBnote::releasekey(void)
{
    AmpEnvelope->releasekey();
    if (FreqEnvelope != NULL)
        FreqEnvelope->releasekey();
    if (BandWidthEnvelope != NULL)
        BandWidthEnvelope->releasekey();
    if (GlobalFilterEnvelope != NULL)
        GlobalFilterEnvelope->releasekey();
}

float SUBnote::getHgain(int harmonic)
{
    float hmagnew = 1.0f - pars->Phmag[pos[harmonic]] / 127.0f;
    float hgain;

    switch (pars->Phmagtype)
    {
        case 1:
            hgain = expf(hmagnew * log_0_01);
            break;

        case 2:
            hgain = expf(hmagnew * log_0_001);
            break;

        case 3:
            hgain = expf(hmagnew * log_0_0001);
            break;

        case 4:
            hgain = expf(hmagnew * log_0_00001);
            break;

        default:
            hgain = 1.0f - hmagnew;
    }

    return hgain;
}

void SUBnote::updatefilterbank(void)
{
    int createdFilters = createNewFilters();

    // moved from noteon
    // how much the amplitude is normalised (because the harmonics)
    float reduceamp = 0.0;

    for (int n = 0; n < numharmonics; ++n)
    {
        float freq =  basefreq * pars->POvertoneFreqMult[pos[n]];
        overtone_freq[n] = freq;
        overtone_rolloff[n] = computerolloff(freq);

        // the bandwidth is not absolute(Hz); it is relative to frequency
        float bw = powf(10.0f, (pars->Pbandwidth - 127.0f) / 127.0f * 4.0f) * numstages;

        // Bandwidth Scale
        bw *= powf(1000.0f / freq, (pars->Pbwscale - 64.0f) / 64.0f * 3.0f);

        // Relative BandWidth
        bw *= powf(100.0f, (pars->Phrelbw[pos[n]] - 64.0f) / 64.0f);

        if (bw > 25.0f)
            bw = 25.0f;

        // try to keep same amplitude on all freqs and bw. (empirically)
        float gain = sqrtf(1500.0f / (bw * freq));

        float hgain = getHgain(n);

        gain *= hgain;
        reduceamp += hgain;

        for (int nph = 0; nph < numstages; ++nph)
        {
            float amp = 1.0f;
            if (nph == 0)
                amp = gain;
            bpfilter *filter = &lfilter[nph + n * numstages];
            filter->amp = amp;
            filter->freq = freq + OffsetHz;
            filter->bw = bw;
            if (stereo)
            {
                filter = &rfilter[nph + n * numstages];
                filter->amp = amp;
                filter->freq = freq + OffsetHz;
                filter->bw = bw;
            }
        }
    }

    initfilters(numharmonics - createdFilters);
    computeallfiltercoefs();

    if (reduceamp < 0.001f)
        reduceamp = 1.0f;
    volume /= reduceamp;

}

