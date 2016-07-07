
#ifndef FIX_STR_H_INCLUDED
#define FIX_STR_H_INCLUDED

// 	The MIT License

/**
@file
<pre>
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
</pre>
 *
 * email: rpbg123@yahoo.com
 *
 */


/**
@mainpage fix_str

Assignable but otherwise immutable string classes.
<p>
<p>
&nbsp;&nbsp; =&gt; for description see article
    <a href="../fix_str.html">here</a> and at
    <a href="http://www.codeproject.com/" target="_blank">CodeProject</a>

<p>
<!--
<a href="ptr_vector_demo.cpp.html"> <b>Example (Demo)</b> </a>
<p>
-->
email: <a href="mailto:rpbg123@yahoo.com">rpbg123@yahoo.com</a>
<p>

*/


#include <limits.h> // ULONG_MAX
#include <algorithm> // find_end
#include <iterator>

#if defined(_WIN32)
#  include <windows.h>
#endif

#include "fs_str_util.h"

// assert-like macros that can optionally be turned into 'soft failing',
// currently not activated
#if 0
#  define FS_ASSERT_RETURN(COND, RET) do {if(COND){;}else{return (RET);}} while(0)
#  define FS_ASSERT_DO(COND, DOIT)    do {if(COND){;}else{(DOIT);}} while(0)
#else
#  define FS_ASSERT_RETURN(COND, RET) assert(COND)
#  define FS_ASSERT_DO(COND, DOIT)    assert(COND)
#endif


#if defined (_MSC_VER)
#  pragma warning (push)
#  pragma warning (disable : 4290) // warning C4290: C++ exception specification ignored
#endif


#if defined (_MSC_VER) && _MSC_VER >= 1300  // >= MSVC++ 7.0
namespace std {
// Microsoft .NET 2003 C++ compiler may need this explicit specializations
template <>
struct iterator_traits<const char*> {
   typedef char         value_type;
   typedef ptrdiff_t    difference_type;
   typedef const char*  pointer;
   typedef const char&  reference;
   typedef random_access_iterator_tag iterator_category;
};

template <>
struct iterator_traits<const wchar_t*> {
   typedef wchar_t         value_type;
   typedef ptrdiff_t       difference_type;
   typedef const wchar_t*  pointer;
   typedef const wchar_t&  reference;
   typedef random_access_iterator_tag iterator_category;
};

} // std
#endif



// #include <fstream>
// extern std::ofstream dbgout;




 /**
  * @class fix_str_as
  * assignable but otherwise immutable string class
  * - value type: char
  * - suitable for multi-threaded environments: NO
  * - safe for concurrent writes to the same fix_str_as object: NO
  * 
  * example: 
  * <pre>
  * fix_str_as fs1 (_T("Hello, world!"));
  * size_t pos = fs1.find (_T("ello"));
  * fix_str_as fs2 = fix_str_as::sub_str (fs1, 0, 4);
  * </pre>
  */
class fix_str_as {
public:
   typedef char value_type;
   typedef value_type* pointer;
   typedef value_type& reference;
   typedef const value_type* const_pointer;
   typedef const value_type& const_reference;
   typedef size_t    size_type;
   typedef ptrdiff_t difference_type;

   enum { npos = size_type (-1) };

public:

   /** default constructor;
    *  create fix_str_as object with length() == 0; this function does not allocate
    *  dynamic memory (=> exception specification: throw())
    */
   fix_str_as() throw() {
      static fix_str_imp empty_imp = {1, 0, ULONG_MAX, eos()};
      imp = &empty_imp;
      on_create(); // necessary here because construct() is not called! 
      increment();
   }

   /** copy constructor; this function does not allocate
    *  dynamic memory (=> exception specification: throw())
    */
   fix_str_as (const fix_str_as& other) throw() {
      imp = other.imp;
      increment();
   }

   /** constructor */
   fix_str_as (const fix_str_as& f1, const fix_str_as& f2) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length());
   }

   /** constructor */
   fix_str_as (const fix_str_as& f1, const fix_str_as& f2, const fix_str_as& f3) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length());
   }
   
   /** constructor */
   fix_str_as (const fix_str_as& f1, const fix_str_as& f2, const fix_str_as& f3, const fix_str_as& f4) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length());
   }

   /** constructor */
   fix_str_as (const fix_str_as& f1, const fix_str_as& f2, const fix_str_as& f3, const fix_str_as& f4, const fix_str_as& f5) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length());
   }

   /** constructor */
   fix_str_as (const fix_str_as& f1, const fix_str_as& f2, const fix_str_as& f3, const fix_str_as& f4, const fix_str_as& f5, const fix_str_as& f6) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length(), f6.c_str(), f6.length());
   }

   /** constructor */
   fix_str_as (const fix_str_as& f1, const fix_str_as& f2, const fix_str_as& f3, const fix_str_as& f4, const fix_str_as& f5, const fix_str_as& f6, const fix_str_as& f7) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length(), f6.c_str(), f6.length(), f7.c_str(), f7.length());
   }

   /** constructor */
   fix_str_as (const fix_str_as& f1, const fix_str_as& f2, const fix_str_as& f3, const fix_str_as& f4, const fix_str_as& f5, const fix_str_as& f6, const fix_str_as& f7, const fix_str_as& f8) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length(), f6.c_str(), f6.length(), f7.c_str(), f7.length(), f8.c_str(), f8.length());
   }
   
   /** constructor */
   fix_str_as (const_pointer s) throw (std::bad_alloc) {
      FS_ASSERT_DO(s, s=peos());
      construct (s, fs_strlen (s));
   }

   /** constructor for 2 - 8 arguments
    * @param s_:   pointer to an array of characters terminated with '\0'
    */
  fix_str_as (const_pointer s1, const_pointer s2,
           const_pointer s3 = peos(), const_pointer s4 = peos(), const_pointer s5 = peos(),
           const_pointer s6 = peos(), const_pointer s7 = peos(), const_pointer s8 = peos()) throw (std::bad_alloc) {
     FS_ASSERT_DO(s1, s1=peos()); FS_ASSERT_DO(s2, s2=peos()); FS_ASSERT_DO(s3, s3=peos()); FS_ASSERT_DO(s4, s4=peos());
     FS_ASSERT_DO(s5, s5=peos()); FS_ASSERT_DO(s6, s6=peos()); FS_ASSERT_DO(s7, s7=peos()); FS_ASSERT_DO(s8, s8=peos());
     construct (s1, fs_strlen (s1), s2, fs_strlen (s2), s3, fs_strlen (s3), s4, fs_strlen (s4),
                s5, fs_strlen (s5), s6, fs_strlen (s6), s7, fs_strlen (s7), s8, fs_strlen (s8));
   }

   /** constructor
    * @param s:    pointer to an array of characters
    * @param len:  number of characters
    */
   fix_str_as (const_pointer s, size_type len) throw (std::bad_alloc) {
      FS_ASSERT_DO(s, s=peos());
      construct (s, len);
   }

   /** constructor for 2 - 8 arguments
    * @param s_:    pointer to an array of characters
    * @param len_:  number of characters
    */
   fix_str_as (const_pointer s1, size_type len1, const_pointer s2, size_type len2,
            const_pointer s3 = peos(), size_type len3 = 0, const_pointer s4 = peos(), size_type len4 = 0,
            const_pointer s5 = peos(), size_type len5 = 0, const_pointer s6 = peos(), size_type len6 = 0,
            const_pointer s7 = peos(), size_type len7 = 0, const_pointer s8 = peos(), size_type len8 = 0 ) throw (std::bad_alloc) {
     FS_ASSERT_DO(s1, s1=peos()); FS_ASSERT_DO(s2, s2=peos()); FS_ASSERT_DO(s3, s3=peos()); FS_ASSERT_DO(s4, s4=peos());
     FS_ASSERT_DO(s5, s5=peos()); FS_ASSERT_DO(s6, s6=peos()); FS_ASSERT_DO(s7, s7=peos()); FS_ASSERT_DO(s8, s8=peos());
     construct (s1, len1, s2, len2, s3, len3, s4, len4, s5, len5, s6, len6, s7, len7, s8, len8);
   }
   
   /** constructor
    * @param c: a character
    * @param n: number of repetitions of c
    */
  fix_str_as (value_type c, size_type n=1) throw (std::bad_alloc) {
     construct (c, n);
  }

   /** destructor */
   ~fix_str_as() throw() {
      dispose();
   }

   /** assignment operator; this function does not allocate
    *  dynamic memory (=> exception specification: throw())
    */
   fix_str_as& operator= (const fix_str_as& other) throw() {
      if (this != &other) {
         dispose(); 
         imp = other.imp;
         increment();
      }
      return *this;
   }

   /** STL random access reverse iterator type*/
#if defined (_MSC_VER) && _MSC_VER <= 1200  // MSVC++ 6.0
	typedef std::reverse_iterator<const_pointer, value_type, const value_type&, const value_type*, difference_type> const_reverse_iterator;
#else
   typedef std::reverse_iterator<const_pointer> const_reverse_iterator;
#endif
   /** STL const random access reverse iterator type*/
   typedef const_reverse_iterator reverse_iterator;
   /** STL random access iterator type*/
   typedef const_pointer iterator;
   /** STL const random access iterator type*/
   typedef const_pointer const_iterator;

   /** @return STL iterator to the first character in the object. */
   iterator begin() const throw() { return c_str(); }

   /** @return STL iterator to one past the end of the sequence of characters */
   iterator end()   const throw() { return c_str() + length(); }

   /** @return STL reverse iterator to the first character in the object
    *          (referring to the reversed sequence of characters).
    */
   reverse_iterator rbegin() const throw() { return reverse_iterator (end()); }

   /** @return STL reverse iterator to one past the end
    *          (referring to the reversed sequence of characters).
    */
   reverse_iterator rend()   const throw() { return reverse_iterator (begin()); }

   /** @return first character; length() must be > 0 */
   value_type front() const throw() { FS_ASSERT_RETURN(length(), eos()); return * begin(); }

   /** @return last character; length() must be > 0 */
   value_type back () const throw() { FS_ASSERT_RETURN(length(), eos()); return * (end() - 1); }

   /** @return const pointer to the internal array of characters with length length() */
   const_pointer c_str() const throw() {
      return imp->data;
   }

   /** @return number of characters, not bytes */
   size_type length() const throw() {
      return imp->len;
   }

   value_type operator[] (size_type pos) const throw() {
      FS_ASSERT_RETURN((pos < length()), eos());
      return * (c_str() + pos);
   }

   /** @return hash code */
   unsigned long hash_code() const throw() {
      if (imp->hashCode == ULONG_MAX) {
         imp->hashCode = fnv_32_buf ((void*) c_str(), length() * sizeof (value_type));
         if (imp->hashCode == ULONG_MAX) {
            imp->hashCode = 0; // ULONG_MAX is a 'reserved' value
         }
      }
      return imp->hashCode;
   }

   /** @return position of character c or fix_str_as::npos if c is not found
    *  @param  c:      character to be found
    *  @param  offset: position where search starts
    */
   size_type find (value_type c, size_type offset = 0) const throw() {
      size_type ret = fix_str_as::npos;
      if (offset < length()) {
         const_pointer p = fs_strchr (c_str() + offset, c);
         if (p) {
            ret = p - c_str();
         }
      }
      return ret;
   }

   /** @return position of str or fix_str_as::npos if str is not found
    *  @param  str:    fix_str_as to be found
    *  @param  offset: position where search starts
    */
   size_type find (const fix_str_as& str, size_type offset = 0) const throw() {
      return find (str.c_str(), offset);
   }

   /** @return position of str or fix_str_as::npos if str is not found
    *  @param  s:      character string be found
    *  @param  offset: position where search starts
    */
   size_type find (const_pointer s, size_type offset = 0) const throw() {
      FS_ASSERT_RETURN(s, fix_str_as::npos);
      size_type ret = fix_str_as::npos;
      if (offset < length()) {
         const_pointer p = fs_strstr (c_str() + offset, s);
         if (p) {
            ret = p - c_str();
         }
      }
      return ret;
   }

   /** @return position of last occurrence of c or fix_str_as::npos if c is not found
    *  @param  c:      character to be found
    *  @param  offset: position (from the beginning) where search starts
    */
   size_type rfind (value_type c, size_type offset = 0) const throw() {
      size_type ret = fix_str_as::npos;
      if (offset < length()) {
         const_pointer p = fs_strrchr (c_str() + offset, c);
         if (p) {
           ret = p - c_str();
         }
      }
      return ret;
   }

   /** @return position of last occurrence of str or fix_str_as::npos if str is not found
    *  @param  str:    fix_str_as to be found
    *  @param  offset: position (from the beginning) where search starts
    */
   size_type rfind (const fix_str_as& str, size_type offset = 0) const throw() {
      size_type ret = fix_str_as::npos;
      if (offset < length()) {
         iterator result = std::find_end (begin() + offset, end(), str.begin(), str.end());
         if (result != end()) {
            ret = result - begin();
         }
         if (str.length() == 0) {  // special case empty search string
            ret = offset;
         }
      }
      return ret;
   }

   /** @return position of last occurrence of s or fix_str_as::npos if str is not found
    *  @param  s:      character string to be found
    *  @param  offset: position (from the beginning) where search starts
    */
   size_type rfind (const_pointer s, size_type offset = 0) const throw() {
      FS_ASSERT_RETURN(s, fix_str_as::npos);
      size_type ret = fix_str_as::npos;
      if (offset < length()) {
         size_type lenStr = fs_strlen (s);
         iterator result = std::find_end (begin() + offset, end(), s, s + lenStr);
         if (result != end()) {
            ret = result - begin();
         }
         if (lenStr == 0) {  // special case empty search string
            ret = offset;
         }
      }
      return ret;
   }
   
   friend inline
   bool operator== (const fix_str_as& left, const fix_str_as& right) throw() {
      bool eq = left.imp == right.imp;
      if (! eq) {
         eq =   left.length()    == right.length()
             && *(left.c_str())  == *(right.c_str())
             && left.hash_code() == right.hash_code()
             && fs_strcmp (left.c_str(), right.c_str()) == 0;
      }
      return eq;
   }
   friend inline 
   bool operator== (const fix_str_as& left, const_pointer right) throw() {
      FS_ASSERT_RETURN(right, false);
      return fs_strcmp (left.c_str(), right) == 0;
   }
   friend inline
   bool operator== (const_pointer left, const fix_str_as& right) throw() {
      FS_ASSERT_RETURN(left, false);
      return fs_strcmp (left, right.c_str()) == 0;
   }

   friend inline
   bool operator< (const fix_str_as& left, const fix_str_as& right) throw() {
      return fs_strcmp (left.c_str(), right.c_str()) < 0;
   }
   friend inline 
   bool operator< (const fix_str_as& left, const_pointer right) throw() {
      FS_ASSERT_DO(right, right = peos());
      return fs_strcmp (left.c_str(), right) < 0;
   }
   friend inline 
   bool operator< (const_pointer left, const fix_str_as& right) throw() {
      FS_ASSERT_DO(left, left = peos());
      return fs_strcmp (left, right.c_str()) < 0;
   }

   friend inline bool operator!= (const fix_str_as& left, const fix_str_as& right) throw() { return ! (left == right); }
   friend inline bool operator!= (const fix_str_as& left, const_pointer right) throw()  { return ! (left == right); }
   friend inline bool operator!= (const_pointer left, const fix_str_as& right) throw()  { return ! (left == right); }

   friend inline bool operator>  (const fix_str_as& left, const fix_str_as& right) throw() { return right < left; }
   friend inline bool operator>  (const fix_str_as& left, const_pointer right) throw() { return right < left; }
   friend inline bool operator>  (const_pointer left, const fix_str_as& right) throw() { return right < left; }
   
   friend inline bool operator<= (const fix_str_as& left, const fix_str_as& right) throw() { return !(right < left); }
   friend inline bool operator<= (const fix_str_as& left, const_pointer right) throw() { return !(right < left); }
   friend inline bool operator<= (const_pointer left, const fix_str_as& right) throw() { return !(right < left); }

   friend inline bool operator>= (const fix_str_as& left, const fix_str_as& right) throw() { return !(left < right); }
   friend inline bool operator>= (const fix_str_as& left, const_pointer right) throw() { return !(left < right); }
   friend inline bool operator>= (const_pointer left, const fix_str_as& right) throw() { return !(left < right); }

