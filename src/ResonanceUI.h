// ResonanceUI.h
// generated by Fast Light User Interface Designer (fluid) version 1.0302

#ifndef ResonanceUI_h
#define ResonanceUI_h
#include <FL/Fl.H>
// Original ZynAddSubFX author Nasca Octavian Paul
// Copyright (C) 2002-2005 Nasca Octavian Paul
// Copyright 2009-2010, Alan Calvert

// This file is part of yoshimi, which is free software: you can redistribute
// it and/or modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.

// yoshimi is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.   See the GNU General Public License (version 2 or
// later) for more details.

// You should have received a copy of the GNU General Public License along with
// yoshimi; if not, write to the Free Software Foundation, Inc., 51 Franklin
// Street, Fifth Floor, Boston, MA  02110-1301, USA.

// This file is a derivative of the ZynAddSubFX original, modified January 2010

#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Value_Output.H>
#include "WidgetPDialUI.h"
#include "PresetsUI.h"
#include "Synth/Resonance.h"
#include "Misc/SynthEngine.h"

class ResonanceGraph : public Fl_Box {
public:
  ResonanceGraph(int x,int y, int w, int h, const char *label=0);
  void init(Resonance *respar_,Fl_Value_Output *khzvalue_,Fl_Value_Output *dbvalue_);
  void draw_freq_line(float freq,int type);
  void draw();
  int handle(int event);
  void setcbwidget(Fl_Widget *cbwidget,Fl_Widget *applybutton);
private:
  Fl_Value_Output *khzvalue; 
  Fl_Value_Output *dbvalue; 
  Resonance *respar; 
  int oldx,oldy; 
public:
  float khzval; 
private:
  Fl_Widget *cbwidget,*applybutton; 
};
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Value_Output.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Roller.H>
#include <FL/Fl_Box.H>

class ResonanceUI : PresetsUI_ {
public:
  Fl_Double_Window* make_window();
  Fl_Double_Window *resonancewindow;
  Fl_Value_Output *khzvalue;
  Fl_Value_Output *dbvalue;
private:
  inline void cb_Close_i(Fl_Button*, void*);
  static void cb_Close(Fl_Button*, void*);
  inline void cb_Zero_i(Fl_Button*, void*);
  static void cb_Zero(Fl_Button*, void*);
  inline void cb_Smooth_i(Fl_Button*, void*);
  static void cb_Smooth(Fl_Button*, void*);
public:
  Fl_Check_Button *enabled;
private:
  inline void cb_enabled_i(Fl_Check_Button*, void*);
  static void cb_enabled(Fl_Check_Button*, void*);
public:
  Fl_Roller *maxdb;
private:
  inline void cb_maxdb_i(Fl_Roller*, void*);
  static void cb_maxdb(Fl_Roller*, void*);
public:
  Fl_Value_Output *maxdbvo;
private:
  inline void cb_maxdbvo_i(Fl_Value_Output*, void*);
  static void cb_maxdbvo(Fl_Value_Output*, void*);
public:
  Fl_Value_Output *centerfreqvo;
private:
  inline void cb_centerfreqvo_i(Fl_Value_Output*, void*);
  static void cb_centerfreqvo(Fl_Value_Output*, void*);
public:
  Fl_Value_Output *octavesfreqvo;
private:
  inline void cb_octavesfreqvo_i(Fl_Value_Output*, void*);
  static void cb_octavesfreqvo(Fl_Value_Output*, void*);
  inline void cb_RND2_i(Fl_Button*, void*);
  static void cb_RND2(Fl_Button*, void*);
  inline void cb_RND1_i(Fl_Button*, void*);
  static void cb_RND1(Fl_Button*, void*);
  inline void cb_RND3_i(Fl_Button*, void*);
  static void cb_RND3(Fl_Button*, void*);
public:
  Fl_Check_Button *p1st;
private:
  inline void cb_p1st_i(Fl_Check_Button*, void*);
  static void cb_p1st(Fl_Check_Button*, void*);
  inline void cb_InterpP_i(Fl_Button*, void*);
  static void cb_InterpP(Fl_Button*, void*);
public:
  WidgetPDial *centerfreq;
private:
  inline void cb_centerfreq_i(WidgetPDial*, void*);
  static void cb_centerfreq(WidgetPDial*, void*);
public:
  WidgetPDial *octavesfreq;
private:
  inline void cb_octavesfreq_i(WidgetPDial*, void*);
  static void cb_octavesfreq(WidgetPDial*, void*);
  inline void cb_C_i(Fl_Button*, void*);
  static void cb_C(Fl_Button*, void*);
  inline void cb_P_i(Fl_Button*, void*);
  static void cb_P(Fl_Button*, void*);
public:
  Fl_Button *applybutton;
private:
  inline void cb_applybutton_i(Fl_Button*, void*);
  static void cb_applybutton(Fl_Button*, void*);
public:
  ResonanceUI(Resonance *respar_);
  virtual ~ResonanceUI();
  void redrawPADnoteApply();
  void setcbwidget(Fl_Widget *cbwidget,Fl_Widget *cbapplywidget);
  void refresh();
  Resonance *respar; 
private:
  ResonanceGraph *rg; 
  Fl_Widget *cbwidget,*cbapplywidget; 
  SynthEngine *synth; 
};
#endif
