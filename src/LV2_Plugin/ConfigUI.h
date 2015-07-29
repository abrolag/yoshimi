// Original ZynAddSubFX author Nasca Octavian Paul
// Copyright (C) 2002-2005 Nasca Octavian Paul
// Copyright 2009-2011, Alan Calvert
// Copyright 2014, Will Godfrey   

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

// This file is derivative of ZynAddSubFX original code, modified September 2014

// generated by Fast Light User Interface Designer (fluid) version 1.0302

#ifndef ConfigUI_h
#define ConfigUI_h
#include <FL/Fl.H>
#include <cmath>
#include <FL/Fl_File_Chooser.H>
#include "Misc/Config.h"
#include "Misc/MiscFuncs.h"
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Check_Browser.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Check_Button.H>

class ConfigUI : private MiscFuncs {
public:
  Fl_Double_Window* make_window();
  Fl_Double_Window *configwindow;
private:
  inline void cb_configwindow_i(Fl_Double_Window*, void*);
  static void cb_configwindow(Fl_Double_Window*, void*);
  inline void cb_AddSynth_i(Fl_Choice*, void*);
  static void cb_AddSynth(Fl_Choice*, void*);
  static Fl_Menu_Item menu_AddSynth[];
  inline void cb_Internal_i(Fl_Choice*, void*);
  static void cb_Internal(Fl_Choice*, void*);
  static Fl_Menu_Item menu_Internal[];
  inline void cb_PADsynth_i(Fl_Choice*, void*);
  static void cb_PADsynth(Fl_Choice*, void*);
  static Fl_Menu_Item menu_PADsynth[];
  inline void cb_Virtual_i(Fl_Choice*, void*);
  static void cb_Virtual(Fl_Choice*, void*);
  static Fl_Menu_Item menu_Virtual[];
public:
  Fl_Counter *compression;
private:
  inline void cb_compression_i(Fl_Counter*, void*);
  static void cb_compression(Fl_Counter*, void*);
  inline void cb_Send_i(Fl_Choice*, void*);
  static void cb_Send(Fl_Choice*, void*);
  static Fl_Menu_Item menu_Send[];
public:
  Fl_Input *SessionStateFile;
private:
  inline void cb_SessionStateFile_i(Fl_Input*, void*);
  static void cb_SessionStateFile(Fl_Input*, void*);
public:
  Fl_Button *StateFileButton;
private:
  inline void cb_StateFileButton_i(Fl_Button*, void*);
  static void cb_StateFileButton(Fl_Button*, void*);
public:
  Fl_Browser *presetbrowse;
private:
  inline void cb_presetbrowse_i(Fl_Browser*, void*);
  static void cb_presetbrowse(Fl_Browser*, void*);
  inline void cb_Add_i(Fl_Button*, void*);
  static void cb_Add(Fl_Button*, void*);
public:
  Fl_Button *removepresetbutton;
private:
  inline void cb_removepresetbutton_i(Fl_Button*, void*);
  static void cb_removepresetbutton(Fl_Button*, void*);
public:
  Fl_Button *makedefaultpresetbutton;
private:
  inline void cb_makedefaultpresetbutton_i(Fl_Button*, void*);
  static void cb_makedefaultpresetbutton(Fl_Button*, void*);
  inline void cb_Jack_i(Fl_Input*, void*);
  static void cb_Jack(Fl_Input*, void*);
  inline void cb_Alsa_i(Fl_Input*, void*);
  static void cb_Alsa(Fl_Input*, void*);
  inline void cb_Alsa1_i(Fl_Input*, void*);
  static void cb_Alsa1(Fl_Input*, void*);
  inline void cb_Samplerate_i(Fl_Check_Browser*, void*);
  static void cb_Samplerate(Fl_Check_Browser*, void*);
  inline void cb_MIDI_i(Fl_Group*, void*);
  static void cb_MIDI(Fl_Group*, void*);
public:
  Fl_Spinner *BankRoot;
private:
  inline void cb_BankRoot_i(Fl_Spinner*, void*);
  static void cb_BankRoot(Fl_Spinner*, void*);
public:
  Fl_Button *Rpend;
private:
  inline void cb_Rpend_i(Fl_Button*, void*);
  static void cb_Rpend(Fl_Button*, void*);
  inline void cb_Enable_i(Fl_Check_Button*, void*);
  static void cb_Enable(Fl_Check_Button*, void*);
public:
  Fl_Choice *bankchange;
private:
  inline void cb_bankchange_i(Fl_Choice*, void*);
  static void cb_bankchange(Fl_Choice*, void*);
  static Fl_Menu_Item menu_bankchange[];
public:
  static Fl_Menu_Item *banklsb;
  static Fl_Menu_Item *bankusb;
  static Fl_Menu_Item *bankoff;
private:
  inline void cb_Enable1_i(Fl_Check_Button*, void*);
  static void cb_Enable1(Fl_Check_Button*, void*);
  inline void cb_Enable2_i(Fl_Check_Button*, void*);
  static void cb_Enable2(Fl_Check_Button*, void*);
public:
  Fl_Spinner *EPCspinner;
private:
  inline void cb_EPCspinner_i(Fl_Spinner*, void*);
  static void cb_EPCspinner(Fl_Spinner*, void*);
public:
  Fl_Button *Epend;
private:
  inline void cb_Epend_i(Fl_Button*, void*);
  static void cb_Epend(Fl_Button*, void*);
  inline void cb_Enable3_i(Fl_Check_Button*, void*);
  static void cb_Enable3(Fl_Check_Button*, void*);
public:
  Fl_Spinner *Control;
private:
  inline void cb_Control_i(Fl_Spinner*, void*);
  static void cb_Control(Fl_Spinner*, void*);
public:
  Fl_Spinner *Value;
private:
  inline void cb_Value_i(Fl_Spinner*, void*);
  static void cb_Value(Fl_Spinner*, void*);
public:
  Fl_Button *CCpend;
private:
  inline void cb_CCpend_i(Fl_Button*, void*);
  static void cb_CCpend(Fl_Button*, void*);
  inline void cb_Close_i(Fl_Button*, void*);
  static void cb_Close(Fl_Button*, void*);
  inline void cb_Save_i(Fl_Button*, void*);
  static void cb_Save(Fl_Button*, void*);
public:
  ConfigUI(SynthEngine *_synth);
  void activatebutton_presetdir(bool active);
  int recoverID();
  void readpresetcfg();
  void writepresetcfg();
  void Show();
  void Hide();
private:
  void savesettings(void);
public:
  bool configChanged; 
private:
  SynthEngine *synth; 
  long selectedRootID; 
  int EPCvalue; 
  int BankRootvalue; 
  int oldBank; 
  int oldrootID; 
};
#endif
