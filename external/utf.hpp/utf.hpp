
//          Copyright Jesper Dam 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NP_UTF_HPP
#define NP_UTF_HPP

#include <cstddef>
#include <cassert>
#include <stdint.h>
#include <iterator>
#include <algorithm>

#ifdef UTFHPP_NO_CPP11
namespace utf {
    typedef uint16_t char16_t;
    typedef uint32_t char32_t;
}
#endif

namespace utf {
    struct utf8;
    struct utf16; // uses native endianness
    struct utf32;

    typedef char32_t codepoint_type;

    namespace internal {
        template <size_t S>
        struct encoding_for_size;

        template <>
        struct encoding_for_size<1> {
            typedef utf8 type;
        };
        template <>
        struct encoding_for_size<2> {
            typedef utf16 type;
        };
        template <>
        struct encoding_for_size<4> {
            typedef utf32 type;
        };
        template <typename T>
        struct native_encoding {
            typedef typename encoding_for_size<sizeof(T)>::type type;
        };

        inline bool validate_codepoint(codepoint_type c) {
            if (c < 0xd800) { return true; }
            if (c < 0xe000) { return false; }
            if (c < 0x110000) { return true; }
            
            return false;
        }

        template <typename E>
        struct utf_traits;

        template <>
        struct utf_traits<utf8> {
            typedef char codeunit_type;
            static size_t read_length(codeunit_type c) {
                if ((c & 0x80) == 0x00) { return 1; }
                if ((c & 0xe0) == 0xc0) { return 2; }
                if ((c & 0xf0) == 0xe0) { return 3; }
                if ((c & 0xf8) == 0xf0) { return 4; }

                return 1;
            }
            static size_t write_length(codepoint_type c) {
                if (c <= 0x7f) { return 1; }
                if (c < 0x0800) { return 2; }
                if (c < 0xd800) { return 3; }
                if (c < 0xe000) { return 0; }
                if (c < 0x010000) { return 3; }
                if (c < 0x110000) { return 4; }

                return 0;
            }

            // responsible only for validating the utf8 encoded subsequence, not the codepoint it maps to
            template <typename Iter>
            static bool validate(Iter first, Iter last) {
                size_t len = last - first;
                unsigned char lead = (unsigned char)*first;
                switch (len) {
                    case 1:
                        if ((lead & 0x80) != 0x00) { return false; }
                        break;
                    case 2:
                        if ((lead & 0xe0) != 0xc0) { return false; }
                        break;
                    case 3:
                        if ((lead & 0xf0) != 0xe0) { return false; }
                        break;
                    case 4:
                        if ((lead & 0xf8) != 0xf0) { return false; }
                        break;
                    default:
                        return false;
                }

                for (size_t i = 1; i < len; ++i) {
                    unsigned char c = static_cast<unsigned char>(first[i]);
                    if ((c & 0xc0) != 0x80) { return false; }
                }

                // check for overlong encodings
                switch (len) {
                    case 2:
                        if (((unsigned char)*first) <= 0xc1) { return false; }
                        break;
                    case 3:
                        if (((unsigned char)*first) == 0xe0) { return false; }
                        break;
                    case 4:
                        if (((unsigned char)*first) == 0xf0
                            && ((unsigned char)first[1]) < 0x90) { return false; }
                        break;
                    default: break;
                }

                return true;
            }

            template <typename OutIt>
            static OutIt encode(codepoint_type c, OutIt dest) {

                size_t len = write_length(c);

                unsigned char res[4] = {};

                // loop to catch remaining
                for (size_t i = len; i != 1; --i) {
                    // select lower 6 bits
                    res[i-1] = (c & 0x3f) | 0x80;
                    c = c >> 6;
                }

                // switch on first byte
                switch (len) {
                    case 1: res[0] = c; break;
                    case 2: res[0] = c | 0xc0; break;
                    case 3: res[0] = c | 0xe0; break;
                    case 4: res[0] = c | 0xf0; break;
                    default:
                        assert(false && "bad utf8 codeunit");
                };

                for (size_t i = 0; i < len; ++i) {
                    *dest = res[i];
                    ++dest;
                }

                return dest;
            }