// ---- static --------------------------------------------------------------------------
//  functions creating a new fix_str_as
// ---- static --------------------------------------------------------------------------
   
   /** @return new instance of fix_str_as for which is guaranteed:
       original.c_str() != returned.c_str()
   */
   static inline
   fix_str_as duplicate (const fix_str_as& original) throw (std::bad_alloc) {
      return fix_str_as (original.c_str(), original.length());
   }

   /** @return new instance representing a partial copy of the original
    *  @param  original: fix_str_as to be partially copied
    *  @param  offset:   position where the copying starts
    */
   static inline
   fix_str_as sub_str (const fix_str_as& original, size_type offset) throw (std::bad_alloc) {
      FS_ASSERT_RETURN((offset == 0 && original.length() == 0) || (offset < original.length()), fix_str_as());
      return fix_str_as (original.c_str() + offset, original.length() - offset);
   }

   /** @return new instance representing a partial copy of the original
    *  @param  original: fix_str_as to be partially copied
    *  @param  offset:   position where the copying starts
    *  @param  len:      number of copied characters starting from offset
    */
   static inline
   fix_str_as sub_str (const fix_str_as& original, size_type offset, size_type len) throw (std::bad_alloc) {
      FS_ASSERT_RETURN((offset == 0 && len == 0 && original.length() == 0) || (offset < original.length() && offset + len <= original.length()), fix_str_as_wm());      
      return fix_str_as (original.c_str() + offset, len);
   }

   /** @return new instance representing a copy of the original without leading
    *          white space; if the original contains no leading white space
    *          the original is returned
    *  @param  original: fix_str_as to be front trimmed
    */
   static inline
   fix_str_as trim_front (const fix_str_as& original) throw (std::bad_alloc) {
      fix_str_as ret = original;
      size_type start = find_first_non_whitespace (original.c_str(), original.length());
      
      if (start == 0) {
         ;  // original contains no leading whitespace
      } else if (start == fix_str_as::npos) {
         ret = fix_str_as(); // original contains whitespace only!
      } else {
         ret = fix_str_as (original.c_str() + start, original.length() - start);
      }
      
      return ret;
   }

   /** @return new instance representing a copy of the original without trailing
    *          white space; if the original contains no trailing white space
    *          the original is returned
    *  @param  original: fix_str_as to be back trimmed
    */
   static inline
   fix_str_as trim_back (const fix_str_as& original) throw (std::bad_alloc) {
      fix_str_as ret = original;
      size_type last = find_last_non_whitespace (original.c_str(), original.length());
      
      if (last == fix_str_as::npos) {
         ret = fix_str_as(); // original contains whitespace only!
      } else if (1 + last == original.length()) {
         ;  // original contains no trailing whitespace
      } else {
         ret = fix_str_as (original.c_str(), 1 + last);
      }
      
      return ret;
   }

   /** @return new instance representing a copy of the original without leading
    *          and trailing white space; if the original contains no leading
    *          and trailing white space the original is returned
    *  @param  original: fix_str_as to be back trimmed
    */
   static inline
   fix_str_as trim (const fix_str_as& original) throw (std::bad_alloc) {
      fix_str_as ret = original;
      size_type start = find_first_non_whitespace (original.c_str(), original.length()),
                last  = find_last_non_whitespace  (original.c_str(), original.length());
      
      if (start == fix_str_as::npos) {
         assert (last == fix_str_as::npos);
         ret = fix_str_as(); // original contains whitespace only!
      } else if (start == 0 && 1 + last == original.length()) {
         ;  // original contains no leading or trailing whitespace
      } else {
         ret = fix_str_as (original.c_str() + start, 1 + last - start);
      }
      
      return ret;
   }
   
   /** @return new instance representing a copy of the original padded to the
    *          specified length at the front;
    *          if the original needs no padding the original is returned
    *  @param  original: fix_str_as to be front padded
    *  @param  n:        minimal length of returned fix_str_as
    *  @param  c:        character for padding
    *  @param  cutLeadingWhiteSpace: true if leading whitespace should be cut
    *                                before padding with c, false otherwise
    */
   static inline
   fix_str_as pad_front (const fix_str_as& original, size_type n, value_type c, bool cutLeadingWhiteSpace = false) throw (std::bad_alloc) {
      fix_str_as   ret = original;
      size_type len = original.length(),
                offset = 0;
      
      if (cutLeadingWhiteSpace) {
         offset = find_first_non_whitespace (original.c_str(), original.length());
         len = (offset == fix_str_as::npos) ? 0 : len - offset;
      } 
      if (len < n) { // trim
         ret = pad (-1, original.c_str() + offset, len, n - len, c);
      } else if (len != original.length()) { // don't trim, cut leading whitespace only
         ret = sub_str (original, offset, len);
      }
      
      return ret;
   }

   /** @return new instance representing a copy of the original padded to the
    *          specified length at the back;
    *          if the original needs no padding the original is returned
    *  @param  original: fix_str_as to be back padded
    *  @param  n:        minimal length of returned fix_str_as
    *  @param  c:        character for padding
    *  @param  cutTrailingWhiteSpace: true if trailing whitespace should be cut
    *                                 before padding with c, false otherwise
    */
   static inline
   fix_str_as pad_back (const fix_str_as& original, size_type n, value_type c, bool cutTrailingWhiteSpace = false) throw (std::bad_alloc) {
      fix_str_as   ret = original;
      size_type len = original.length();
      
      if (cutTrailingWhiteSpace) {
         len = 1 + find_last_non_whitespace (original.c_str(), original.length());
      }
      if (len < n) { // trim
         ret = pad (1, original.c_str(), len, n - len, c);
      } else if (len != original.length()) { // don't trim, cut trailing whitespace only
         ret = sub_str (original, 0, len);
      }
      
      return ret;
   }

   /** @return new instance of the fix_str_as representation of a long value
    */
   static inline
   fix_str_as value_of (int i) throw (std::bad_alloc) {
      fix_str_as ret;
      value_type buf[32 * sizeof (value_type)] = {eos()};
		int n = fs_snprintf (buf, sizeof (buf), i);
      if (n > 0) {
         ret = fix_str_as (buf, n);
      }

      return ret;
   }

   /** @return new instance of the fix_str_as representation of a double value
    */
   static inline
   fix_str_as value_of (double d) throw (std::bad_alloc) {
      fix_str_as ret;
      value_type buf[128 * sizeof (value_type)] = {eos()};
		int n = fs_snprintf (buf, sizeof (buf), d);
      if (n > 0) {
         ret = fix_str_as (buf, n);
      }

      return ret;
   }

private:
   struct fix_str_imp {
      long       counter;
      size_type  len;
      unsigned long hashCode;
      value_type    data[sizeof(int)];
   };

   fix_str_imp* imp;

   void init (size_type len) throw (std::bad_alloc) {
      const size_type fixSize =   sizeof (fix_str_imp)
                                - sizeof (value_type[sizeof(int)]) // - fix_str_imp::data
                                + sizeof (value_type);   // + end of string indicator ('\0')
      size_type varSize = len * sizeof (value_type);     // actual size required by number of characters
      size_type rawSize = fixSize + varSize;

      imp = static_cast<fix_str_imp*> (malloc (rawSize));
      if (! imp) {
         error_out_of_memory();
      }
//      dbgout << (void*) imp << " malloc" << std::endl;

      imp->counter  = 1;
      imp->len      = len;
      imp->hashCode = ULONG_MAX;
      imp->data[0]  = eos();
   }

   bool copy_str (const_pointer s, size_type len, size_type offset = 0) throw() {
      fs_strncpy (imp->data + offset, s, len)[len] = eos();
      return true;
   }

   void construct (
            const_pointer s1 = peos(), size_type len1 = 0,
            const_pointer s2 = peos(), size_type len2 = 0,
            const_pointer s3 = peos(), size_type len3 = 0,
            const_pointer s4 = peos(), size_type len4 = 0,
            const_pointer s5 = peos(), size_type len5 = 0,
            const_pointer s6 = peos(), size_type len6 = 0,
            const_pointer s7 = peos(), size_type len7 = 0,
            const_pointer s8 = peos(), size_type len8 = 0
      ) throw (std::bad_alloc) {

      init   (len1 + len2 + len3 + len4 + len5 + len6 + len7 + len8);

      len1 && copy_str (s1, len1);
      len2 && copy_str (s2, len2, len1);
      len3 && copy_str (s3, len3, len1 + len2);
      len4 && copy_str (s4, len4, len1 + len2 + len3);
      len5 && copy_str (s5, len5, len1 + len2 + len3 + len4);
      len6 && copy_str (s6, len6, len1 + len2 + len3 + len4 + len5);
      len7 && copy_str (s7, len7, len1 + len2 + len3 + len4 + len5 + len6);
      len8 && copy_str (s8, len8, len1 + len2 + len3 + len4 + len5 + len6 + len7);

      on_create();
   }

   void construct (value_type c, size_type n) throw (std::bad_alloc) {
      init (n);
      for (size_type i = 0; i < n; ++i) {
         imp->data[i] = c;
      }
      imp->data[n] = eos();

      on_create();
   }


   void on_create() throw() {
      return;
   }

   void increment() throw() {
      atomic_increment();
   }

   void dispose() throw() {
     assert (*(c_str() + length()) == eos());
     if (atomic_decrement() == 0) {
         free (imp);
//          dbgout << (void*) imp << " free" << std::endl;
      }
   }

   void atomic_increment() throw() {
      ++imp->counter; 
   }

   long atomic_decrement() throw() {
      return --imp->counter;
   }


