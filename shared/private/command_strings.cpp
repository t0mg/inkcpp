/**
 * EENK — command_strings.cpp
 *
 * Provides the ink::CommandStrings symbol required by inkcpp runtime.
 * This is a copy of the definition from inkcpp_compiler/command.cpp,
 * extracted to avoid including json.hpp which causes GCC 15 namespace issues.
 *
 * IMPORTANT: Must stay in sync with shared/private/command.h Command enum
 * and inkcpp_compiler/command.cpp.
 */
#include "command.h"  // shared/private/command.h — no json.hpp

namespace ink
{
// Exact copy of CommandStrings from inkcpp_compiler/command.cpp
const char* CommandStrings[]
    = {"inkcpp_STR",
       "inkcpp_INT",
       "inkcpp_BOOL",
       "inkcpp_FLOAT",
       "inkcpp_VALUE_POINTER",
       "inkcpp_DIVERT_VAL",
       "inkcpp_LIST",
       "\n",
       "<>",
       "void",
       "inkcpp_TAG",
       "inkcpp_DIVERT",
       "inkcpp_DIVERT_TO_VARIABLE",
       "inkcpp_TUNNEL",
       "inkcpp_FUNCTION",
       "done",
       "end",
       "->->",
       "~ret",

       "inkcpp_DEFINE_TEMP",
       "inkcpp_SET_VARIABLE",

       "ev",
       "/ev",
       "out",
       "pop",
       "du",
       "inkcpp_PUSH_VARIABLE_VALUE",
       "visit",
       "turn",
       "inkcpp_READ_COUNT",
       "seq",
       "srnd",

       "str",
       "/str",
       "#",
       "/#",

       "inkcpp_CHOICE",
       "thread",

       "range",

       "+",
       "-",
       "/",
       "*",
       "%",
       "rnd",
       "==",
       ">",
       "<",
       ">=",
       "<=",
       "!=",
       "&&",
       "||",
       "MIN",
       "MAX",
       "?",
       "!?",
       "L^",
       "listInt",

       "!",
       "~",
       "_",
       "LIST_COUNT",
       "LIST_MIN",
       "LIST_MAX",
       "readc",
       "turns",
       "lrnd",
       "FLOOR",
       "CEILING",
       "INT",
       "LIST_ALL",
       "LIST_INVERT",
       "LIST_VALUE",
       "choiceCnt",

       "START_CONTAINER",
       "END_CONTAINER",

       "CALL_EXTERNAL"};
} // namespace ink