            template <typename Iter>
            static codepoint_type decode(Iter c) {
                size_t len = read_length(static_cast<codeunit_type>(*c));

                codepoint_type res = 0;
                // switch on first byte
                switch (len) {
                    case 1: res = *c; break;
                    case 2: res = *c & 0x1f; break;
                    case 3: res = *c & 0x0f; break;
                    case 4: res = *c & 0x07; break;
                    default:
                        assert(false && "bad utf8 codeunit");
                };

                // then loop to catch remaining?
                for (size_t i = 1; i < len; ++i) {
                    res = (res << 6) | (c[i] & 0x3f);
                }
                return res;
            }
        };

        template <>
        struct utf_traits<utf16> {
            typedef char16_t codeunit_type;
            static size_t read_length(codeunit_type c) {
                if (c < 0xd800) { return 1; }
                if (c < 0xdc00) { return 2; }
                return 1;
            }
            static size_t write_length(codepoint_type c) {
                if (c < 0xd800) { return 1; }
                if (c < 0xe000) { return 0; }
                if (c < 0x010000) { return 1; }
                if (c < 0x110000) { return 2; }
                
                return 0;
            }

            template <typename Iter>
            static bool validate(Iter first, Iter last) {
                size_t len = last - first;
                switch (len) {
                    case 1:
                    {
                        char16_t lead = *first;
                        if (lead >= 0xd800 && lead < 0xe000) { return false; }
                        break;
                    }
                    case 2:
                    {
                        char16_t lead = first[0];
                        char16_t trail = first[1];
                        if (lead < 0xd800 || lead >= 0xdc00) { return false; }
                        if (trail < 0xdc00 || trail >= 0xe000) { return false; }
                        break;
                    }
                    default:
                        return false;
                }
                return true;
            }
            template <typename OutIt>
            static OutIt encode(codepoint_type c, OutIt dest) {
                size_t len = write_length(c);
                
                if (len == 1) {
                    *dest = c;
                    ++dest;
                    return dest;
                }

                // 20-bit intermediate value
                size_t tmp = c - 0x10000;
                
                *dest = static_cast<char16_t>((tmp >> 10) + 0xd800);
                ++dest;
                *dest = static_cast<char16_t>((tmp & 0x03ff) + 0xdc00);
                ++dest;
                return dest;
            }

            template <typename Iter>
            static codepoint_type decode(Iter c) {
                size_t len = read_length(*c);
                
                char16_t lead = *c;
                if (len == 1) {
                    return lead;
                }

                codepoint_type res = 0;
                // 10 most significant bits
                res = (lead - 0xd800) << 10;
                char16_t trail = c[1];
                // 10 least significant bits
                res +=  (trail - 0xdc00);
                return res + 0x10000;
            }
        };

        template <>
        struct utf_traits<utf32> {
            typedef char32_t codeunit_type;
            static size_t read_length(codeunit_type c) { return 1; }
            static size_t write_length(codepoint_type c) {
                if (c < 0xd800) { return 1; }
                if (c < 0xe000) { return 0; }
                if (c < 0x110000) { return 1; }
                return 0;
            }

            template <typename T>
            static bool validate(const T* first, const T* last) {
                // actually looking at the cp value is done by free validate function.
                return last - first == 1;
            }

            template <typename OutIt>
            static OutIt encode(codepoint_type c, OutIt dest) {
                *dest = c;
                ++dest;
                return dest;
            }
            template <typename Iter>
            static codepoint_type decode(Iter c) {
                return *c;
            }
        };
    }
    
    template <typename It>
    class codepoint_iterator : public std::iterator<std::input_iterator_tag
    , const codepoint_type, ptrdiff_t
    , const codepoint_type*
    , const codepoint_type&> {
        typedef typename std::iterator_traits<It>::value_type codeunit_type;
        typedef typename internal::native_encoding<codeunit_type>::type encoding;
        typedef internal::utf_traits<encoding> traits_type;
        codepoint_type val;
        It pos;

    public:
        explicit codepoint_iterator() : val(), pos() {}
        explicit codepoint_iterator(It pos) : val(), pos(pos) {}
        codepoint_iterator(const codepoint_iterator& it) : val(it.val), pos(it.pos) {}

        typename std::iterator_traits<codepoint_iterator>::reference operator*() {
            val = traits_type::decode(pos);
            return val;
        }
        typename std::iterator_traits<codepoint_iterator>::pointer operator->() const { return (&**this); }
        codepoint_iterator& operator++() {
            It next = pos + traits_type::read_length(*pos);
            pos = next;
            return *this;
		}
        codepoint_iterator operator++(int) {
            codepoint_type tmp = *this;
            ++(*this);
            return tmp;
        }
        friend bool operator != (codepoint_iterator lhs, codepoint_iterator rhs) { return lhs.pos != rhs.pos; }
        friend bool operator == (codepoint_iterator lhs, codepoint_iterator rhs) { return !(lhs != rhs); }
    };