// ---- helper functions ----

   static inline
   size_type find_first_non_whitespace (const_pointer s, size_type length) throw() {
      assert (s);
      iterator first (s), last (s + length), current (first);

      for (; current != last && fs_isspace (*current);  ++current) {
         ;
      }

      size_type pos = current - first;
      if (current == last) {
         pos = fix_str_as::npos;
      }

      return pos;
   }

   static inline
   size_type find_last_non_whitespace (const_pointer s, size_type length) throw() {
      assert (s);
      reverse_iterator first (s + length), last (s), current (first);

      for (; current != last && fs_isspace (*current);  ++current) {
         ;
      }

      size_type pos = fix_str_as::npos;
      if (current != last) {
         pos = last - current - 1;
      }

      return pos;
   }

   struct array_deleter {
      array_deleter() throw() : arr(0) {}
      ~array_deleter() throw() { delete [] arr; }
      void set (value_type array[]) throw() { assert (arr == 0); arr = array; }
   private:
      pointer arr;      
      array_deleter (const array_deleter&);
      array_deleter& operator= (const array_deleter&);
   };

   static inline 
   fix_str_as pad (int where, const_pointer startpos, size_type length, size_type numpad, value_type c) throw (std::bad_alloc) {
      const size_type bufSiz = 128;
      value_type buf[bufSiz];
      pointer startpad = buf;
      array_deleter arrDel;
      if (! (numpad < bufSiz)) {
         startpad = new (std::nothrow) value_type[numpad + 1];
         if (! startpad) {
            error_out_of_memory();
         }
         arrDel.set (startpad);
      }
      size_type i = 0;
      for (; i < numpad; ++i) {
         startpad[i] = c;
      }
      startpad[i] = eos();
      
      fix_str_as ret; 
      if (where < 0) { // pad front
         ret = fix_str_as (startpad,  numpad, startpos, length);
      } else {         // pad back
         ret = fix_str_as (startpos, length, startpad,  numpad);
      }

      return ret;
   }

   static
   void error_out_of_memory() throw (std::bad_alloc) {
      throw std::bad_alloc();
   }

   static inline
   value_type eos() throw() {
      return fs_eos ((const_pointer) 0);
   }

   static inline
   const_pointer peos() throw() {
      return fs_peos ((const_pointer) 0);
   }

};


 /**
  * @class fix_str_am
  * assignable but otherwise immutable string class
  * - value type: char
  * - suitable for multi-threaded environments: YES
  * - safe for concurrent writes to the same fix_str_am object: NO
  * 
  * example: 
  * <pre>
  * fix_str_am fs1 (_T("Hello, world!"));
  * size_t pos = fs1.find (_T("ello"));
  * fix_str_am fs2 = fix_str_am::sub_str (fs1, 0, 4);
  * </pre>
  */
class fix_str_am {
public:
   typedef char value_type;
   typedef value_type* pointer;
   typedef value_type& reference;
   typedef const value_type* const_pointer;
   typedef const value_type& const_reference;
   typedef size_t    size_type;
   typedef ptrdiff_t difference_type;

   enum { npos = size_type (-1) };

public:

   /** default constructor;
    *  create fix_str_am object with length() == 0; this function does not allocate
    *  dynamic memory (=> exception specification: throw())
    */
   fix_str_am() throw() {
      static fix_str_imp empty_imp = {1, 0, ULONG_MAX, eos()};
      imp = &empty_imp;
      on_create(); // necessary here because construct() is not called! 
      increment();
   }

   /** copy constructor; this function does not allocate
    *  dynamic memory (=> exception specification: throw())
    */
   fix_str_am (const fix_str_am& other) throw() {
      imp = other.imp;
      increment();
   }

   /** constructor */
   fix_str_am (const fix_str_am& f1, const fix_str_am& f2) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length());
   }

   /** constructor */
   fix_str_am (const fix_str_am& f1, const fix_str_am& f2, const fix_str_am& f3) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length());
   }
   
   /** constructor */
   fix_str_am (const fix_str_am& f1, const fix_str_am& f2, const fix_str_am& f3, const fix_str_am& f4) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length());
   }

   /** constructor */
   fix_str_am (const fix_str_am& f1, const fix_str_am& f2, const fix_str_am& f3, const fix_str_am& f4, const fix_str_am& f5) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length());
   }

   /** constructor */
   fix_str_am (const fix_str_am& f1, const fix_str_am& f2, const fix_str_am& f3, const fix_str_am& f4, const fix_str_am& f5, const fix_str_am& f6) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length(), f6.c_str(), f6.length());
   }

   /** constructor */
   fix_str_am (const fix_str_am& f1, const fix_str_am& f2, const fix_str_am& f3, const fix_str_am& f4, const fix_str_am& f5, const fix_str_am& f6, const fix_str_am& f7) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length(), f6.c_str(), f6.length(), f7.c_str(), f7.length());
   }

   /** constructor */
   fix_str_am (const fix_str_am& f1, const fix_str_am& f2, const fix_str_am& f3, const fix_str_am& f4, const fix_str_am& f5, const fix_str_am& f6, const fix_str_am& f7, const fix_str_am& f8) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length(), f6.c_str(), f6.length(), f7.c_str(), f7.length(), f8.c_str(), f8.length());
   }
   
   /** constructor */
   fix_str_am (const_pointer s) throw (std::bad_alloc) {
      FS_ASSERT_DO(s, s=peos());
      construct (s, fs_strlen (s));
   }

   /** constructor for 2 - 8 arguments
    * @param s_:   pointer to an array of characters terminated with '\0'
    */
  fix_str_am (const_pointer s1, const_pointer s2,
           const_pointer s3 = peos(), const_pointer s4 = peos(), const_pointer s5 = peos(),
           const_pointer s6 = peos(), const_pointer s7 = peos(), const_pointer s8 = peos()) throw (std::bad_alloc) {
     FS_ASSERT_DO(s1, s1=peos()); FS_ASSERT_DO(s2, s2=peos()); FS_ASSERT_DO(s3, s3=peos()); FS_ASSERT_DO(s4, s4=peos());
     FS_ASSERT_DO(s5, s5=peos()); FS_ASSERT_DO(s6, s6=peos()); FS_ASSERT_DO(s7, s7=peos()); FS_ASSERT_DO(s8, s8=peos());
     construct (s1, fs_strlen (s1), s2, fs_strlen (s2), s3, fs_strlen (s3), s4, fs_strlen (s4),
                s5, fs_strlen (s5), s6, fs_strlen (s6), s7, fs_strlen (s7), s8, fs_strlen (s8));
   }

   /** constructor
    * @param s:    pointer to an array of characters
    * @param len:  number of characters
    */
   fix_str_am (const_pointer s, size_type len) throw (std::bad_alloc) {
      FS_ASSERT_DO(s, s=peos());
      construct (s, len);
   }

   /** constructor for 2 - 8 arguments
    * @param s_:    pointer to an array of characters
    * @param len_:  number of characters
    */
   fix_str_am (const_pointer s1, size_type len1, const_pointer s2, size_type len2,
            const_pointer s3 = peos(), size_type len3 = 0, const_pointer s4 = peos(), size_type len4 = 0,
            const_pointer s5 = peos(), size_type len5 = 0, const_pointer s6 = peos(), size_type len6 = 0,
            const_pointer s7 = peos(), size_type len7 = 0, const_pointer s8 = peos(), size_type len8 = 0 ) throw (std::bad_alloc) {
     FS_ASSERT_DO(s1, s1=peos()); FS_ASSERT_DO(s2, s2=peos()); FS_ASSERT_DO(s3, s3=peos()); FS_ASSERT_DO(s4, s4=peos());
     FS_ASSERT_DO(s5, s5=peos()); FS_ASSERT_DO(s6, s6=peos()); FS_ASSERT_DO(s7, s7=peos()); FS_ASSERT_DO(s8, s8=peos());
     construct (s1, len1, s2, len2, s3, len3, s4, len4, s5, len5, s6, len6, s7, len7, s8, len8);
   }
   
   /** constructor
    * @param c: a character
    * @param n: number of repetitions of c
    */
  fix_str_am (value_type c, size_type n=1) throw (std::bad_alloc) {
     construct (c, n);
  }

   /** destructor */
   ~fix_str_am() throw() {
      dispose();
   }

   /** assignment operator; this function does not allocate
    *  dynamic memory (=> exception specification: throw())
    */
   fix_str_am& operator= (const fix_str_am& other) throw() {
      if (this != &other) {
         dispose(); 
         imp = other.imp;
         increment();
      }
      return *this;
   }

   /** STL random access reverse iterator type*/
#if defined (_MSC_VER) && _MSC_VER <= 1200  // MSVC++ 6.0
	typedef std::reverse_iterator<const_pointer, value_type, const value_type&, const value_type*, difference_type> const_reverse_iterator;
#else
   typedef std::reverse_iterator<const_pointer> const_reverse_iterator;
#endif
   /** STL const random access reverse iterator type*/
   typedef const_reverse_iterator reverse_iterator;
   /** STL random access iterator type*/
   typedef const_pointer iterator;
   /** STL const random access iterator type*/
   typedef const_pointer const_iterator;

   /** @return STL iterator to the first character in the object. */
   iterator begin() const throw() { return c_str(); }

   /** @return STL iterator to one past the end of the sequence of characters */
   iterator end()   const throw() { return c_str() + length(); }

   /** @return STL reverse iterator to the first character in the object
    *          (referring to the reversed sequence of characters).
    */
   reverse_iterator rbegin() const throw() { return reverse_iterator (end()); }

   /** @return STL reverse iterator to one past the end
    *          (referring to the reversed sequence of characters).
    */
   reverse_iterator rend()   const throw() { return reverse_iterator (begin()); }

   /** @return first character; length() must be > 0 */
   value_type front() const throw() { FS_ASSERT_RETURN(length(), eos()); return * begin(); }

   /** @return last character; length() must be > 0 */
   value_type back () const throw() { FS_ASSERT_RETURN(length(), eos()); return * (end() - 1); }

   /** @return const pointer to the internal array of characters with length length() */
   const_pointer c_str() const throw() {
      return imp->data;
   }

   /** @return number of characters, not bytes */
   size_type length() const throw() {
      return imp->len;
   }

   value_type operator[] (size_type pos) const throw() {
      FS_ASSERT_RETURN((pos < length()), eos());
      return * (c_str() + pos);
   }

   /** @return hash code */
   unsigned long hash_code() const throw() {
      if (imp->hashCode == ULONG_MAX) {
         imp->hashCode = fnv_32_buf ((void*) c_str(), length() * sizeof (value_type));
         if (imp->hashCode == ULONG_MAX) {
            imp->hashCode = 0; // ULONG_MAX is a 'reserved' value
         }
      }
      return imp->hashCode;
   }

   /** @return position of character c or fix_str_am::npos if c is not found
    *  @param  c:      character to be found
    *  @param  offset: position where search starts
    */
   size_type find (value_type c, size_type offset = 0) const throw() {
      size_type ret = fix_str_am::npos;
      if (offset < length()) {
         const_pointer p = fs_strchr (c_str() + offset, c);
         if (p) {
            ret = p - c_str();
         }
      }
      return ret;
   }

   /** @return position of str or fix_str_am::npos if str is not found
    *  @param  str:    fix_str_am to be found
    *  @param  offset: position where search starts
    */
   size_type find (const fix_str_am& str, size_type offset = 0) const throw() {
      return find (str.c_str(), offset);
   }

   /** @return position of str or fix_str_am::npos if str is not found
    *  @param  s:      character string be found
    *  @param  offset: position where search starts
    */
   size_type find (const_pointer s, size_type offset = 0) const throw() {
      FS_ASSERT_RETURN(s, fix_str_am::npos);
      size_type ret = fix_str_am::npos;
      if (offset < length()) {
         const_pointer p = fs_strstr (c_str() + offset, s);
         if (p) {
            ret = p - c_str();
         }
      }
      return ret;
   }

   /** @return position of last occurrence of c or fix_str_am::npos if c is not found
    *  @param  c:      character to be found
    *  @param  offset: position (from the beginning) where search starts
    */
   size_type rfind (value_type c, size_type offset = 0) const throw() {
      size_type ret = fix_str_am::npos;
      if (offset < length()) {
         const_pointer p = fs_strrchr (c_str() + offset, c);
         if (p) {
           ret = p - c_str();
         }
      }
      return ret;
   }

   /** @return position of last occurrence of str or fix_str_am::npos if str is not found
    *  @param  str:    fix_str_am to be found
    *  @param  offset: position (from the beginning) where search starts
    */
   size_type rfind (const fix_str_am& str, size_type offset = 0) const throw() {
      size_type ret = fix_str_am::npos;
      if (offset < length()) {
         iterator result = std::find_end (begin() + offset, end(), str.begin(), str.end());
         if (result != end()) {
            ret = result - begin();
         }
         if (str.length() == 0) {  // special case empty search string
            ret = offset;
         }
      }
      return ret;
   }

   /** @return position of last occurrence of s or fix_str_am::npos if str is not found
    *  @param  s:      character string to be found
    *  @param  offset: position (from the beginning) where search starts
    */
   size_type rfind (const_pointer s, size_type offset = 0) const throw() {
      FS_ASSERT_RETURN(s, fix_str_am::npos);
      size_type ret = fix_str_am::npos;
      if (offset < length()) {
         size_type lenStr = fs_strlen (s);
         iterator result = std::find_end (begin() + offset, end(), s, s + lenStr);
         if (result != end()) {
            ret = result - begin();
         }
         if (lenStr == 0) {  // special case empty search string
            ret = offset;
         }
      }
      return ret;
   }
   
   friend inline
   bool operator== (const fix_str_am& left, const fix_str_am& right) throw() {
      bool eq = left.imp == right.imp;
      if (! eq) {
         eq =   left.length()    == right.length()
             && *(left.c_str())  == *(right.c_str())
             && left.hash_code() == right.hash_code()
             && fs_strcmp (left.c_str(), right.c_str()) == 0;
      }
      return eq;
   }
   friend inline 
   bool operator== (const fix_str_am& left, const_pointer right) throw() {
      FS_ASSERT_RETURN(right, false);
      return fs_strcmp (left.c_str(), right) == 0;
   }
   friend inline
   bool operator== (const_pointer left, const fix_str_am& right) throw() {
      FS_ASSERT_RETURN(left, false);
      return fs_strcmp (left, right.c_str()) == 0;
   }

   friend inline
   bool operator< (const fix_str_am& left, const fix_str_am& right) throw() {
      return fs_strcmp (left.c_str(), right.c_str()) < 0;
   }
   friend inline 
   bool operator< (const fix_str_am& left, const_pointer right) throw() {
      FS_ASSERT_DO(right, right = peos());
      return fs_strcmp (left.c_str(), right) < 0;
   }
   friend inline 
   bool operator< (const_pointer left, const fix_str_am& right) throw() {
      FS_ASSERT_DO(left, left = peos());
      return fs_strcmp (left, right.c_str()) < 0;
   }

   friend inline bool operator!= (const fix_str_am& left, const fix_str_am& right) throw() { return ! (left == right); }
   friend inline bool operator!= (const fix_str_am& left, const_pointer right) throw()  { return ! (left == right); }
   friend inline bool operator!= (const_pointer left, const fix_str_am& right) throw()  { return ! (left == right); }

   friend inline bool operator>  (const fix_str_am& left, const fix_str_am& right) throw() { return right < left; }
   friend inline bool operator>  (const fix_str_am& left, const_pointer right) throw() { return right < left; }
   friend inline bool operator>  (const_pointer left, const fix_str_am& right) throw() { return right < left; }
   
   friend inline bool operator<= (const fix_str_am& left, const fix_str_am& right) throw() { return !(right < left); }
   friend inline bool operator<= (const fix_str_am& left, const_pointer right) throw() { return !(right < left); }
   friend inline bool operator<= (const_pointer left, const fix_str_am& right) throw() { return !(right < left); }

   friend inline bool operator>= (const fix_str_am& left, const fix_str_am& right) throw() { return !(left < right); }
   friend inline bool operator>= (const fix_str_am& left, const_pointer right) throw() { return !(left < right); }
   friend inline bool operator>= (const_pointer left, const fix_str_am& right) throw() { return !(left < right); }

