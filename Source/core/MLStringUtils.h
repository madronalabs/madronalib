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
#include "MLDSP.h"

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

// MLSymbol template specialization for case insensitive compares. 	
template <>
class caseInsensitiveCompare<MLSymbol> : std::function<bool(MLSymbol, MLSymbol)>
{
public:
	bool operator()(const MLSymbol& s1, const MLSymbol& s2) const
	{
		const std::string& r1 = s1.getString();
		const std::string& r2 = s2.getString();
		return std::lexicographical_compare
		(r1.begin(), r1.end(), r2.begin(), r2.end(), [](const unsigned char& c1, const unsigned char& c2)
		 {
			 return tolower(c1) < tolower(c2);
		 }); 	
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