    template <typename Iter, typename E = typename internal::native_encoding<typename std::iterator_traits<Iter>::value_type>::type>
    struct stringview {
        typedef typename std::iterator_traits<Iter>::value_type codeunit_type;

        stringview(const Iter first, const Iter last)
        : first(first), last(last) {}

        stringview()
        : first(), last() {}

        Iter raw_begin() const { return first; }
        Iter raw_end() const { return last; }

        codepoint_iterator<Iter> begin() const { return codepoint_iterator<Iter>(first); }
        codepoint_iterator<Iter> end() const { return codepoint_iterator<Iter>(last); }
        
        bool validate() const {
            typedef internal::utf_traits<E> traits_t;
            for (Iter it = first;  it < last;) {
                size_t len = traits_t::read_length(*it);
                if (last - it < static_cast<ptrdiff_t>(len)) {
                    return false;
                }
                if (!traits_t::validate(it, it + len)) {
                    return false;
                }
                codepoint_type cp = traits_t::decode(it);
                if (!internal::validate_codepoint(cp)) {
                    return false;
                }
                it += len;
            }
            return true;
        }

        bool empty() const {
            return begin() == end();
        }
        size_t codepoints() const {
            return std::distance(begin(), end());
        }

        size_t bytes() const {
            return codeunits() * sizeof(typename internal::utf_traits<E>::codeunit_type);
        }

        // length in source encoding
        template <typename EDest>
        size_t bytes() const {
            return codeunits<EDest>() * sizeof(typename internal::utf_traits<EDest>::codeunit_type);
        }

        size_t codeunits() const { return last - first; }

        template <typename EDest>
        size_t codeunits() const {
            size_t cus = 0;
            for (codepoint_iterator<Iter> it = begin(); it != end(); ++it) {
                cus += internal::utf_traits<EDest>::write_length(*it);
            }
            return cus;
        }

        template <typename EDest, typename OutIt>
        OutIt to(OutIt dest) const {
            for (codepoint_iterator<Iter> it = begin(); it != end(); ++it) {
                dest = internal::utf_traits<EDest>::encode(*it, dest);
            }
            return dest;
        }

    private:
        Iter first;
        Iter last;
    };

    template <typename IterL, typename IterR, typename E>
    inline bool operator == (const stringview<IterL, E>& lhs, const stringview<IterR, E>& rhs) {
        return lhs.codepoints() == rhs.codepoints() && std::equal(lhs.raw_begin(), lhs.raw_end(), rhs.raw_begin());
    }
    template <typename IterL, typename IterR, typename E>
    inline bool operator != (const stringview<IterL, E>& lhs, const stringview<IterR, E>& rhs) {
        return !(lhs == rhs);
    }

    template <typename IterL, typename EL, typename IterR, typename ER>
    inline bool operator != (const stringview<IterL, EL>& lhs, const stringview<IterR, ER>& rhs) {
        return lhs.codepoints() == rhs.codepoints() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    template <typename IterL, typename EL, typename IterR, typename ER>
    inline bool operator == (const stringview<IterL, EL>& lhs, const stringview<IterR, ER>& rhs) {
        return !(lhs != rhs);
    }

    // convenience stuff
    template <typename T, size_t N>
    stringview<const T*> make_stringview(T (&arr)[N]) {
        return stringview<const T*>(arr, arr + N);
    }

    template <typename Iter>
    stringview<Iter>
    make_stringview(Iter first, Iter last) {
        return stringview<Iter>(first, last);
    }
}

#endif