// ---- static --------------------------------------------------------------------------
//  functions creating a new fix_str_am
// ---- static --------------------------------------------------------------------------
   
   /** @return new instance of fix_str_am for which is guaranteed:
       original.c_str() != returned.c_str()
   */
   static inline
   fix_str_am duplicate (const fix_str_am& original) throw (std::bad_alloc) {
      return fix_str_am (original.c_str(), original.length());
   }

   /** @return new instance representing a partial copy of the original
    *  @param  original: fix_str_am to be partially copied
    *  @param  offset:   position where the copying starts
    */
   static inline
   fix_str_am sub_str (const fix_str_am& original, size_type offset) throw (std::bad_alloc) {
      FS_ASSERT_RETURN((offset == 0 && original.length() == 0) || (offset < original.length()), fix_str_am());
      return fix_str_am (original.c_str() + offset, original.length() - offset);
   }

   /** @return new instance representing a partial copy of the original
    *  @param  original: fix_str_am to be partially copied
    *  @param  offset:   position where the copying starts
    *  @param  len:      number of copied characters starting from offset
    */
   static inline
   fix_str_am sub_str (const fix_str_am& original, size_type offset, size_type len) throw (std::bad_alloc) {
      FS_ASSERT_RETURN((offset == 0 && len == 0 && original.length() == 0) || (offset < original.length() && offset + len <= original.length()), fix_str_am_wm());      
      return fix_str_am (original.c_str() + offset, len);
   }

   /** @return new instance representing a copy of the original without leading
    *          white space; if the original contains no leading white space
    *          the original is returned
    *  @param  original: fix_str_am to be front trimmed
    */
   static inline
   fix_str_am trim_front (const fix_str_am& original) throw (std::bad_alloc) {
      fix_str_am ret = original;
      size_type start = find_first_non_whitespace (original.c_str(), original.length());
      
      if (start == 0) {
         ;  // original contains no leading whitespace
      } else if (start == fix_str_am::npos) {
         ret = fix_str_am(); // original contains whitespace only!
      } else {
         ret = fix_str_am (original.c_str() + start, original.length() - start);
      }
      
      return ret;
   }

   /** @return new instance representing a copy of the original without trailing
    *          white space; if the original contains no trailing white space
    *          the original is returned
    *  @param  original: fix_str_am to be back trimmed
    */
   static inline
   fix_str_am trim_back (const fix_str_am& original) throw (std::bad_alloc) {
      fix_str_am ret = original;
      size_type last = find_last_non_whitespace (original.c_str(), original.length());
      
      if (last == fix_str_am::npos) {
         ret = fix_str_am(); // original contains whitespace only!
      } else if (1 + last == original.length()) {
         ;  // original contains no trailing whitespace
      } else {
         ret = fix_str_am (original.c_str(), 1 + last);
      }
      
      return ret;
   }

   /** @return new instance representing a copy of the original without leading
    *          and trailing white space; if the original contains no leading
    *          and trailing white space the original is returned
    *  @param  original: fix_str_am to be back trimmed
    */
   static inline
   fix_str_am trim (const fix_str_am& original) throw (std::bad_alloc) {
      fix_str_am ret = original;
      size_type start = find_first_non_whitespace (original.c_str(), original.length()),
                last  = find_last_non_whitespace  (original.c_str(), original.length());
      
      if (start == fix_str_am::npos) {
         assert (last == fix_str_am::npos);
         ret = fix_str_am(); // original contains whitespace only!
      } else if (start == 0 && 1 + last == original.length()) {
         ;  // original contains no leading or trailing whitespace
      } else {
         ret = fix_str_am (original.c_str() + start, 1 + last - start);
      }
      
      return ret;
   }
   
   /** @return new instance representing a copy of the original padded to the
    *          specified length at the front;
    *          if the original needs no padding the original is returned
    *  @param  original: fix_str_am to be front padded
    *  @param  n:        minimal length of returned fix_str_am
    *  @param  c:        character for padding
    *  @param  cutLeadingWhiteSpace: true if leading whitespace should be cut
    *                                before padding with c, false otherwise
    */
   static inline
   fix_str_am pad_front (const fix_str_am& original, size_type n, value_type c, bool cutLeadingWhiteSpace = false) throw (std::bad_alloc) {
      fix_str_am   ret = original;
      size_type len = original.length(),
                offset = 0;
      
      if (cutLeadingWhiteSpace) {
         offset = find_first_non_whitespace (original.c_str(), original.length());
         len = (offset == fix_str_am::npos) ? 0 : len - offset;
      } 
      if (len < n) { // trim
         ret = pad (-1, original.c_str() + offset, len, n - len, c);
      } else if (len != original.length()) { // don't trim, cut leading whitespace only
         ret = sub_str (original, offset, len);
      }
      
      return ret;
   }

   /** @return new instance representing a copy of the original padded to the
    *          specified length at the back;
    *          if the original needs no padding the original is returned
    *  @param  original: fix_str_am to be back padded
    *  @param  n:        minimal length of returned fix_str_am
    *  @param  c:        character for padding
    *  @param  cutTrailingWhiteSpace: true if trailing whitespace should be cut
    *                                 before padding with c, false otherwise
    */
   static inline
   fix_str_am pad_back (const fix_str_am& original, size_type n, value_type c, bool cutTrailingWhiteSpace = false) throw (std::bad_alloc) {
      fix_str_am   ret = original;
      size_type len = original.length();
      
      if (cutTrailingWhiteSpace) {
         len = 1 + find_last_non_whitespace (original.c_str(), original.length());
      }
      if (len < n) { // trim
         ret = pad (1, original.c_str(), len, n - len, c);
      } else if (len != original.length()) { // don't trim, cut trailing whitespace only
         ret = sub_str (original, 0, len);
      }
      
      return ret;
   }

   /** @return new instance of the fix_str_am representation of a long value
    */
   static inline
   fix_str_am value_of (int i) throw (std::bad_alloc) {
      fix_str_am ret;
      value_type buf[32 * sizeof (value_type)] = {eos()};
		int n = fs_snprintf (buf, sizeof (buf), i);
      if (n > 0) {
         ret = fix_str_am (buf, n);
      }

      return ret;
   }

   /** @return new instance of the fix_str_am representation of a double value
    */
   static inline
   fix_str_am value_of (double d) throw (std::bad_alloc) {
      fix_str_am ret;
      value_type buf[128 * sizeof (value_type)] = {eos()};
		int n = fs_snprintf (buf, sizeof (buf), d);
      if (n > 0) {
         ret = fix_str_am (buf, n);
      }

      return ret;
   }

private:
   struct fix_str_imp {
      long       counter;
      size_type  len;
      unsigned long hashCode;
      value_type    data[sizeof(int)];
   };

   fix_str_imp* imp;

   void init (size_type len) throw (std::bad_alloc) {
      const size_type fixSize =   sizeof (fix_str_imp)
                                - sizeof (value_type[sizeof(int)]) // - fix_str_imp::data
                                + sizeof (value_type);   // + end of string indicator ('\0')
      size_type varSize = len * sizeof (value_type);     // actual size required by number of characters
      size_type rawSize = fixSize + varSize;

      imp = static_cast<fix_str_imp*> (malloc (rawSize));
      if (! imp) {
         error_out_of_memory();
      }
//      dbgout << (void*) imp << " malloc" << std::endl;

      imp->counter  = 1;
      imp->len      = len;
      imp->hashCode = ULONG_MAX;
      imp->data[0]  = eos();
   }

   bool copy_str (const_pointer s, size_type len, size_type offset = 0) throw() {
      fs_strncpy (imp->data + offset, s, len)[len] = eos();
      return true;
   }

   void construct (
            const_pointer s1 = peos(), size_type len1 = 0,
            const_pointer s2 = peos(), size_type len2 = 0,
            const_pointer s3 = peos(), size_type len3 = 0,
            const_pointer s4 = peos(), size_type len4 = 0,
            const_pointer s5 = peos(), size_type len5 = 0,
            const_pointer s6 = peos(), size_type len6 = 0,
            const_pointer s7 = peos(), size_type len7 = 0,
            const_pointer s8 = peos(), size_type len8 = 0
      ) throw (std::bad_alloc) {

      init   (len1 + len2 + len3 + len4 + len5 + len6 + len7 + len8);

      len1 && copy_str (s1, len1);
      len2 && copy_str (s2, len2, len1);
      len3 && copy_str (s3, len3, len1 + len2);
      len4 && copy_str (s4, len4, len1 + len2 + len3);
      len5 && copy_str (s5, len5, len1 + len2 + len3 + len4);
      len6 && copy_str (s6, len6, len1 + len2 + len3 + len4 + len5);
      len7 && copy_str (s7, len7, len1 + len2 + len3 + len4 + len5 + len6);
      len8 && copy_str (s8, len8, len1 + len2 + len3 + len4 + len5 + len6 + len7);

      on_create();
   }

   void construct (value_type c, size_type n) throw (std::bad_alloc) {
      init (n);
      for (size_type i = 0; i < n; ++i) {
         imp->data[i] = c;
      }
      imp->data[n] = eos();

      on_create();
   }


   void on_create() throw() {
      hash_code(); // lazy initialization of hash value only in single threaded mode 
      return;
   }

   void increment() throw() {
      atomic_increment();
   }

   void dispose() throw() {
     assert (*(c_str() + length()) == eos());
     if (atomic_decrement() == 0) {
         free (imp);
//          dbgout << (void*) imp << " free" << std::endl;
      }
   }

#  ifdef _WIN32
   void atomic_increment() throw() {
      InterlockedIncrement (& (imp->counter)); 
   }

   long atomic_decrement() throw() {
      return InterlockedDecrement (& (imp->counter)); 
   }

#  else
#    error not yet implemented for Non-Win32 systems // Linux/Intel: see eg.  __atomic_add() in  bits/atomicity.h
#  endif      



// ---- helper functions ----

   static inline
   size_type find_first_non_whitespace (const_pointer s, size_type length) throw() {
      assert (s);
      iterator first (s), last (s + length), current (first);

      for (; current != last && fs_isspace (*current);  ++current) {
         ;
      }

      size_type pos = current - first;
      if (current == last) {
         pos = fix_str_am::npos;
      }

      return pos;
   }

   static inline
   size_type find_last_non_whitespace (const_pointer s, size_type length) throw() {
      assert (s);
      reverse_iterator first (s + length), last (s), current (first);

      for (; current != last && fs_isspace (*current);  ++current) {
         ;
      }

      size_type pos = fix_str_am::npos;
      if (current != last) {
         pos = last - current - 1;
      }

      return pos;
   }

   struct array_deleter {
      array_deleter() throw() : arr(0) {}
      ~array_deleter() throw() { delete [] arr; }
      void set (value_type array[]) throw() { assert (arr == 0); arr = array; }
   private:
      pointer arr;      
      array_deleter (const array_deleter&);
      array_deleter& operator= (const array_deleter&);
   };

   static inline 
   fix_str_am pad (int where, const_pointer startpos, size_type length, size_type numpad, value_type c) throw (std::bad_alloc) {
      const size_type bufSiz = 128;
      value_type buf[bufSiz];
      pointer startpad = buf;
      array_deleter arrDel;
      if (! (numpad < bufSiz)) {
         startpad = new (std::nothrow) value_type[numpad + 1];
         if (! startpad) {
            error_out_of_memory();
         }
         arrDel.set (startpad);
      }
      size_type i = 0;
      for (; i < numpad; ++i) {
         startpad[i] = c;
      }
      startpad[i] = eos();
      
      fix_str_am ret; 
      if (where < 0) { // pad front
         ret = fix_str_am (startpad,  numpad, startpos, length);
      } else {         // pad back
         ret = fix_str_am (startpos, length, startpad,  numpad);
      }

      return ret;
   }

   static
   void error_out_of_memory() throw (std::bad_alloc) {
      throw std::bad_alloc();
   }

   static inline
   value_type eos() throw() {
      return fs_eos ((const_pointer) 0);
   }

   static inline
   const_pointer peos() throw() {
      return fs_peos ((const_pointer) 0);
   }

};


 /**
  * @class fix_str_ws
  * assignable but otherwise immutable string class
  * - value type: wchar_t
  * - suitable for multi-threaded environments: NO
  * - safe for concurrent writes to the same fix_str_ws object: NO
  * 
  * example: 
  * <pre>
  * fix_str_ws fs1 (_T("Hello, world!"));
  * size_t pos = fs1.find (_T("ello"));
  * fix_str_ws fs2 = fix_str_ws::sub_str (fs1, 0, 4);
  * </pre>
  */
