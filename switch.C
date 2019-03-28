/*
    Copyright (C) 2003 and onward Robert Muth <robert at muth dot org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    $Id: switch.C 1175 2009-04-19 16:54:26Z muth $
*/

/* ================================================================== */
// imports
/* ================================================================== */

// C++ imports
#include <fstream>
#include <iostream>
#include <string>

// Local imports
using namespace std;
#include "basic.H"
#include "switch.H"

// ========================================================================
UINT32 SWITCH::SwitchNumber() {
  UINT32 n = 0;

  for (SWITCH *sw = _list; sw; sw = sw->_next) {
    n++;
  }

  return n;
}

// ========================================================================
SWITCH *SWITCH::SwitchFind(const string &name, BOOL enabled) {
  for (SWITCH *sw = _list; sw; sw = sw->_next) {
    if (enabled && !sw->_enabled)
      continue;

    if (sw->_name == name)
      return sw;
  }

  return 0;
}

// ========================================================================
VOID SWITCH::FamilyDisable(const string &family) {
  for (SWITCH *sw = _list; sw; sw = sw->_next) {
    if (0 == sw->_family.find(family)) {
      sw->_enabled = 0;
    }
  }
}

// ========================================================================
VOID SWITCH::FamilyEnable(const string &family) {
  for (SWITCH *sw = _list; sw; sw = sw->_next) {
    if (0 == sw->_family.find(family)) {
      sw->_enabled = 1;
    }
  }
}

// ========================================================================
VOID SWITCH::SwitchDisable(const string &name) {
  SWITCH *sw = SwitchFind(name);
  ASSERTX(sw);
  sw->_enabled = 0;
}

// ========================================================================
VOID SWITCH::SwitchEnable(const string &name) {
  SWITCH *sw = SwitchFind(name);
  ASSERTX(sw);
  sw->_enabled = 1;
}

// ========================================================================
UINT32 SWITCH::ValueNumber() const {
  UINT32 n = 0;

  for (VALUE *v = _value; v; v = v->_next)
    n++;
  return n;
}

// ========================================================================
VALUE *SWITCH::ValueGetByIndex(UINT32 index) const {
  VALUE *v;
  for (v = _value; v && index > 0; v = v->_next, index--)
    ;

  ASSERTX(v != 0);
  return v;
}

// ========================================================================
VALUE *SWITCH::ValueAdd(const string &value) {
  if (_mode == SWITCH_MODE_OVERWRITE) {
    delete _value;
    _value = new VALUE(value);
    return _value;
  } else if (_mode == SWITCH_MODE_ACCUMULATE) {
    VALUE *v;

    for (v = _value; v->_next; v = v->_next)
      ;

    v->_next = new VALUE(value);
    return v->_next;
  } else {
    ASSERTZ("unknown mode\n");
    return 0;
  }
}

// ========================================================================
static int cmp(const void *x1, const void *x2) {
  SWITCH *o1 = *(SWITCH **)x1;
  SWITCH *o2 = *(SWITCH **)x2;

  int result = o1->Family().compare(o2->Family());
  if (result == 0)
    result = o1->Name().compare(o2->Name());
  return result;
}

// ========================================================================
string StringLong(SWITCH_TYPE type) {
  switch (type) {
  case SWITCH_TYPE_BOOL:
    return "";
  case SWITCH_TYPE_INT32:
    return "integer-32";
  case SWITCH_TYPE_FLT32:
    return "floa-32";
  case SWITCH_TYPE_STRING:
    return "string";
  default:
    ASSERTX(0);
    return "";
  }
}

// ========================================================================
string StringShort(SWITCH_TYPE type) {
  switch (type) {
  case SWITCH_TYPE_BOOL:
    return "B";
  case SWITCH_TYPE_INT32:
    return "I";
  case SWITCH_TYPE_FLT32:
    return "F";
  case SWITCH_TYPE_STRING:
    return "S";
  default:
    ASSERTX(0);
    return "X";
  }
}

// ========================================================================
string StringShort(SWITCH_MODE mode) {
  switch (mode) {
  case SWITCH_MODE_ACCUMULATE:
    return "AC";
  case SWITCH_MODE_OVERWRITE:
    return "OV";
  default:
    ASSERTX(0);
    return "XZ";
  }
}

