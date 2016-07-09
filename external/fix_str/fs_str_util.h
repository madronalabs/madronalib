
/**
  Copyright (c) 2005, Roland Pibinger
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  - Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  - Neither the name of the copyright holders nor the names of contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 *
 * @email rpbg123@yahoo.com
 *
 */


#ifndef FS_STR_UTIL_H
#define FS_STR_UTIL_H

#include <stdlib.h>
#include <ctype.h>  // isspace
#include <wctype.h> // iswspace 
#include <wchar.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>


#if defined (_WIN32) && defined (_MSC_VER)
#  define snprintf   _snprintf
#  define snwprintf  _snwprintf
#endif // WIN32


/**
 *   This file mainly contains C++ wrapper functions for Ansi-C string functions.
 *   Uniform usage for char and wchar_t is provided by function overloading
 *   (not macros).
 */

// ------------------------------------------------------------------------
// ---- char functions ----
// ------------------------------------------------------------------------

inline char fs_eos (const char*) throw() { return '\0'; }

inline const char* fs_peos (const char*) throw() { return ""; }

inline size_t fs_strlen (const char* s) throw() {
   assert (s);
   size_t len = 0;
   if (*s != '\0') {
      len = ::strlen (s);
   }
   return len;
}

inline char* fs_strncpy (char* dest, const char* src, size_t n) throw() {
   assert (dest);
   assert (src);
   return ::strncpy (dest, src, n);
}

inline const char* fs_strchr (const char* s, char c) throw() {
   assert (s);
   return ::strchr (s, c);
}

inline const char* fs_strrchr (const char* s, char c) throw() {
   assert (s);
   return ::strrchr (s, c);
}

inline const char* fs_strstr (const char* s, const char* sub) throw() {
   assert (s);
   assert (sub);
#ifdef __BORLANDC__
   if (*s == '\0' && *sub == '\0') {
      return 0;
   }
#endif
   return ::strstr (s, sub); 
}

inline int fs_strcmp (const char* s1, const char* s2) throw() {
   assert (s1);
   assert (s2);
   return ::strcmp (s1, s2);
}

inline int fs_strncmp (const char* s1, const char* s2, size_t n) throw() {
   assert (s1);
   assert (s2);
   return ::strncmp (s1, s2, n);
}

inline int fs_isspace (char c ) throw() {
   return isspace (c);
}

inline int fs_snprintf (char* buffer, size_t n, const char* formatString, int i) throw() {
   assert (buffer);
   assert (formatString);
   return ::snprintf (buffer, n, formatString, i);
}

inline int fs_snprintf (char* buffer, size_t n, int i) throw() {
   return fs_snprintf (buffer, n, "%d", i); 
}

inline int fs_snprintf (char* buffer, size_t n, const char* formatString, double d) throw() {
   assert (buffer);
   assert (formatString);
   return ::snprintf (buffer, n, formatString, d);
}

inline int fs_snprintf (char* buffer, size_t n, double d) throw() {
   return fs_snprintf (buffer, n, "%f", d);
}

inline long fs_strtol (const char* s, char** endptr, int base) throw() {
   assert (s);
   return ::strtol (s, endptr, base);
}

inline double fs_strtod (const char* s, char **endptr) throw() {
   assert (s);
   return ::strtod (s, endptr);
}


// ------------------------------------------------------------------------
// ---- wchar_t functions in string.h or wchar.h ----
// ------------------------------------------------------------------------

inline wchar_t fs_eos (const wchar_t*) throw() { return L'\0'; }

inline const wchar_t* fs_peos (const wchar_t*) throw() { return L""; }

inline size_t fs_strlen (const wchar_t* s) throw() {
   assert (s);
   size_t len = 0;
   if (*s != L'\0') {
      len = ::wcslen (s);  
   }
   return len;
}

inline wchar_t* fs_strncpy (wchar_t* dest, const wchar_t* src, size_t n) throw() {
   assert (dest);
   assert (src);
   return ::wcsncpy (dest, src, n);
}