class fix_str_ws {
public:
   typedef wchar_t value_type;
   typedef value_type* pointer;
   typedef value_type& reference;
   typedef const value_type* const_pointer;
   typedef const value_type& const_reference;
   typedef size_t    size_type;
   typedef ptrdiff_t difference_type;

   enum { npos = size_type (-1) };

public:

   /** default constructor;
    *  create fix_str_ws object with length() == 0; this function does not allocate
    *  dynamic memory (=> exception specification: throw())
    */
   fix_str_ws() throw() {
      static fix_str_imp empty_imp = {1, 0, ULONG_MAX, eos()};
      imp = &empty_imp;
      on_create(); // necessary here because construct() is not called! 
      increment();
   }

   /** copy constructor; this function does not allocate
    *  dynamic memory (=> exception specification: throw())
    */
   fix_str_ws (const fix_str_ws& other) throw() {
      imp = other.imp;
      increment();
   }

   /** constructor */
   fix_str_ws (const fix_str_ws& f1, const fix_str_ws& f2) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length());
   }

   /** constructor */
   fix_str_ws (const fix_str_ws& f1, const fix_str_ws& f2, const fix_str_ws& f3) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length());
   }
   
   /** constructor */
   fix_str_ws (const fix_str_ws& f1, const fix_str_ws& f2, const fix_str_ws& f3, const fix_str_ws& f4) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length());
   }

   /** constructor */
   fix_str_ws (const fix_str_ws& f1, const fix_str_ws& f2, const fix_str_ws& f3, const fix_str_ws& f4, const fix_str_ws& f5) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length());
   }

   /** constructor */
   fix_str_ws (const fix_str_ws& f1, const fix_str_ws& f2, const fix_str_ws& f3, const fix_str_ws& f4, const fix_str_ws& f5, const fix_str_ws& f6) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length(), f6.c_str(), f6.length());
   }

   /** constructor */
   fix_str_ws (const fix_str_ws& f1, const fix_str_ws& f2, const fix_str_ws& f3, const fix_str_ws& f4, const fix_str_ws& f5, const fix_str_ws& f6, const fix_str_ws& f7) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length(), f6.c_str(), f6.length(), f7.c_str(), f7.length());
   }

   /** constructor */
   fix_str_ws (const fix_str_ws& f1, const fix_str_ws& f2, const fix_str_ws& f3, const fix_str_ws& f4, const fix_str_ws& f5, const fix_str_ws& f6, const fix_str_ws& f7, const fix_str_ws& f8) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length(), f6.c_str(), f6.length(), f7.c_str(), f7.length(), f8.c_str(), f8.length());
   }
   
   /** constructor */
   fix_str_ws (const_pointer s) throw (std::bad_alloc) {
      FS_ASSERT_DO(s, s=peos());
      construct (s, fs_strlen (s));
   }

   /** constructor for 2 - 8 arguments
    * @param s_:   pointer to an array of characters terminated with '\0'
    */
  fix_str_ws (const_pointer s1, const_pointer s2,
           const_pointer s3 = peos(), const_pointer s4 = peos(), const_pointer s5 = peos(),
           const_pointer s6 = peos(), const_pointer s7 = peos(), const_pointer s8 = peos()) throw (std::bad_alloc) {
     FS_ASSERT_DO(s1, s1=peos()); FS_ASSERT_DO(s2, s2=peos()); FS_ASSERT_DO(s3, s3=peos()); FS_ASSERT_DO(s4, s4=peos());
     FS_ASSERT_DO(s5, s5=peos()); FS_ASSERT_DO(s6, s6=peos()); FS_ASSERT_DO(s7, s7=peos()); FS_ASSERT_DO(s8, s8=peos());
     construct (s1, fs_strlen (s1), s2, fs_strlen (s2), s3, fs_strlen (s3), s4, fs_strlen (s4),
                s5, fs_strlen (s5), s6, fs_strlen (s6), s7, fs_strlen (s7), s8, fs_strlen (s8));
   }

   /** constructor
    * @param s:    pointer to an array of characters
    * @param len:  number of characters
    */
   fix_str_ws (const_pointer s, size_type len) throw (std::bad_alloc) {
      FS_ASSERT_DO(s, s=peos());
      construct (s, len);
   }

   /** constructor for 2 - 8 arguments
    * @param s_:    pointer to an array of characters
    * @param len_:  number of characters
    */
   fix_str_ws (const_pointer s1, size_type len1, const_pointer s2, size_type len2,
            const_pointer s3 = peos(), size_type len3 = 0, const_pointer s4 = peos(), size_type len4 = 0,
            const_pointer s5 = peos(), size_type len5 = 0, const_pointer s6 = peos(), size_type len6 = 0,
            const_pointer s7 = peos(), size_type len7 = 0, const_pointer s8 = peos(), size_type len8 = 0 ) throw (std::bad_alloc) {
     FS_ASSERT_DO(s1, s1=peos()); FS_ASSERT_DO(s2, s2=peos()); FS_ASSERT_DO(s3, s3=peos()); FS_ASSERT_DO(s4, s4=peos());
     FS_ASSERT_DO(s5, s5=peos()); FS_ASSERT_DO(s6, s6=peos()); FS_ASSERT_DO(s7, s7=peos()); FS_ASSERT_DO(s8, s8=peos());
     construct (s1, len1, s2, len2, s3, len3, s4, len4, s5, len5, s6, len6, s7, len7, s8, len8);
   }
   
   /** constructor
    * @param c: a character
    * @param n: number of repetitions of c
    */
  fix_str_ws (value_type c, size_type n=1) throw (std::bad_alloc) {
     construct (c, n);
  }

   /** destructor */
   ~fix_str_ws() throw() {
      dispose();
   }

   /** assignment operator; this function does not allocate
    *  dynamic memory (=> exception specification: throw())
    */
   fix_str_ws& operator= (const fix_str_ws& other) throw() {
      if (this != &other) {
         dispose(); 
         imp = other.imp;
         increment();
      }
      return *this;
   }

   /** STL random access reverse iterator type*/
#if defined (_MSC_VER) && _MSC_VER <= 1200  // MSVC++ 6.0
	typedef std::reverse_iterator<const_pointer, value_type, const value_type&, const value_type*, difference_type> const_reverse_iterator;
#else
   typedef std::reverse_iterator<const_pointer> const_reverse_iterator;
#endif
   /** STL const random access reverse iterator type*/
   typedef const_reverse_iterator reverse_iterator;
   /** STL random access iterator type*/
   typedef const_pointer iterator;
   /** STL const random access iterator type*/
   typedef const_pointer const_iterator;

   /** @return STL iterator to the first character in the object. */
   iterator begin() const throw() { return c_str(); }

   /** @return STL iterator to one past the end of the sequence of characters */
   iterator end()   const throw() { return c_str() + length(); }

   /** @return STL reverse iterator to the first character in the object
    *          (referring to the reversed sequence of characters).
    */
   reverse_iterator rbegin() const throw() { return reverse_iterator (end()); }

   /** @return STL reverse iterator to one past the end
    *          (referring to the reversed sequence of characters).
    */
   reverse_iterator rend()   const throw() { return reverse_iterator (begin()); }

   /** @return first character; length() must be > 0 */
   value_type front() const throw() { FS_ASSERT_RETURN(length(), eos()); return * begin(); }

   /** @return last character; length() must be > 0 */
   value_type back () const throw() { FS_ASSERT_RETURN(length(), eos()); return * (end() - 1); }

   /** @return const pointer to the internal array of characters with length length() */
   const_pointer c_str() const throw() {
      return imp->data;
   }

   /** @return number of characters, not bytes */
   size_type length() const throw() {
      return imp->len;
   }

   value_type operator[] (size_type pos) const throw() {
      FS_ASSERT_RETURN((pos < length()), eos());
      return * (c_str() + pos);
   }

   /** @return hash code */
   unsigned long hash_code() const throw() {
      if (imp->hashCode == ULONG_MAX) {
         imp->hashCode = fnv_32_buf ((void*) c_str(), length() * sizeof (value_type));
         if (imp->hashCode == ULONG_MAX) {
            imp->hashCode = 0; // ULONG_MAX is a 'reserved' value
         }
      }
      return imp->hashCode;
   }

   /** @return position of character c or fix_str_ws::npos if c is not found
    *  @param  c:      character to be found
    *  @param  offset: position where search starts
    */
   size_type find (value_type c, size_type offset = 0) const throw() {
      size_type ret = fix_str_ws::npos;
      if (offset < length()) {
         const_pointer p = fs_strchr (c_str() + offset, c);
         if (p) {
            ret = p - c_str();
         }
      }
      return ret;
   }

   /** @return position of str or fix_str_ws::npos if str is not found
    *  @param  str:    fix_str_ws to be found
    *  @param  offset: position where search starts
    */
   size_type find (const fix_str_ws& str, size_type offset = 0) const throw() {
      return find (str.c_str(), offset);
   }

   /** @return position of str or fix_str_ws::npos if str is not found
    *  @param  s:      character string be found
    *  @param  offset: position where search starts
    */
   size_type find (const_pointer s, size_type offset = 0) const throw() {
      FS_ASSERT_RETURN(s, fix_str_ws::npos);
      size_type ret = fix_str_ws::npos;
      if (offset < length()) {
         const_pointer p = fs_strstr (c_str() + offset, s);
         if (p) {
            ret = p - c_str();
         }
      }
      return ret;
   }

   /** @return position of last occurrence of c or fix_str_ws::npos if c is not found
    *  @param  c:      character to be found
    *  @param  offset: position (from the beginning) where search starts
    */
   size_type rfind (value_type c, size_type offset = 0) const throw() {
      size_type ret = fix_str_ws::npos;
      if (offset < length()) {
         const_pointer p = fs_strrchr (c_str() + offset, c);
         if (p) {
           ret = p - c_str();
         }
      }
      return ret;
   }

   /** @return position of last occurrence of str or fix_str_ws::npos if str is not found
    *  @param  str:    fix_str_ws to be found
    *  @param  offset: position (from the beginning) where search starts
    */
   size_type rfind (const fix_str_ws& str, size_type offset = 0) const throw() {
      size_type ret = fix_str_ws::npos;
      if (offset < length()) {
         iterator result = std::find_end (begin() + offset, end(), str.begin(), str.end());
         if (result != end()) {
            ret = result - begin();
         }
         if (str.length() == 0) {  // special case empty search string
            ret = offset;
         }
      }
      return ret;
   }

   /** @return position of last occurrence of s or fix_str_ws::npos if str is not found
    *  @param  s:      character string to be found
    *  @param  offset: position (from the beginning) where search starts
    */
   size_type rfind (const_pointer s, size_type offset = 0) const throw() {
      FS_ASSERT_RETURN(s, fix_str_ws::npos);
      size_type ret = fix_str_ws::npos;
      if (offset < length()) {
         size_type lenStr = fs_strlen (s);
         iterator result = std::find_end (begin() + offset, end(), s, s + lenStr);
         if (result != end()) {
            ret = result - begin();
         }
         if (lenStr == 0) {  // special case empty search string
            ret = offset;
         }
      }
      return ret;
   }
   
   friend inline
   bool operator== (const fix_str_ws& left, const fix_str_ws& right) throw() {
      bool eq = left.imp == right.imp;
      if (! eq) {
         eq =   left.length()    == right.length()
             && *(left.c_str())  == *(right.c_str())
             && left.hash_code() == right.hash_code()
             && fs_strcmp (left.c_str(), right.c_str()) == 0;
      }
      return eq;
   }
   friend inline 
   bool operator== (const fix_str_ws& left, const_pointer right) throw() {
      FS_ASSERT_RETURN(right, false);
      return fs_strcmp (left.c_str(), right) == 0;
   }
   friend inline
   bool operator== (const_pointer left, const fix_str_ws& right) throw() {
      FS_ASSERT_RETURN(left, false);
      return fs_strcmp (left, right.c_str()) == 0;
   }

   friend inline
   bool operator< (const fix_str_ws& left, const fix_str_ws& right) throw() {
      return fs_strcmp (left.c_str(), right.c_str()) < 0;
   }
   friend inline 
   bool operator< (const fix_str_ws& left, const_pointer right) throw() {
      FS_ASSERT_DO(right, right = peos());
      return fs_strcmp (left.c_str(), right) < 0;
   }
   friend inline 
   bool operator< (const_pointer left, const fix_str_ws& right) throw() {
      FS_ASSERT_DO(left, left = peos());
      return fs_strcmp (left, right.c_str()) < 0;
   }

   friend inline bool operator!= (const fix_str_ws& left, const fix_str_ws& right) throw() { return ! (left == right); }
   friend inline bool operator!= (const fix_str_ws& left, const_pointer right) throw()  { return ! (left == right); }
   friend inline bool operator!= (const_pointer left, const fix_str_ws& right) throw()  { return ! (left == right); }

   friend inline bool operator>  (const fix_str_ws& left, const fix_str_ws& right) throw() { return right < left; }
   friend inline bool operator>  (const fix_str_ws& left, const_pointer right) throw() { return right < left; }
   friend inline bool operator>  (const_pointer left, const fix_str_ws& right) throw() { return right < left; }
   
   friend inline bool operator<= (const fix_str_ws& left, const fix_str_ws& right) throw() { return !(right < left); }
   friend inline bool operator<= (const fix_str_ws& left, const_pointer right) throw() { return !(right < left); }
   friend inline bool operator<= (const_pointer left, const fix_str_ws& right) throw() { return !(right < left); }

   friend inline bool operator>= (const fix_str_ws& left, const fix_str_ws& right) throw() { return !(left < right); }
   friend inline bool operator>= (const fix_str_ws& left, const_pointer right) throw() { return !(left < right); }
   friend inline bool operator>= (const_pointer left, const fix_str_ws& right) throw() { return !(left < right); }

