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

	int findFirst(const TextFragment& frag, const utf::codepoint_type c);
	int findLast(const TextFragment& frag, const utf::codepoint_type c);

	// Return a new TextFragment consisting of the codepoints from indices start to (end - 1) in the input frag.
	TextFragment subText(const TextFragment& frag, int start, int end);
	
	// Return the prefix of the input frag as a new TextFragment, stripping the last dot and any codepoints after it. 
	TextFragment stripExtension(const TextFragment& frag);
	
	// If the input fragment contains a slash, return a new TextFragment containing any characters 
	// after the final slash. Else return the input.
	TextFragment getShortName(const TextFragment& frag);

	// Return a new TextFragment containing any characters up to a final slash. 
	TextFragment getPath(const TextFragment& frag);
	
	bool beginsWith (TextFragment fa, TextFragment fb);
	bool endsWith (TextFragment fa, TextFragment fb);

	std::vector< std::string > parsePath(const std::string& pathStr);
	std::vector< std::string > vectorOfNonsenseWords( int len );
	
	char * spaceStr( int numIndents );
	
} }

#endif /* defined(__MLStringUtils__) */
