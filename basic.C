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
*/

/*! @file
 */

/* ================================================================== */
// imports
/* ================================================================== */

// C imports
// for backtrace
#include <execinfo.h>
#include <signal.h>
// this includes are only needed to be able
// to use the unix process accounting functions
#include <sys/resource.h>
#include <sys/time.h>

// C++ imports
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

// Local imports
using namespace std;
#include "basic.H"

// ========================================================================

LOCALVAR CHAR DisabledPrefices[256]; // zero initialized

LOCALVAR BOOL has_executed_before = FALSE;

LOCALVAR string ImageName("");

LOCALVAR VOID (*TerminateCallBack)() = DefaultTerminmate;
LOCALVAR VOID (*TraceCallBack)() = DefaultTrace;
LOCALVAR string (*ResinfoCallBack)() = DefaultResinfo;

GLOBALVAR const string Line1(80, '#');
GLOBALVAR const string Line2(80, '=');
GLOBALVAR const string Line3(80, '-');
GLOBALVAR const string Line4(80, '.');

// ========================================================================

/*!
  Disable printing of all messages with a given prefix, e.g.
  W for warning, I for Info, etc..
 */

GLOBALFUN VOID DisableMessage(UINT32 prefix) { DisabledPrefices[prefix] = 1; }

// ========================================================================
/*!
  In order for the stack tracing to work the image location has be made known.
 (This is used for extracting symbol table info)
*/

GLOBALFUN VOID RegisterImageName(const string &name) { ImageName = name; }

// ========================================================================
/*!

*/

GLOBALFUN VOID RegisterNewTerminate(VOID (*cb)()) { TerminateCallBack = cb; }

// ========================================================================
/*!
  All messages are eventually channeled through this routine,
  If term is TRUE, the program will be exited.
  If trace is TRUE we attempt to print a stack trace.
 */

GLOBALFUN VOID Message(const string &prefix, const string &message, BOOL term,
                       BOOL trace, BOOL resinfo) {
  ostream &out = cout;

  if (!DisabledPrefices[UINT32(prefix[0])]) {
    string m = message;
    if (resinfo)
      m = ResinfoCallBack() + " " + message;

    UINT32 pos_start = 0;
    const UINT32 length = m.length();

    while (pos_start < length) {
      out << prefix;

      UINT32 pos_end = pos_start;
      while (pos_end < length && m[pos_end] != '\n') {
        pos_end++;
      }

      if (pos_end < length && m[pos_end] == '\n')
        pos_end++;

      out << m.substr(pos_start, pos_end - pos_start);

      pos_start = pos_end;
    }
  }

  if (trace) {
    TraceCallBack();
  }

  if (term) {
    TerminateCallBack();

    // To avoid dumping stack due the forced "Floating exception" below
    has_executed_before = TRUE;
    // force a "Floating exception" so that gdb stops here automaticall.
    __builtin_trap();
    exit(-1);
  }
}

// ========================================================================

GLOBALFUN VOID DefaultTrace() {
#if defined(__linux__)

  // Store up to SIZE return address of the current program state in
  // ARRAY and return the exact number of values stored.

  const int size = 128;
  void *array[size];

  int actual_size = backtrace(array, size);

  // Return names of functions from the backtrace list in ARRAY in a newly
  // allocated memory block.

  string prefix("T: ");

  //
  // the stack trace seems to not work in ia64
  //
#if 0
    char **trace  = backtrace_symbols(array,size);

    string suffix("\n");


    for( int i=0; i < actual_size; i++ )
    {
        //Print("%d %p %s\n",i,array[i],trace[i]);

        //Print(prefix,string("fdfdsfsdf\n"));

        //Print(prefix,string(trace[i]) + suffix);
    }

    Print(prefix,
          "use \"addr2line -f -e img line1 ...\" to obtain symbolic addresses\n");
#else
  ostream &out = cout;

  string commandline;

  commandline += "addr2line -C -f -e ";
  commandline += ImageName;

  for (int i = 1; i < actual_size; i++) {
    commandline += " " + ptrstr(array[i]);
  }

  out << Line1 << endl;
  out << "## STACK TRACE" << endl;
  out << Line1 << endl;

  out << commandline << endl;

  int result = system(commandline.c_str());
  if (result) {
    out << "addr2line failed with " << result;
  }

#endif
#endif
}

// ========================================================================

LOCALFUN UINT32 MilliSecondsElapsed() {
  ASSERTX(CLOCKS_PER_SEC == 1000000);

  static clock_t old_time = 0;

  clock_t new_time = clock();

  UINT32 delta = (new_time - old_time) / 1000;

  old_time = new_time;

  return delta;
}

// ========================================================================
LOCALFUN UINT32 KiloBytesUsed() {
  ifstream is("/proc/self/status");

  string tag;
  string value;

  while (!is.eof()) {
    is >> tag >> value;
    is.ignore(1000, '\n');
    // cout << tag << "  " << value << endl;
    if (tag == string("VmSize:"))
      return strtol(value.c_str(), NULL, 0);
  }

  ASSERTX(0);
  return 0;
}

// ========================================================================
GLOBALFUN string DefaultResinfo() {
  return string("[") + StringDec(MilliSecondsElapsed(), 6) + "ms," +
         StringDec(KiloBytesUsed() / 1024L, 4) + "MB]";
}