// ---- static --------------------------------------------------------------------------
//  functions creating a new fix_str_ws
// ---- static --------------------------------------------------------------------------
   
   /** @return new instance of fix_str_ws for which is guaranteed:
       original.c_str() != returned.c_str()
   */
   static inline
   fix_str_ws duplicate (const fix_str_ws& original) throw (std::bad_alloc) {
      return fix_str_ws (original.c_str(), original.length());
   }

   /** @return new instance representing a partial copy of the original
    *  @param  original: fix_str_ws to be partially copied
    *  @param  offset:   position where the copying starts
    */
   static inline
   fix_str_ws sub_str (const fix_str_ws& original, size_type offset) throw (std::bad_alloc) {
      FS_ASSERT_RETURN((offset == 0 && original.length() == 0) || (offset < original.length()), fix_str_ws());
      return fix_str_ws (original.c_str() + offset, original.length() - offset);
   }

   /** @return new instance representing a partial copy of the original
    *  @param  original: fix_str_ws to be partially copied
    *  @param  offset:   position where the copying starts
    *  @param  len:      number of copied characters starting from offset
    */
   static inline
   fix_str_ws sub_str (const fix_str_ws& original, size_type offset, size_type len) throw (std::bad_alloc) {
      FS_ASSERT_RETURN((offset == 0 && len == 0 && original.length() == 0) || (offset < original.length() && offset + len <= original.length()), fix_str_ws_wm());      
      return fix_str_ws (original.c_str() + offset, len);
   }

   /** @return new instance representing a copy of the original without leading
    *          white space; if the original contains no leading white space
    *          the original is returned
    *  @param  original: fix_str_ws to be front trimmed
    */
   static inline
   fix_str_ws trim_front (const fix_str_ws& original) throw (std::bad_alloc) {
      fix_str_ws ret = original;
      size_type start = find_first_non_whitespace (original.c_str(), original.length());
      
      if (start == 0) {
         ;  // original contains no leading whitespace
      } else if (start == fix_str_ws::npos) {
         ret = fix_str_ws(); // original contains whitespace only!
      } else {
         ret = fix_str_ws (original.c_str() + start, original.length() - start);
      }
      
      return ret;
   }

   /** @return new instance representing a copy of the original without trailing
    *          white space; if the original contains no trailing white space
    *          the original is returned
    *  @param  original: fix_str_ws to be back trimmed
    */
   static inline
   fix_str_ws trim_back (const fix_str_ws& original) throw (std::bad_alloc) {
      fix_str_ws ret = original;
      size_type last = find_last_non_whitespace (original.c_str(), original.length());
      
      if (last == fix_str_ws::npos) {
         ret = fix_str_ws(); // original contains whitespace only!
      } else if (1 + last == original.length()) {
         ;  // original contains no trailing whitespace
      } else {
         ret = fix_str_ws (original.c_str(), 1 + last);
      }
      
      return ret;
   }

   /** @return new instance representing a copy of the original without leading
    *          and trailing white space; if the original contains no leading
    *          and trailing white space the original is returned
    *  @param  original: fix_str_ws to be back trimmed
    */
   static inline
   fix_str_ws trim (const fix_str_ws& original) throw (std::bad_alloc) {
      fix_str_ws ret = original;
      size_type start = find_first_non_whitespace (original.c_str(), original.length()),
                last  = find_last_non_whitespace  (original.c_str(), original.length());
      
      if (start == fix_str_ws::npos) {
         assert (last == fix_str_ws::npos);
         ret = fix_str_ws(); // original contains whitespace only!
      } else if (start == 0 && 1 + last == original.length()) {
         ;  // original contains no leading or trailing whitespace
      } else {
         ret = fix_str_ws (original.c_str() + start, 1 + last - start);
      }
      
      return ret;
   }
   
   /** @return new instance representing a copy of the original padded to the
    *          specified length at the front;
    *          if the original needs no padding the original is returned
    *  @param  original: fix_str_ws to be front padded
    *  @param  n:        minimal length of returned fix_str_ws
    *  @param  c:        character for padding
    *  @param  cutLeadingWhiteSpace: true if leading whitespace should be cut
    *                                before padding with c, false otherwise
    */
   static inline
   fix_str_ws pad_front (const fix_str_ws& original, size_type n, value_type c, bool cutLeadingWhiteSpace = false) throw (std::bad_alloc) {
      fix_str_ws   ret = original;
      size_type len = original.length(),
                offset = 0;
      
      if (cutLeadingWhiteSpace) {
         offset = find_first_non_whitespace (original.c_str(), original.length());
         len = (offset == fix_str_ws::npos) ? 0 : len - offset;
      } 
      if (len < n) { // trim
         ret = pad (-1, original.c_str() + offset, len, n - len, c);
      } else if (len != original.length()) { // don't trim, cut leading whitespace only
         ret = sub_str (original, offset, len);
      }
      
      return ret;
   }

   /** @return new instance representing a copy of the original padded to the
    *          specified length at the back;
    *          if the original needs no padding the original is returned
    *  @param  original: fix_str_ws to be back padded
    *  @param  n:        minimal length of returned fix_str_ws
    *  @param  c:        character for padding
    *  @param  cutTrailingWhiteSpace: true if trailing whitespace should be cut
    *                                 before padding with c, false otherwise
    */
   static inline
   fix_str_ws pad_back (const fix_str_ws& original, size_type n, value_type c, bool cutTrailingWhiteSpace = false) throw (std::bad_alloc) {
      fix_str_ws   ret = original;
      size_type len = original.length();
      
      if (cutTrailingWhiteSpace) {
         len = 1 + find_last_non_whitespace (original.c_str(), original.length());
      }
      if (len < n) { // trim
         ret = pad (1, original.c_str(), len, n - len, c);
      } else if (len != original.length()) { // don't trim, cut trailing whitespace only
         ret = sub_str (original, 0, len);
      }
      
      return ret;
   }

   /** @return new instance of the fix_str_ws representation of a long value
    */
   static inline
   fix_str_ws value_of (int i) throw (std::bad_alloc) {
      fix_str_ws ret;
      value_type buf[32 * sizeof (value_type)] = {eos()};
		int n = fs_snprintf (buf, sizeof (buf), i);
      if (n > 0) {
         ret = fix_str_ws (buf, n);
      }

      return ret;
   }

   /** @return new instance of the fix_str_ws representation of a double value
    */
   static inline
   fix_str_ws value_of (double d) throw (std::bad_alloc) {
      fix_str_ws ret;
      value_type buf[128 * sizeof (value_type)] = {eos()};
		int n = fs_snprintf (buf, sizeof (buf), d);
      if (n > 0) {
         ret = fix_str_ws (buf, n);
      }

      return ret;
   }

private:
   struct fix_str_imp {
      long       counter;
      size_type  len;
      unsigned long hashCode;
      value_type    data[sizeof(int)];
   };

   fix_str_imp* imp;

   void init (size_type len) throw (std::bad_alloc) {
      const size_type fixSize =   sizeof (fix_str_imp)
                                - sizeof (value_type[sizeof(int)]) // - fix_str_imp::data
                                + sizeof (value_type);   // + end of string indicator ('\0')
      size_type varSize = len * sizeof (value_type);     // actual size required by number of characters
      size_type rawSize = fixSize + varSize;

      imp = static_cast<fix_str_imp*> (malloc (rawSize));
      if (! imp) {
         error_out_of_memory();
      }
//      dbgout << (void*) imp << " malloc" << std::endl;

      imp->counter  = 1;
      imp->len      = len;
      imp->hashCode = ULONG_MAX;
      imp->data[0]  = eos();
   }

   bool copy_str (const_pointer s, size_type len, size_type offset = 0) throw() {
      fs_strncpy (imp->data + offset, s, len)[len] = eos();
      return true;
   }

   void construct (
            const_pointer s1 = peos(), size_type len1 = 0,
            const_pointer s2 = peos(), size_type len2 = 0,
            const_pointer s3 = peos(), size_type len3 = 0,
            const_pointer s4 = peos(), size_type len4 = 0,
            const_pointer s5 = peos(), size_type len5 = 0,
            const_pointer s6 = peos(), size_type len6 = 0,
            const_pointer s7 = peos(), size_type len7 = 0,
            const_pointer s8 = peos(), size_type len8 = 0
      ) throw (std::bad_alloc) {

      init   (len1 + len2 + len3 + len4 + len5 + len6 + len7 + len8);

      len1 && copy_str (s1, len1);
      len2 && copy_str (s2, len2, len1);
      len3 && copy_str (s3, len3, len1 + len2);
      len4 && copy_str (s4, len4, len1 + len2 + len3);
      len5 && copy_str (s5, len5, len1 + len2 + len3 + len4);
      len6 && copy_str (s6, len6, len1 + len2 + len3 + len4 + len5);
      len7 && copy_str (s7, len7, len1 + len2 + len3 + len4 + len5 + len6);
      len8 && copy_str (s8, len8, len1 + len2 + len3 + len4 + len5 + len6 + len7);

      on_create();
   }

   void construct (value_type c, size_type n) throw (std::bad_alloc) {
      init (n);
      for (size_type i = 0; i < n; ++i) {
         imp->data[i] = c;
      }
      imp->data[n] = eos();

      on_create();
   }


   void on_create() throw() {
      return;
   }

   void increment() throw() {
      atomic_increment();
   }

   void dispose() throw() {
     assert (*(c_str() + length()) == eos());
     if (atomic_decrement() == 0) {
         free (imp);
//          dbgout << (void*) imp << " free" << std::endl;
      }
   }

   void atomic_increment() throw() {
      ++imp->counter; 
   }

   long atomic_decrement() throw() {
      return --imp->counter;
   }


// ---- helper functions ----

   static inline
   size_type find_first_non_whitespace (const_pointer s, size_type length) throw() {
      assert (s);
      iterator first (s), last (s + length), current (first);

      for (; current != last && fs_isspace (*current);  ++current) {
         ;
      }

      size_type pos = current - first;
      if (current == last) {
         pos = fix_str_ws::npos;
      }

      return pos;
   }

   static inline
   size_type find_last_non_whitespace (const_pointer s, size_type length) throw() {
      assert (s);
      reverse_iterator first (s + length), last (s), current (first);

      for (; current != last && fs_isspace (*current);  ++current) {
         ;
      }

      size_type pos = fix_str_ws::npos;
      if (current != last) {
         pos = last - current - 1;
      }

      return pos;
   }

   struct array_deleter {
      array_deleter() throw() : arr(0) {}
      ~array_deleter() throw() { delete [] arr; }
      void set (value_type array[]) throw() { assert (arr == 0); arr = array; }
   private:
      pointer arr;      
      array_deleter (const array_deleter&);
      array_deleter& operator= (const array_deleter&);
   };

   static inline 
   fix_str_ws pad (int where, const_pointer startpos, size_type length, size_type numpad, value_type c) throw (std::bad_alloc) {
      const size_type bufSiz = 128;
      value_type buf[bufSiz];
      pointer startpad = buf;
      array_deleter arrDel;
      if (! (numpad < bufSiz)) {
         startpad = new (std::nothrow) value_type[numpad + 1];
         if (! startpad) {
            error_out_of_memory();
         }
         arrDel.set (startpad);
      }
      size_type i = 0;
      for (; i < numpad; ++i) {
         startpad[i] = c;
      }
      startpad[i] = eos();
      
      fix_str_ws ret; 
      if (where < 0) { // pad front
         ret = fix_str_ws (startpad,  numpad, startpos, length);
      } else {         // pad back
         ret = fix_str_ws (startpos, length, startpad,  numpad);
      }

      return ret;
   }

   static
   void error_out_of_memory() throw (std::bad_alloc) {
      throw std::bad_alloc();
   }

   static inline
   value_type eos() throw() {
      return fs_eos ((const_pointer) 0);
   }

   static inline
   const_pointer peos() throw() {
      return fs_peos ((const_pointer) 0);
   }

};


 /**
  * @class fix_str_wm
  * assignable but otherwise immutable string class
  * - value type: wchar_t
  * - suitable for multi-threaded environments: YES
  * - safe for concurrent writes to the same fix_str_wm object: NO
  * 
  * example: 
  * <pre>
  * fix_str_wm fs1 (_T("Hello, world!"));
  * size_t pos = fs1.find (_T("ello"));
  * fix_str_wm fs2 = fix_str_wm::sub_str (fs1, 0, 4);
  * </pre>
  */
class fix_str_wm {
public:
   typedef wchar_t value_type;
   typedef value_type* pointer;
   typedef value_type& reference;
   typedef const value_type* const_pointer;
   typedef const value_type& const_reference;
   typedef size_t    size_type;
   typedef ptrdiff_t difference_type;

