//
//  MLTextUtils.h
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
#include "utf.hpp"

namespace ml { namespace textUtils {

	const size_t countCodePoints(const TextFragment& txt);
	
	
	
const TextFragment stripExtension(const TextFragment& str);
	
const std::string getShortName(const std::string& str);
const std::string getPath(const std::string& str);	
char * spaceStr( int numIndents );
std::vector< std::string > parsePath(const std::string& pathStr);
std::vector< std::string > vectorOfNonsenseWords( int len );
	
} }

#endif /* defined(__MLStringUtils__) */
