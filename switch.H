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

    $Id: switch.H 1175 2009-04-19 16:54:26Z muth $
*/

#ifndef SWITCH_H
#define SWITCH_H

#include <cstdlib>
#include <string>

typedef enum {
  SWITCH_TYPE_INVALID,
  SWITCH_TYPE_BOOL,
  SWITCH_TYPE_INT32,
  SWITCH_TYPE_FLT32,
  SWITCH_TYPE_STRING,
  SWITCH_TYPE_LAST
} SWITCH_TYPE;

typedef enum {
  SWITCH_MODE_INVALID,
  SWITCH_MODE_OVERWRITE,
  SWITCH_MODE_ACCUMULATE,
  SWITCH_MODE_LAST
} SWITCH_MODE;

const std::string STRING_INVALID("*");

class VALUE {
public:
  VALUE *_next;
  const std::string _value;

  VALUE(const std::string &value) : _next(0), _value(value) {}

  BOOL ValBool() { return 0 != strtol(_value.c_str(), NULL, 0); }
  const std::string &ValString() { return _value; }
  UINT32 ValInt32() { return (INT32)strtol(_value.c_str(), NULL, 0); }
  FLT32 ValFlt32() { return (FLT32)strtod(_value.c_str(), NULL); }
};

class SWITCH {
private:
  static SWITCH *_list;

private:
  SWITCH *_next;
  const std::string _family;
  const std::string _name;
  const std::string _purpose;
  const SWITCH_TYPE _type;
  const SWITCH_MODE _mode;
  BOOL _enabled;
  VALUE *_value;

public:
  static UINT32 SwitchNumber();

  static VOID FamilyDisable(const std::string &family);

  static VOID FamilyEnable(const std::string &family);

  static VOID SwitchDisable(const std::string &name);

  static VOID SwitchEnable(const std::string &name);

  static std::string SwitchSummary(BOOL enabled = 1, BOOL listing = 0);

  static SWITCH *SwitchFind(const std::string &name, BOOL enabled = 1);

public:
  SWITCH_TYPE Type() const { return _type; }

  SWITCH_MODE Mode() const { return _mode; }

  std::string Family() const { return _family; }

  std::string Name() const { return _name; }

  UINT32 ValueNumber() const;

  VALUE *ValueGetByIndex(UINT32 index) const;

  VALUE *ValueAdd(const std::string &value);

  SWITCH(const std::string &name, const std::string &family, SWITCH_TYPE type,
         SWITCH_MODE mode, const std::string &value, const std::string &purpose)
      : _next(_list), _family(family), _name(name), _purpose(purpose),
        _type(type), _mode(mode), _enabled(1), _value(new VALUE(value)) {
    SWITCH *sw = SwitchFind(name); // duplicate check

    if (sw != 0) {
      ASSERTZ("duplicate switch " + name + "\n");
    }

    _list = this;
  }

  BOOL ValueBool(UINT32 index = 0) const {
    ASSERTX(_type == SWITCH_TYPE_BOOL);
    return ValueGetByIndex(index)->ValBool();
  }

  INT32 ValueInt32(UINT32 index = 0) const {
    ASSERTX(_type == SWITCH_TYPE_INT32);
    return ValueGetByIndex(index)->ValInt32();
  }

  const std::string &ValueString(UINT32 index = 0) const {
    ASSERTX(_type == SWITCH_TYPE_STRING);
    return ValueGetByIndex(index)->ValString();
  }

  FLT32 ValueFlt32(UINT32 index = 0) const {
    ASSERTX(_type == SWITCH_TYPE_FLT32);
    return ValueGetByIndex(index)->ValFlt32();
  }
};

#endif