   enum { npos = size_type (-1) };

public:

   /** default constructor;
    *  create fix_str_wm object with length() == 0; this function does not allocate
    *  dynamic memory (=> exception specification: throw())
    */
   fix_str_wm() throw() {
      static fix_str_imp empty_imp = {1, 0, ULONG_MAX, eos()};
      imp = &empty_imp;
      on_create(); // necessary here because construct() is not called! 
      increment();
   }

   /** copy constructor; this function does not allocate
    *  dynamic memory (=> exception specification: throw())
    */
   fix_str_wm (const fix_str_wm& other) throw() {
      imp = other.imp;
      increment();
   }

   /** constructor */
   fix_str_wm (const fix_str_wm& f1, const fix_str_wm& f2) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length());
   }

   /** constructor */
   fix_str_wm (const fix_str_wm& f1, const fix_str_wm& f2, const fix_str_wm& f3) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length());
   }
   
   /** constructor */
   fix_str_wm (const fix_str_wm& f1, const fix_str_wm& f2, const fix_str_wm& f3, const fix_str_wm& f4) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length());
   }

   /** constructor */
   fix_str_wm (const fix_str_wm& f1, const fix_str_wm& f2, const fix_str_wm& f3, const fix_str_wm& f4, const fix_str_wm& f5) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length());
   }

   /** constructor */
   fix_str_wm (const fix_str_wm& f1, const fix_str_wm& f2, const fix_str_wm& f3, const fix_str_wm& f4, const fix_str_wm& f5, const fix_str_wm& f6) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length(), f6.c_str(), f6.length());
   }

   /** constructor */
   fix_str_wm (const fix_str_wm& f1, const fix_str_wm& f2, const fix_str_wm& f3, const fix_str_wm& f4, const fix_str_wm& f5, const fix_str_wm& f6, const fix_str_wm& f7) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length(), f6.c_str(), f6.length(), f7.c_str(), f7.length());
   }

   /** constructor */
   fix_str_wm (const fix_str_wm& f1, const fix_str_wm& f2, const fix_str_wm& f3, const fix_str_wm& f4, const fix_str_wm& f5, const fix_str_wm& f6, const fix_str_wm& f7, const fix_str_wm& f8) throw (std::bad_alloc) {
      construct (f1.c_str(), f1.length(), f2.c_str(), f2.length(), f3.c_str(), f3.length(), f4.c_str(), f4.length(),
                 f5.c_str(), f5.length(), f6.c_str(), f6.length(), f7.c_str(), f7.length(), f8.c_str(), f8.length());
   }
   
   /** constructor */
   fix_str_wm (const_pointer s) throw (std::bad_alloc) {
      FS_ASSERT_DO(s, s=peos());
      construct (s, fs_strlen (s));
   }

   /** constructor for 2 - 8 arguments
    * @param s_:   pointer to an array of characters terminated with '\0'
    */
  fix_str_wm (const_pointer s1, const_pointer s2,
           const_pointer s3 = peos(), const_pointer s4 = peos(), const_pointer s5 = peos(),
           const_pointer s6 = peos(), const_pointer s7 = peos(), const_pointer s8 = peos()) throw (std::bad_alloc) {
     FS_ASSERT_DO(s1, s1=peos()); FS_ASSERT_DO(s2, s2=peos()); FS_ASSERT_DO(s3, s3=peos()); FS_ASSERT_DO(s4, s4=peos());
     FS_ASSERT_DO(s5, s5=peos()); FS_ASSERT_DO(s6, s6=peos()); FS_ASSERT_DO(s7, s7=peos()); FS_ASSERT_DO(s8, s8=peos());
     construct (s1, fs_strlen (s1), s2, fs_strlen (s2), s3, fs_strlen (s3), s4, fs_strlen (s4),
                s5, fs_strlen (s5), s6, fs_strlen (s6), s7, fs_strlen (s7), s8, fs_strlen (s8));
   }

   /** constructor
    * @param s:    pointer to an array of characters
    * @param len:  number of characters
    */
   fix_str_wm (const_pointer s, size_type len) throw (std::bad_alloc) {
      FS_ASSERT_DO(s, s=peos());
      construct (s, len);
   }

   /** constructor for 2 - 8 arguments
    * @param s_:    pointer to an array of characters
    * @param len_:  number of characters
    */
   fix_str_wm (const_pointer s1, size_type len1, const_pointer s2, size_type len2,
            const_pointer s3 = peos(), size_type len3 = 0, const_pointer s4 = peos(), size_type len4 = 0,
            const_pointer s5 = peos(), size_type len5 = 0, const_pointer s6 = peos(), size_type len6 = 0,
            const_pointer s7 = peos(), size_type len7 = 0, const_pointer s8 = peos(), size_type len8 = 0 ) throw (std::bad_alloc) {
     FS_ASSERT_DO(s1, s1=peos()); FS_ASSERT_DO(s2, s2=peos()); FS_ASSERT_DO(s3, s3=peos()); FS_ASSERT_DO(s4, s4=peos());
     FS_ASSERT_DO(s5, s5=peos()); FS_ASSERT_DO(s6, s6=peos()); FS_ASSERT_DO(s7, s7=peos()); FS_ASSERT_DO(s8, s8=peos());
     construct (s1, len1, s2, len2, s3, len3, s4, len4, s5, len5, s6, len6, s7, len7, s8, len8);
   }
   
   /** constructor
    * @param c: a character
    * @param n: number of repetitions of c
    */
  fix_str_wm (value_type c, size_type n=1) throw (std::bad_alloc) {
     construct (c, n);
  }

   /** destructor */
   ~fix_str_wm() throw() {
      dispose();
   }

   /** assignment operator; this function does not allocate
    *  dynamic memory (=> exception specification: throw())
    */
   fix_str_wm& operator= (const fix_str_wm& other) throw() {
      if (this != &other) {
         dispose(); 
         imp = other.imp;
         increment();
      }
      return *this;
   }

   /** STL random access reverse iterator type*/
#if defined (_MSC_VER) && _MSC_VER <= 1200  // MSVC++ 6.0
	typedef std::reverse_iterator<const_pointer, value_type, const value_type&, const value_type*, difference_type> const_reverse_iterator;
#else
   typedef std::reverse_iterator<const_pointer> const_reverse_iterator;
#endif
   /** STL const random access reverse iterator type*/
   typedef const_reverse_iterator reverse_iterator;
   /** STL random access iterator type*/
   typedef const_pointer iterator;
   /** STL const random access iterator type*/
   typedef const_pointer const_iterator;

   /** @return STL iterator to the first character in the object. */
   iterator begin() const throw() { return c_str(); }

   /** @return STL iterator to one past the end of the sequence of characters */
   iterator end()   const throw() { return c_str() + length(); }

   /** @return STL reverse iterator to the first character in the object
    *          (referring to the reversed sequence of characters).
    */
   reverse_iterator rbegin() const throw() { return reverse_iterator (end()); }

   /** @return STL reverse iterator to one past the end
    *          (referring to the reversed sequence of characters).
    */
   reverse_iterator rend()   const throw() { return reverse_iterator (begin()); }

   /** @return first character; length() must be > 0 */
   value_type front() const throw() { FS_ASSERT_RETURN(length(), eos()); return * begin(); }

   /** @return last character; length() must be > 0 */
   value_type back () const throw() { FS_ASSERT_RETURN(length(), eos()); return * (end() - 1); }

   /** @return const pointer to the internal array of characters with length length() */
   const_pointer c_str() const throw() {
      return imp->data;
   }

   /** @return number of characters, not bytes */
   size_type length() const throw() {
      return imp->len;
   }

   value_type operator[] (size_type pos) const throw() {
      FS_ASSERT_RETURN((pos < length()), eos());
      return * (c_str() + pos);
   }

   /** @return hash code */
   unsigned long hash_code() const throw() {
      if (imp->hashCode == ULONG_MAX) {
         imp->hashCode = fnv_32_buf ((void*) c_str(), length() * sizeof (value_type));
         if (imp->hashCode == ULONG_MAX) {
            imp->hashCode = 0; // ULONG_MAX is a 'reserved' value
         }
      }
      return imp->hashCode;
   }

   /** @return position of character c or fix_str_wm::npos if c is not found
    *  @param  c:      character to be found
    *  @param  offset: position where search starts
    */
   size_type find (value_type c, size_type offset = 0) const throw() {
      size_type ret = fix_str_wm::npos;
      if (offset < length()) {
         const_pointer p = fs_strchr (c_str() + offset, c);
         if (p) {
            ret = p - c_str();
         }
      }
      return ret;
   }

   /** @return position of str or fix_str_wm::npos if str is not found
    *  @param  str:    fix_str_wm to be found
    *  @param  offset: position where search starts
    */
   size_type find (const fix_str_wm& str, size_type offset = 0) const throw() {
      return find (str.c_str(), offset);
   }

   /** @return position of str or fix_str_wm::npos if str is not found
    *  @param  s:      character string be found
    *  @param  offset: position where search starts
    */
   size_type find (const_pointer s, size_type offset = 0) const throw() {
      FS_ASSERT_RETURN(s, fix_str_wm::npos);
      size_type ret = fix_str_wm::npos;
      if (offset < length()) {
         const_pointer p = fs_strstr (c_str() + offset, s);
         if (p) {
            ret = p - c_str();
         }
      }
      return ret;
   }

   /** @return position of last occurrence of c or fix_str_wm::npos if c is not found
    *  @param  c:      character to be found
    *  @param  offset: position (from the beginning) where search starts
    */
   size_type rfind (value_type c, size_type offset = 0) const throw() {
      size_type ret = fix_str_wm::npos;
      if (offset < length()) {
         const_pointer p = fs_strrchr (c_str() + offset, c);
         if (p) {
           ret = p - c_str();
         }
      }
      return ret;
   }

   /** @return position of last occurrence of str or fix_str_wm::npos if str is not found
    *  @param  str:    fix_str_wm to be found
    *  @param  offset: position (from the beginning) where search starts
    */
   size_type rfind (const fix_str_wm& str, size_type offset = 0) const throw() {
      size_type ret = fix_str_wm::npos;
      if (offset < length()) {
         iterator result = std::find_end (begin() + offset, end(), str.begin(), str.end());
         if (result != end()) {
            ret = result - begin();
         }
         if (str.length() == 0) {  // special case empty search string
            ret = offset;
         }
      }
      return ret;
   }

   /** @return position of last occurrence of s or fix_str_wm::npos if str is not found
    *  @param  s:      character string to be found
    *  @param  offset: position (from the beginning) where search starts
    */
   size_type rfind (const_pointer s, size_type offset = 0) const throw() {
      FS_ASSERT_RETURN(s, fix_str_wm::npos);
      size_type ret = fix_str_wm::npos;
      if (offset < length()) {
         size_type lenStr = fs_strlen (s);
         iterator result = std::find_end (begin() + offset, end(), s, s + lenStr);
         if (result != end()) {
            ret = result - begin();
         }
         if (lenStr == 0) {  // special case empty search string
            ret = offset;
         }
      }
      return ret;
   }
   
   friend inline
   bool operator== (const fix_str_wm& left, const fix_str_wm& right) throw() {
      bool eq = left.imp == right.imp;
      if (! eq) {
         eq =   left.length()    == right.length()
             && *(left.c_str())  == *(right.c_str())
             && left.hash_code() == right.hash_code()
             && fs_strcmp (left.c_str(), right.c_str()) == 0;
      }
      return eq;
   }
   friend inline 
   bool operator== (const fix_str_wm& left, const_pointer right) throw() {
      FS_ASSERT_RETURN(right, false);
      return fs_strcmp (left.c_str(), right) == 0;
   }
   friend inline
   bool operator== (const_pointer left, const fix_str_wm& right) throw() {
      FS_ASSERT_RETURN(left, false);
      return fs_strcmp (left, right.c_str()) == 0;
   }

   friend inline
   bool operator< (const fix_str_wm& left, const fix_str_wm& right) throw() {
      return fs_strcmp (left.c_str(), right.c_str()) < 0;
   }
   friend inline 
   bool operator< (const fix_str_wm& left, const_pointer right) throw() {
      FS_ASSERT_DO(right, right = peos());
      return fs_strcmp (left.c_str(), right) < 0;
   }
   friend inline 
   bool operator< (const_pointer left, const fix_str_wm& right) throw() {
      FS_ASSERT_DO(left, left = peos());
      return fs_strcmp (left, right.c_str()) < 0;
   }

   friend inline bool operator!= (const fix_str_wm& left, const fix_str_wm& right) throw() { return ! (left == right); }
   friend inline bool operator!= (const fix_str_wm& left, const_pointer right) throw()  { return ! (left == right); }
   friend inline bool operator!= (const_pointer left, const fix_str_wm& right) throw()  { return ! (left == right); }

   friend inline bool operator>  (const fix_str_wm& left, const fix_str_wm& right) throw() { return right < left; }
   friend inline bool operator>  (const fix_str_wm& left, const_pointer right) throw() { return right < left; }
   friend inline bool operator>  (const_pointer left, const fix_str_wm& right) throw() { return right < left; }
   
   friend inline bool operator<= (const fix_str_wm& left, const fix_str_wm& right) throw() { return !(right < left); }
   friend inline bool operator<= (const fix_str_wm& left, const_pointer right) throw() { return !(right < left); }
   friend inline bool operator<= (const_pointer left, const fix_str_wm& right) throw() { return !(right < left); }

   friend inline bool operator>= (const fix_str_wm& left, const fix_str_wm& right) throw() { return !(left < right); }
   friend inline bool operator>= (const fix_str_wm& left, const_pointer right) throw() { return !(left < right); }
   friend inline bool operator>= (const_pointer left, const fix_str_wm& right) throw() { return !(left < right); }

