//
//  MLStringCompare.h
//  MadronaLib
//
//  Created by Randy Jones on 4/25/14.
//
//

// types and functors for supporting core ML classes.

#ifndef MLStringCompare_h
#define MLStringCompare_h

struct MLStringCompareFn : std::binary_function<std::string, std::string, bool>
{
    struct caseInsensitiveCompare : public std::binary_function<unsigned char,unsigned char,bool>
    {
        bool operator() (const unsigned char& c1, const unsigned char& c2) const
        {
            return tolower (c1) < tolower (c2);
        }
    };
    bool operator() (const std::string & s1, const std::string & s2) const
    {
        return std::lexicographical_compare
        (s1.begin(), s1.end(),   // source range
         s2.begin(), s2.end(),   // dest range
         caseInsensitiveCompare ());  // comparison
    }
};


#endif