// ========================================================================
string SWITCH::SwitchSummary(BOOL enabled, BOOL listing) {
  typedef SWITCH *PSWITCH; // for array new below to supress warning

  INT32 count = SWITCH::SwitchNumber();

  SWITCH **array = new PSWITCH[count];

  INT32 i = 0;
  for (SWITCH *sw = _list; sw; sw = sw->_next) {
    array[i] = sw;
    i++;
  }

  ASSERTX(count == i);

  qsort(array, count, sizeof(SWITCH *), cmp);

  string ostr;

  if (listing) {
    ostr += ljstr("Family", 15 + 1);
    ostr += ljstr("Knob", 20 + 1);
    ostr += ljstr("Flag", 4 + 1);
    ostr += ljstr("Mode", 4 + 1);
    ostr += ljstr("Type", 4 + 1);
    ostr += string("Value");
  }

  for (i = 0; i < count; i++) {
    SWITCH *sw = array[i];

    if (enabled && !sw->_enabled)
      continue;

    if (listing) {
      ostr += ljstr(sw->_family, 15 + 1);
      ostr += ljstr(sw->_name, 20 + 1);
      ostr += "[" + string(sw->_enabled ? "E" : "_") + "]  ";
      ostr += StringShort(sw->_mode) + "  ";
      ostr += StringShort(sw->_type) + "  ";
      ostr += "[ ";

      BOOL add_space = 0;

      for (VALUE *v = sw->_value; v; v = v->_next) {
        cout << "found " << v->_value << endl;

        if (add_space) {
          ostr += " ";
        } else {
          add_space = 1;
        }

        ostr += v->_value;
      }

      ostr += "]\n";
    } else {
      ostr += sw->_name + " " + StringLong(sw->_type) + "   default [";

      string sep = "";
      for (VALUE *v = sw->_value; v; v = v->_next) {
        ostr += sep + "\"" + v->_value + "\"";
        sep = "  ";
      }

      ostr += "]\n";

      ostr += Reformat(sw->_purpose, "\t", 20, 75);
    }
  }

  delete[] array;
  return ostr;
}

// ========================================================================

SWITCH *SWITCH::_list = 0;

// ========================================================================
#ifdef STAND_ALONE

SWITCH SwicthVerbose("verbose", "general", SWITCH_TYPE_BOOL,
                     SWITCH_MODE_OVERWRITE, "0",
                     "enable verbose message output");

SWITCH SwitchNumber("number", "general", SWITCH_TYPE_INT32,
                    SWITCH_MODE_OVERWRITE, "666", "blurb");

SWITCH SwitchFloat("float", "general", SWITCH_TYPE_FLT32, SWITCH_MODE_OVERWRITE,
                   "0.666", "blurb");

SWITCH SwicthTest("test", "general", SWITCH_TYPE_BOOL, SWITCH_MODE_OVERWRITE,
                  "0", "blurb");

SWITCH opt_odouble("odouble", "optimize", SWITCH_TYPE_STRING,
                   SWITCH_MODE_OVERWRITE, STRING_INVALID, "blurb");

int main(int argc, char *argv[]) {
  cout << "HELP\n"
          "==========================\n";

  cout += SWITCH::SwitchSummary(1, 0);

  for (argc--, argv++; argc > 0; argc--, argv++) {
    if (*argv[0] == '-') {
      SWITCH *sw = SWITCH::SwitchFind(argv[0] + 1);
      if (sw == 0) {
        cout += "bad option " += argv[0] += endl;
        exit(-1);
      }

      cout += "good option " += argv[0] += endl;

      if (sw->Type() == SWITCH_TYPE_BOOL) {
        sw->ValueAdd("1");
      } else {
        ASSERTX(argc > 0);
        argc--;
        argv++;
        sw->ValueAdd(argv[0]);
      }
    }
  }

  cout << "STATS AFTER\n"
          "==========================\n";

  cout << SWITCH::SwitchSummary(1, 1);

  return 0;
}

#endif