// ---- static --------------------------------------------------------------------------
//  functions creating a new fix_str_wm
// ---- static --------------------------------------------------------------------------
   
   /** @return new instance of fix_str_wm for which is guaranteed:
       original.c_str() != returned.c_str()
   */
   static inline
   fix_str_wm duplicate (const fix_str_wm& original) throw (std::bad_alloc) {
      return fix_str_wm (original.c_str(), original.length());
   }

   /** @return new instance representing a partial copy of the original
    *  @param  original: fix_str_wm to be partially copied
    *  @param  offset:   position where the copying starts
    */
   static inline
   fix_str_wm sub_str (const fix_str_wm& original, size_type offset) throw (std::bad_alloc) {
      FS_ASSERT_RETURN((offset == 0 && original.length() == 0) || (offset < original.length()), fix_str_wm());
      return fix_str_wm (original.c_str() + offset, original.length() - offset);
   }

   /** @return new instance representing a partial copy of the original
    *  @param  original: fix_str_wm to be partially copied
    *  @param  offset:   position where the copying starts
    *  @param  len:      number of copied characters starting from offset
    */
   static inline
   fix_str_wm sub_str (const fix_str_wm& original, size_type offset, size_type len) throw (std::bad_alloc) {
      FS_ASSERT_RETURN((offset == 0 && len == 0 && original.length() == 0) || (offset < original.length() && offset + len <= original.length()), fix_str_wm_wm());      
      return fix_str_wm (original.c_str() + offset, len);
   }

   /** @return new instance representing a copy of the original without leading
    *          white space; if the original contains no leading white space
    *          the original is returned
    *  @param  original: fix_str_wm to be front trimmed
    */
   static inline
   fix_str_wm trim_front (const fix_str_wm& original) throw (std::bad_alloc) {
      fix_str_wm ret = original;
      size_type start = find_first_non_whitespace (original.c_str(), original.length());
      
      if (start == 0) {
         ;  // original contains no leading whitespace
      } else if (start == fix_str_wm::npos) {
         ret = fix_str_wm(); // original contains whitespace only!
      } else {
         ret = fix_str_wm (original.c_str() + start, original.length() - start);
      }
      
      return ret;
   }

   /** @return new instance representing a copy of the original without trailing
    *          white space; if the original contains no trailing white space
    *          the original is returned
    *  @param  original: fix_str_wm to be back trimmed
    */
   static inline
   fix_str_wm trim_back (const fix_str_wm& original) throw (std::bad_alloc) {
      fix_str_wm ret = original;
      size_type last = find_last_non_whitespace (original.c_str(), original.length());
      
      if (last == fix_str_wm::npos) {
         ret = fix_str_wm(); // original contains whitespace only!
      } else if (1 + last == original.length()) {
         ;  // original contains no trailing whitespace
      } else {
         ret = fix_str_wm (original.c_str(), 1 + last);
      }
      
      return ret;
   }

   /** @return new instance representing a copy of the original without leading
    *          and trailing white space; if the original contains no leading
    *          and trailing white space the original is returned
    *  @param  original: fix_str_wm to be back trimmed
    */
   static inline
   fix_str_wm trim (const fix_str_wm& original) throw (std::bad_alloc) {
      fix_str_wm ret = original;
      size_type start = find_first_non_whitespace (original.c_str(), original.length()),
                last  = find_last_non_whitespace  (original.c_str(), original.length());
      
      if (start == fix_str_wm::npos) {
         assert (last == fix_str_wm::npos);
         ret = fix_str_wm(); // original contains whitespace only!
      } else if (start == 0 && 1 + last == original.length()) {
         ;  // original contains no leading or trailing whitespace
      } else {
         ret = fix_str_wm (original.c_str() + start, 1 + last - start);
      }
      
      return ret;
   }
   
   /** @return new instance representing a copy of the original padded to the
    *          specified length at the front;
    *          if the original needs no padding the original is returned
    *  @param  original: fix_str_wm to be front padded
    *  @param  n:        minimal length of returned fix_str_wm
    *  @param  c:        character for padding
    *  @param  cutLeadingWhiteSpace: true if leading whitespace should be cut
    *                                before padding with c, false otherwise
    */
   static inline
   fix_str_wm pad_front (const fix_str_wm& original, size_type n, value_type c, bool cutLeadingWhiteSpace = false) throw (std::bad_alloc) {
      fix_str_wm   ret = original;
      size_type len = original.length(),
                offset = 0;
      
      if (cutLeadingWhiteSpace) {
         offset = find_first_non_whitespace (original.c_str(), original.length());
         len = (offset == fix_str_wm::npos) ? 0 : len - offset;
      } 
      if (len < n) { // trim
         ret = pad (-1, original.c_str() + offset, len, n - len, c);
      } else if (len != original.length()) { // don't trim, cut leading whitespace only
         ret = sub_str (original, offset, len);
      }
      
      return ret;
   }

   /** @return new instance representing a copy of the original padded to the
    *          specified length at the back;
    *          if the original needs no padding the original is returned
    *  @param  original: fix_str_wm to be back padded
    *  @param  n:        minimal length of returned fix_str_wm
    *  @param  c:        character for padding
    *  @param  cutTrailingWhiteSpace: true if trailing whitespace should be cut
    *                                 before padding with c, false otherwise
    */
   static inline
   fix_str_wm pad_back (const fix_str_wm& original, size_type n, value_type c, bool cutTrailingWhiteSpace = false) throw (std::bad_alloc) {
      fix_str_wm   ret = original;
      size_type len = original.length();
      
      if (cutTrailingWhiteSpace) {
         len = 1 + find_last_non_whitespace (original.c_str(), original.length());
      }
      if (len < n) { // trim
         ret = pad (1, original.c_str(), len, n - len, c);
      } else if (len != original.length()) { // don't trim, cut trailing whitespace only
         ret = sub_str (original, 0, len);
      }
      
      return ret;
   }

   /** @return new instance of the fix_str_wm representation of a long value
    */
   static inline
   fix_str_wm value_of (int i) throw (std::bad_alloc) {
      fix_str_wm ret;
      value_type buf[32 * sizeof (value_type)] = {eos()};
		int n = fs_snprintf (buf, sizeof (buf), i);
      if (n > 0) {
         ret = fix_str_wm (buf, n);
      }

      return ret;
   }

   /** @return new instance of the fix_str_wm representation of a double value
    */
   static inline
   fix_str_wm value_of (double d) throw (std::bad_alloc) {
      fix_str_wm ret;
      value_type buf[128 * sizeof (value_type)] = {eos()};
		int n = fs_snprintf (buf, sizeof (buf), d);
      if (n > 0) {
         ret = fix_str_wm (buf, n);
      }

      return ret;
   }

private:
   struct fix_str_imp {
      long       counter;
      size_type  len;
      unsigned long hashCode;
      value_type    data[sizeof(int)];
   };

   fix_str_imp* imp;

   void init (size_type len) throw (std::bad_alloc) {
      const size_type fixSize =   sizeof (fix_str_imp)
                                - sizeof (value_type[sizeof(int)]) // - fix_str_imp::data
                                + sizeof (value_type);   // + end of string indicator ('\0')
      size_type varSize = len * sizeof (value_type);     // actual size required by number of characters
      size_type rawSize = fixSize + varSize;

      imp = static_cast<fix_str_imp*> (malloc (rawSize));
      if (! imp) {
         error_out_of_memory();
      }
//      dbgout << (void*) imp << " malloc" << std::endl;

      imp->counter  = 1;
      imp->len      = len;
      imp->hashCode = ULONG_MAX;
      imp->data[0]  = eos();
   }

   bool copy_str (const_pointer s, size_type len, size_type offset = 0) throw() {
      fs_strncpy (imp->data + offset, s, len)[len] = eos();
      return true;
   }

   void construct (
            const_pointer s1 = peos(), size_type len1 = 0,
            const_pointer s2 = peos(), size_type len2 = 0,
            const_pointer s3 = peos(), size_type len3 = 0,
            const_pointer s4 = peos(), size_type len4 = 0,
            const_pointer s5 = peos(), size_type len5 = 0,
            const_pointer s6 = peos(), size_type len6 = 0,
            const_pointer s7 = peos(), size_type len7 = 0,
            const_pointer s8 = peos(), size_type len8 = 0
      ) throw (std::bad_alloc) {

      init   (len1 + len2 + len3 + len4 + len5 + len6 + len7 + len8);

      len1 && copy_str (s1, len1);
      len2 && copy_str (s2, len2, len1);
      len3 && copy_str (s3, len3, len1 + len2);
      len4 && copy_str (s4, len4, len1 + len2 + len3);
      len5 && copy_str (s5, len5, len1 + len2 + len3 + len4);
      len6 && copy_str (s6, len6, len1 + len2 + len3 + len4 + len5);
      len7 && copy_str (s7, len7, len1 + len2 + len3 + len4 + len5 + len6);
      len8 && copy_str (s8, len8, len1 + len2 + len3 + len4 + len5 + len6 + len7);

      on_create();
   }

   void construct (value_type c, size_type n) throw (std::bad_alloc) {
      init (n);
      for (size_type i = 0; i < n; ++i) {
         imp->data[i] = c;
      }
      imp->data[n] = eos();

      on_create();
   }


   void on_create() throw() {
      hash_code(); // lazy initialization of hash value only in single threaded mode 
      return;
   }

   void increment() throw() {
      atomic_increment();
   }

   void dispose() throw() {
     assert (*(c_str() + length()) == eos());
     if (atomic_decrement() == 0) {
         free (imp);
//          dbgout << (void*) imp << " free" << std::endl;
      }
   }

#  ifdef _WIN32
   void atomic_increment() throw() {
      InterlockedIncrement (& (imp->counter)); 
   }

   long atomic_decrement() throw() {
      return InterlockedDecrement (& (imp->counter)); 
   }

#  else
#    error not yet implemented for Non-Win32 systems // Linux/Intel: see eg.  __atomic_add() in  bits/atomicity.h
#  endif      



// ---- helper functions ----

   static inline
   size_type find_first_non_whitespace (const_pointer s, size_type length) throw() {
      assert (s);
      iterator first (s), last (s + length), current (first);

      for (; current != last && fs_isspace (*current);  ++current) {
         ;
      }

      size_type pos = current - first;
      if (current == last) {
         pos = fix_str_wm::npos;
      }

      return pos;
   }

   static inline
   size_type find_last_non_whitespace (const_pointer s, size_type length) throw() {
      assert (s);
      reverse_iterator first (s + length), last (s), current (first);

      for (; current != last && fs_isspace (*current);  ++current) {
         ;
      }

      size_type pos = fix_str_wm::npos;
      if (current != last) {
         pos = last - current - 1;
      }

      return pos;
   }

   struct array_deleter {
      array_deleter() throw() : arr(0) {}
      ~array_deleter() throw() { delete [] arr; }
      void set (value_type array[]) throw() { assert (arr == 0); arr = array; }
   private:
      pointer arr;      
      array_deleter (const array_deleter&);
      array_deleter& operator= (const array_deleter&);
   };

   static inline 
   fix_str_wm pad (int where, const_pointer startpos, size_type length, size_type numpad, value_type c) throw (std::bad_alloc) {
      const size_type bufSiz = 128;
      value_type buf[bufSiz];
      pointer startpad = buf;
      array_deleter arrDel;
      if (! (numpad < bufSiz)) {
         startpad = new (std::nothrow) value_type[numpad + 1];
         if (! startpad) {
            error_out_of_memory();
         }
         arrDel.set (startpad);
      }
      size_type i = 0;
      for (; i < numpad; ++i) {
         startpad[i] = c;
      }
      startpad[i] = eos();
      
      fix_str_wm ret; 
      if (where < 0) { // pad front
         ret = fix_str_wm (startpad,  numpad, startpos, length);
      } else {         // pad back
         ret = fix_str_wm (startpos, length, startpad,  numpad);
      }

      return ret;
   }

   static
   void error_out_of_memory() throw (std::bad_alloc) {
      throw std::bad_alloc();
   }

   static inline
   value_type eos() throw() {
      return fs_eos ((const_pointer) 0);
   }

   static inline
   const_pointer peos() throw() {
      return fs_peos ((const_pointer) 0);
   }

};


// default typedef for fix_str dependent on the defintion 
// of _MT (Multi_Threaded) and _UNICODE

#ifdef _WIN32

#if    defined (_UNICODE) &&  defined (_MT) // UNICODE, Multi-Threaded
  typedef fix_str_wm fix_str;
#elif  defined (_UNICODE) && !defined (_MT) // UNICODE, Single-Threaded
  typedef fix_str_ws fix_str;
#elif !defined (_UNICODE) &&  defined (_MT) // ASCII,   Multi-Threaded
  typedef fix_str_am fix_str;
#elif !defined (_UNICODE) && !defined (_MT) // ASCII,   Single-Threaded
  typedef fix_str_as fix_str;
#endif

#else

typedef fix_str_as fix_str;

#endif // _WIN32

#if !defined(_T)
#  if defined (_UNICODE)
#    define _T(x) L##x
#  else
#    define _T(x) x
#  endif
#endif

#if defined (_MSC_VER)
#  pragma warning (pop)
#endif

#endif // FIX_STR_H_INCLUDED
