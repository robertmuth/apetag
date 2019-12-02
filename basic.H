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

#ifndef BASIC_H
#define BASIC_H

#include <stdint.h>
#include <string>
// ========================================================================
#define GLOBALFUN extern
#define LOCALFUN static
#define MEMBERFUN

#define MEMBERVAR
#define GLOBALVAR
#define LOCALVAR static

#define LOCALTYPE
#define GLOBALTYPE

// ========================================================================
typedef int32_t INT32;
typedef int16_t INT16;
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uintptr_t PTRINT;

typedef bool BOOL;
typedef float FLT32;
typedef double FLT64;
typedef void VOID;
typedef char CHAR;

#ifndef TRUE
#define TRUE true
#endif

#ifndef FALSE
#define FALSE false
#endif

// ========================================================================
extern std::string StringHex(UINT32 val, UINT32 width = 0);

extern std::string StringDec(UINT32 val, UINT32 width = 0, CHAR padding = ' ');

extern std::string StringDecSigned(INT32 val, UINT32 width = 0,
                                   CHAR padding = ' ');

extern std::string StringDecSignedBignum(INT32 val, UINT32 width = 0,
                                         CHAR padding = ' ');

extern std::string StringHexBignum(UINT32 val, UINT32 width = 0,
                                   CHAR padding = ' ');

extern std::string StringFlt(FLT64 val, UINT32 precision = 0, UINT32 width = 0);

extern std::string StringBool(BOOL b);

// ========================================================================

inline std::string decstr(INT32 val, UINT32 width = 0) {
  return StringDecSigned((INT32)val, width);
}
inline std::string decstr(INT16 val, UINT32 width = 0) {
  return StringDecSigned((INT32)val, width);
}

inline std::string decstr(UINT32 val, UINT32 width = 0) {
  return StringDec((INT32)val, width);
}
inline std::string decstr(UINT16 val, UINT32 width = 0) {
  return StringDec((INT32)val, width);
}

inline std::string hexstr(INT32 val, UINT32 width = 0) {
  return StringHex((INT32)val, width);
}
inline std::string hexstr(INT16 val, UINT32 width = 0) {
  return StringHex((INT32)val, width);
}

inline std::string hexstr(UINT32 val, UINT32 width = 0) {
  return StringHex((UINT32)val, width);
}
inline std::string hexstr(UINT16 val, UINT32 width = 0) {
  return StringHex((UINT32)val, width);
}

inline std::string fltstr(FLT64 val, UINT32 prec = 0, UINT32 width = 0) {
  return StringFlt(val, prec, width);
}

inline std::string ptrstr(const VOID *val) { return StringHex(PTRINT(val)); }
extern std::string ljstr(const std::string &s, UINT32 width,
                         CHAR padding = ' ');

// ========================================================================

extern std::string Reformat(const std::string &s, const std::string &prefix,
                            UINT32 min_line, UINT32 max_line);

extern UINT32 Tokenize(const std::string &line, std::string *array, UINT32 n);

extern BOOL CaseCompare(const std::string &s1, const std::string &s2);

// ========================================================================
extern const std::string Line1;
extern const std::string Line2;
extern const std::string Line3;
extern const std::string Line4;

// ========================================================================
extern VOID RegisterImageName(const std::string &name);

extern VOID InstallSignalHandlers();

extern VOID DisableMessage(UINT32 prefix);

extern VOID RegisterNewTrace(VOID (*foo)());
extern VOID RegisterNewTerminate(VOID (*foo)());
extern VOID RegisterNewResourceInfo(std::string (*foo)());

extern std::string DefaultResinfo();

extern VOID DefaultTrace();

extern VOID DefaultTerminmate();

// ========================================================================
#define DO_RESINFO 1
#define DO_TERMINATE 1
#define DO_TRACE 1

extern VOID Message(const std::string &prefix, const std::string &message,
                    BOOL term, BOOL trace, BOOL resinfo);

#define QMESSAGE(m, n, term, trace, resinfo) Message(n, m, term, trace, resinfo)
#define QMESSAGE_LIMIT(m, n, max, term, trace, resinfo)                        \
  do {                                                                         \
    static int qqq = 0;                                                        \
    if (qqq++ >= max)                                                          \
      break;                                                                   \
    Message(n, m, term, trace, resinfo);                                       \
  } while (0)

#define ASSERTQ(message) QMESSAGE(message, "A: ", DO_TERMINATE, DO_TRACE, FALSE)

#if !defined(__GNUC__)
#define __FUNCTION__ ""
#endif

#define ASSERT(condition, message)                                             \
  do {                                                                         \
    if (!(condition))                                                          \
      ASSERTQ(std::string(__FILE__) + ":" + __FUNCTION__ + ":" +               \
              decstr(__LINE__) + ": " + message);                              \
  } while (0)

#define ASSERTX(condition)                                                     \
  ASSERT(condition, "assertion failed: " #condition "\n")
#define ASSERTZ(message) ASSERT(0, message)

// below we have be trying to make each prefix unique

#define Error(message) QMESSAGE(message, "E: ", DO_TERMINATE, FALSE, FALSE)

#define Phase(message) QMESSAGE(message, "P: ", FALSE, FALSE, DO_RESINFO)

#define Warning(message) QMESSAGE(message, "W: ", FALSE, FALSE, FALSE)
#define WarningLimit(message, n)                                               \
  QMESSAGE_LIMIT(message, "W: ", n, FALSE, FALSE, FALSE)

#define Info(message) QMESSAGE(message, "I: ", FALSE, FALSE, FALSE)
#define InfoLimit(message, n)                                                  \
  QMESSAGE_LIMIT(message, "I: ", n, FALSE, FALSE, FALSE)

#define Debug(message) QMESSAGE(message, "D: ", FALSE, FALSE, FALSE)

#define Stats(message) QMESSAGE(message, "S: ", FALSE, FALSE, FALSE)

#endif
