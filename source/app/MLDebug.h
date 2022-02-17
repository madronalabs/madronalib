//
//  MLDebug.h
//  madronalib
//
//

#pragma once

#include <iostream>

namespace ml {

// MLTEST
	// stolen from IPlug2 - thanks to Oli Larkin
	// TEMP

#ifdef NDEBUG
  #define DBGMSG(...)
#else
    #ifdef WIN32
    
    #include "windows.h"
    #undef min
    #undef max

    static void DBGMSG(const char *format, ...)
    {
      char buf[512];
      char *p = buf;
      va_list args;
      int     n;

      va_start(args, format);
      n = _vsnprintf(p, sizeof buf - 3, format, args); // buf-3 is room for CR/LF/NUL
      va_end(args);

      p += (n < 0) ? sizeof buf - 3 : n;

      while (p > buf  &&  isspace(p[-1]))
        *--p = '\0';

      *p++ = '\r';
      *p++ = '\n';
      *p = '\0';

      OutputDebugString(buf);
    }
  #else
    #define DBGMSG(...) printf(__VA_ARGS__)
  #endif
#endif
	
} 

