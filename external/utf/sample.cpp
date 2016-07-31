
//          Copyright Jesper Dam 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <vector>
#include <iostream>
#include <iterator>
#include <string>
#include "utf.hpp"

int main() {
    // assume we have a plain UTF-8 string
    const char str[] = "hello world";
    // create a stringview over it
    utf::stringview<const char*> sv(str, str + sizeof(str));
    
    // print out a few statistics
    std::cout << "number of code units: " << sv.codeunits() << '\n';
    std::cout << "byte length: " << sv.bytes() << '\n';
    std::cout << "byte length as utf16: " << sv.bytes<utf::utf16>() << '\n';
    
    // encode as UTF-16 into a pre-allocated buffer
    std::vector<char16_t> buf(sv.codeunits<utf::utf16>(), 0);
    sv.to<utf::utf16>(buf.begin());
    
    // contents of the destination buffer
    std::cout << "utf16 code units: ";
    for (std::vector<char16_t>::iterator c = buf.begin(); c != buf.end(); ++c) {
        std::cout << std::hex << (uint16_t)*c << ' ';
    }
    std::cout << '\n';
    
    // make a stringview from something other than C-style strings
    std::string s = "hello world";
    utf::make_stringview(s.begin(), s.end());

    // Make a UTF-16 or UTF-32 string, depending on compiler
    const wchar_t* str2 = L"hell\xf8 world";
    // deduce the correct encoding and create an appropriate stringview
    auto sv2 = utf::make_stringview(str2, str2 + 11);
    
    // encode as UTF-8, allocating the buffer on the fly
    std::string v;
    sv2.to<utf::utf8>(std::back_inserter(v));
    std::cout << v << '\n';
    
    // iterate over the codepoints represented by an UTF-16 string
    auto svx = utf::make_stringview(buf.begin(), buf.end());
    for (utf::codepoint_iterator<std::vector<char16_t>::iterator> it = svx.begin(); it != svx.end(); ++it) {
        std::cout << std::hex << (unsigned)*it << ' ';
    }
    std::cout << '\n';
}