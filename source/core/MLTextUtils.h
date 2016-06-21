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

// #include "MLTextFragment.h"
#include "MLSymbol.h"
#include "../dsp/MLDSP.h"
#include "utf.hpp/utf.hpp"

namespace ml { namespace textUtils {

	bool isDigit(char32_t c);
	char * spaceStr( int numIndents );	
	TextFragment positiveIntToDigits(int i);
	
	int digitsToPositiveInt(const char32_t* p);
	const char *naturalNumberToDigits(int value, char* pDest);
	
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
	
	// ----------------------------------------------------------------
	// Symbol utilities
	
	Symbol addFinalNumber(Symbol sym, int n);
	Symbol stripFinalNumber(Symbol sym);
	int getFinalNumber(Symbol sym);	
	std::vector< Symbol > parsePath(const char* pathStr);

	std::vector<Symbol> vectorOfNonsenseSymbols( int len );
	
	// ----------------------------------------------------------------
#pragma mark NameMaker
	// a utility to make many short, unique, human-readable names when they are needed. 
	
	class NameMaker
	{
		static const int maxLen = 64;
	public:
		NameMaker() : index(0) {};
		~NameMaker() {};
		
		// return the next name as a symbol, having added it to the symbol table. 
		const Symbol nextName();
		
	private:
		int index;
		char buf[maxLen];
		
	};
	
	
} } // ml::textUtils

#endif /* defined(__MLStringUtils__) */