// ========================================================================
GLOBALFUN VOID DefaultTerminmate() { exit(-1); }

// ========================================================================
LOCALFUN VOID DefaultSignalHandler(int __attribute__((unused)) arg) {
  if (!has_executed_before) {
    TraceCallBack();
    TerminateCallBack();
  }

  exit(-1);
}

// ========================================================================
GLOBALFUN VOID InstallSignalHandlers() {
  signal(2, DefaultSignalHandler);
  signal(3, DefaultSignalHandler);
  signal(8, DefaultSignalHandler);
  signal(10, DefaultSignalHandler);
  signal(11, DefaultSignalHandler);
}

// ========================================================================
GLOBALFUN string ljstr(const string &s, UINT32 width, CHAR padding) {
  string ostr(padding, width);
  ostr.replace(0, s.length(), s);
  return ostr;
}

// ========================================================================
/*!
  convert a UINT32 into a dec string. Padding can be specified as well
 */

GLOBALFUN string StringDec(UINT32 l, UINT32 digits, CHAR padding) {
  CHAR buffer[32];

  UINT32 i = 31;
  buffer[i] = '\0';

  do {
    i--;
    buffer[i] = '0' + l % 10;
    l /= 10;
  } while (l);

  digits = sizeof(buffer) - digits - 1;
  while (i > digits) {
    i--;
    buffer[i] = padding;
  }

  return string(&buffer[i]);
}

// ========================================================================
/*!
  convert an INT32 into a dec string. Padding can be specified as well
 */

GLOBALFUN string StringDecSigned(INT32 l, UINT32 digits, CHAR padding) {

  const BOOL negative = (l < 0);

  if (l < 0)
    l = -l; // FIXME: avoid bug for negative num that cannot be negated

  CHAR buffer[32];

  UINT32 i = 31;
  buffer[i] = '\0';

  do {
    i--;
    buffer[i] = '0' + l % 10;
    l /= 10;
  } while (l);

  if (negative) {
    i--;
    buffer[i] = '-';
  }

  // FIXME bug minus signed should follow after padding

  digits = sizeof(buffer) - digits - 1;
  while (i > digits) {
    i--;
    buffer[i] = padding;
  }

  return string(&buffer[i]);
}

/*  ================================================================== */
/*!
  convert a UINT32 into a hex string. Padding can be specified as well
 */

GLOBALFUN string StringHex(UINT32 l, UINT32 digits) {
  ASSERT(digits < 20, "bad width");
  CHAR buffer[32];
  UINT32 i = 31;
  UINT32  digits_so_far = 0;
  buffer[i] = '\0';

  do {
    i--;
    UINT32 digit = l % 16;
    if (digit > 9)
      digit += 'a' - 10;
    else
      digit += '0';

    buffer[i] = digit;
    digits_so_far++;
    l /= 16;
  } while (l);

  while (digits_so_far < digits) {
    digits_so_far++;
    i--;
    buffer[i] = '0';
  }

  i--;
  buffer[i] = 'x';
  i--;
  buffer[i] = '0';

  return string(&buffer[i]);
}

// ========================================================================
GLOBALFUN string Reformat(const string &s, const string &prefix,
                          UINT32 min_line, UINT32 max_line) {
  string out;

  const UINT32 length = s.length();

  UINT32 pos_start = 0;

  for (;;) {
    // skip of leading whitespace

    while (pos_start < length) {
      if (isspace(s[pos_start])) {
        pos_start++;
      } else {
        break;
      }
    }

    if (pos_start >= length)
      break;

    // find suitable line break about 72 chars from start
    // but no closer than 20 to start

    UINT32 pos_end = pos_start + max_line;

    if (pos_end > length) {
      pos_end = length;
    } else {
      while (pos_end - pos_start >= min_line) {
        if (isspace(s[pos_end - 1])) {
          break;
        } else {
          pos_end--;
        }
      }
    }

    out += prefix;
    out += s.substr(pos_start, pos_end - pos_start);
    out += "\n";
    pos_start = pos_end;
  }

  return out;
}

// ========================================================================
GLOBALFUN UINT32 Tokenize(const string &line, string *array, UINT32 n) {
  const UINT32 length = line.length();

  UINT32 pos_start = 0;
  UINT32 count = 0;

  for (; count < n; count++) {
    // skip leading white space
    while (pos_start < length) {
      if (isspace(line[pos_start])) {
        pos_start++;
      } else {
        break;
      }
    }

    if (pos_start >= length)
      break;

    // find toked end

    UINT32 pos_end = pos_start;

    while (pos_end < length) {
      if (isspace(line[pos_end])) {
        break;
      } else {
        pos_end++;
      }
    }

    array[count] = line.substr(pos_start, pos_end - pos_start);

    pos_start = pos_end;
  }

  return count;
}

// ========================================================================
GLOBALFUN BOOL CaseCompare(const string &s1, const string &s2) {
  if (s2.size() != s1.size())
    return false;
  for (unsigned int i = 0; i < s1.size(); ++i)
    if (tolower(s1[i]) != tolower(s2[i]))
      return false;
  return true;
}

// ========================================================================
