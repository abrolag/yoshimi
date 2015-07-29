// Copyright 2011, Alan Calvert
// Copyright 2014, Will Godfrey   

// This file is part of yoshimi, which is free software: you can
// redistribute it and/or modify it under the terms of the GNU General
// Public License as published by the Free Software Foundation, either
// version 3 of the License, or (at your option) any later version.

// yoshimi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with yoshimi.  If not, see <http://www.gnu.org/licenses/>.

// Modified September 2014

// generated by Fast Light User Interface Designer (fluid) version 1.0302

#ifndef ParametersUI_h
#define ParametersUI_h
#include <FL/Fl.H>
#include <set>
using namespace std;
#include "Misc/SynthEngine.h"
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>

class ParametersUI {
public:
  Fl_Double_Window* make_window(void);
  Fl_Double_Window *Recent;
private:
  inline void cb_Recent_i(Fl_Double_Window*, void*);
  static void cb_Recent(Fl_Double_Window*, void*);
public:
  Fl_Browser *BrowseRecent;
private:
  inline void cb_BrowseRecent_i(Fl_Browser*, void*);
  static void cb_BrowseRecent(Fl_Browser*, void*);
public:
  Fl_Button *Loading;
  Fl_Button *CloseRecent;
private:
  inline void cb_CloseRecent_i(Fl_Button*, void*);
  static void cb_CloseRecent(Fl_Button*, void*);
public:
  ParametersUI(SynthEngine *_synth);
  void loadRecent();
  void Show();
  void Hide();
  ~ParametersUI();
private:
  SynthEngine *synth; 
};
#endif
