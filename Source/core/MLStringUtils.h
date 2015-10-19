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
	template <class K>
	struct caselessCompare : std::function<bool(K, K)>
	{		
		bool operator()(const K& s1, const K& s2) const
		{
			bool r = std::lexicographical_compare
			(s1.begin(), s1.end(), s2.begin(), s2.end(), [](const unsigned char& c1, const unsigned char& c2)
			 {
				 return tolower(c1) < tolower(c2);
			 }); 	
			
			//std::cout << s1 << " / " << s2 << " : " << r << "\n";
			return r;
		}
	};
	
	/*
	template <>
	struct caselessCompare : std::function<bool(MLSymbol, MLSymbol)>
	{		
		bool operator()(const MLSymbol& s1, const MLSymbol& s2) const
		{
			
		//	bool r = std::lexicographical_compare
		//	(s1.begin(), s1.end(), s2.begin(), s2.end(), [](const unsigned char& c1, const unsigned char& c2)
		//	 {
		//		 return tolower(c1) < tolower(c2);
		//	 }); 	
		
			std::cout << s1 << " / " << s2 << " : " << r << "\n";
			return r;
		}
	};
	*/
	
	const std::string stripExtension(const std::string& str);
	const std::string getShortName(const std::string& str);
	const std::string getPath(const std::string& str);	
	char * spaceStr( int numIndents );
	std::vector< std::string > parsePath(const std::string& pathStr);
	std::vector< std::string > vectorOfNonsenseWords( int len );
} }

#endif /* defined(__MLStringUtils__) */