inline const wchar_t* fs_strchr (const wchar_t* s, wchar_t c) throw() {
   assert (s);
   return ::wcschr (s, c);
}

inline const wchar_t* fs_strrchr (const wchar_t* s, wchar_t c) throw() {
   assert (s);
   return ::wcsrchr (s, c);
}

inline const wchar_t* fs_strstr (const wchar_t* s, const wchar_t* sub) throw() {
   assert (s);
   assert (sub);
#ifdef __BORLANDC__
   if (*s == L'\0' && *sub == L'\0') {
      return 0;
   }
#endif
   return ::wcsstr (s, sub);
}

inline int fs_strcmp (const wchar_t* s1, const wchar_t* s2) throw() {
   assert (s1);
   assert (s2);
   return ::wcscmp (s1, s2);
}

inline int fs_strncmp (const wchar_t* s1, const wchar_t* s2, size_t n) throw() {
   assert (s1);
   assert (s2);
   return ::wcsncmp (s1, s2, n);
}

inline int fs_isspace (wchar_t c ) throw() {
   using namespace std; 
   return iswspace (c);
}

inline int fs_snprintf (wchar_t *buffer, size_t n, const wchar_t* formatString, int i) throw() {
   assert (buffer);
   assert (formatString);
   return ::snwprintf (buffer, n, formatString, i);
}

inline int fs_snprintf (wchar_t* buffer, size_t n, int i) throw() {
   return fs_snprintf (buffer, n, L"%d", i); 
}

inline int fs_snprintf (wchar_t* buffer, size_t n, const wchar_t* formatString, double d) throw() {
   assert (buffer);
   assert (formatString);
   return ::snwprintf (buffer, n, formatString, d);
}

inline int fs_snprintf (wchar_t* buffer, size_t n, double d) throw() {
   return fs_snprintf (buffer, n, L"%f", d);
}

inline long fs_strtol (const wchar_t* s, wchar_t** endptr, int base) throw() {
   assert (s);
   return ::wcstol (s, endptr, base);
}

inline double fs_strtod (const wchar_t* s, wchar_t **endptr) throw() {
   assert (s);
   return ::wcstod (s, endptr);
}


// ------------------------------------------------------------------------
// ---- string hash functions ----
// ------------------------------------------------------------------------
   /**
    *  general Java-like hash function: "reasonably good" but not "state-of-the-art"
    *  @see: Bloch, J., "Effective Java", p.39f
    */
   template <typename CharT>
   inline
   unsigned long fs_j_hash (const CharT* s, size_t len) throw() {
      assert (s);
      unsigned long result = 17;
      for (size_t i = 0; i < len; ++i) {
         result = 37 * result + s[i];
      }
      return result;
   }


   typedef unsigned long Fnv32_t;

  /**
   *  Fowler/Noll/Vo hash (FNV hash), negligibly modified;
   *  for details see http://www.isthe.com/chongo/tech/comp/fnv/index.html 
   */ 

  //<pre> Original Copyright Notice for function fnv_32_buf():
  /***
   *
   * Please do not copyright this code.  This code is in the public domain.
   *
   * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
   * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
   * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
   * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
   * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
   * PERFORMANCE OF THIS SOFTWARE.
   *
   * By:
   *  chongo <Landon Curt Noll> /\oo/\
   *      http://www.isthe.com/chongo/
   *
   * Share and Enjoy!   :-)
   */
   //</pre>
   inline
   Fnv32_t fnv_32_buf (void *buf, size_t len, Fnv32_t hval = ((Fnv32_t)0x811c9dc5)) {
      unsigned char *bp = (unsigned char *)buf; /* start of buffer */
      unsigned char *be = bp + len;    /* beyond end of buffer */
      assert (buf);

      /* FNV-1 hash each octet in the buffer */
      while (bp < be) {
      /* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
         hval *= (Fnv32_t)0x01000193);
#else
         hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif
         /* xor the bottom with the current octet */
         hval ^= (Fnv32_t)*bp++;
      }
      /* return our new hash value */
      return hval;
   }



#endif

