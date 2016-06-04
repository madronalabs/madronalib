//
//  MLStringUtils.h
//  madronalib
//
//  Created by Randy Jones on 12/3/14.
//
//

#ifndef __MLStringUtils__
#define __MLStringUtils__

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "MLSymbol.h"
#include "../dsp/MLDSP.h"

namespace ml { namespace stringUtils {

// template for case insensitive compares. For case sensitive compares, std::less<K> can be used.
template <class K>
class caseInsensitiveCompare : std::function<bool(K, K)>
{		
public:
	bool operator()(const K& s1, const K& s2) const
	{
		return std::lexicographical_compare
		(s1.begin(), s1.end(), s2.begin(), s2.end(), [](const unsigned char& c1, const unsigned char& c2)
		 {
			 return tolower(c1) < tolower(c2);
		 }); 	
	}
};

// Symbol template specialization for case insensitive compares. 	
// Used by ResourceMap by default.
template <>
class caseInsensitiveCompare<Symbol> : std::function<bool(Symbol, Symbol)>
{
public:
	bool operator()(const Symbol& s1, const Symbol& s2) const
	{
		const char* r1 = s1.getTextFragment().text;
		const char* r2 = s2.getTextFragment().text;
		
		// MLTEST write compare add utf 8 strings / iterator lib
		/*
		return std::lexicographical_compare
		(r1.begin(), r1.end(), r2.begin(), r2.end(), [](const unsigned char& c1, const unsigned char& c2)
		 {
			 return tolower(c1) < tolower(c2);
		 }); 	
		 */
		return s1.id < s2.id;
	}
};
	
	
const std::string stripExtension(const std::string& str);
const std::string getShortName(const std::string& str);
const std::string getPath(const std::string& str);	
char * spaceStr( int numIndents );
std::vector< std::string > parsePath(const std::string& pathStr);
std::vector< std::string > vectorOfNonsenseWords( int len );
	
} }

#endif /* defined(__MLStringUtils__) */
