# utf.hpp

A really really small, simple and lightweiht library for converting between UTF-8, UTF-16- UTF-32 (and possibly other encodings)


- **utf.hpp is a single header**: the library consists of a single header file (conveniently named `utf.hpp`). Include it, and you're good to go. There's nothing to build, nothing to link. Just `#include "utf.hpp"`.
- **utf.hpp has no external dependencies**: the library uses a few headers from the standard library, but requires no external dependencies.
-  **utf.hpp works with any string representation**: the library relies on iterators (or even raw pointers) to represent strings, and never creates strings or takes ownership of memory. (Which also means no calls to `new` or `malloc`)
- **utf.hpp is tiny**: like, really really small. About 400 lines of code all in all. You could read it in your lunch break.
- **utf.hpp is lightweight**: no heap allocations, no unnecesary copying of data. No virtual functions, and no exceptions. The library does what you ask it to, and nothing else, with no unnecessary overhead.
- **utf.hpp** is a really really easy way to convert text between UTF-8, UTF-16 and UTF-32.

##Example usage:


~~~
#include "utf.hpp"

// suppose we have some UTF-16 encoded data
std::vector<char16_t> u16data;

// and we want to convert it to UTF-8 stored here
std::string u8data;

// we can create a stringview from the input data
auto sv = utf::make_stringview(u16data.begin(), u16data.end());

// and when converting, we just specify where to write the output
sv.to<utf::utf8>(std::back_inserter(utf8_str));
~~~

## Current status
The library is full-featured and, as far as I know, stable and bug-free.
So I'd say go ahead and use it!

For documentation and more examples of use, see the [wiki](https://github.com/jalfd/utf.hpp/wiki).

*Distributed under the [Boost Software License](http://www.boost.org/LICENSE_1_0.txt).*