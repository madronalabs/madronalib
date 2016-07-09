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
#include "utf.hpp/utf.hpp"

#include "../DSP/MLDSPGens.h" // for RandomSource TODO replace

namespace ml { namespace textUtils {

using namespace utf;

	bool isDigit(char32_t c);
	char * spaceStr( int numIndents );	
	int digitsToPositiveInt(const char32_t* p);
	const char *naturalNumberToDigits(int value, char* pDest);
	
	// ----------------------------------------------------------------
	// TextFragment utilities

	TextFragment positiveIntToDigits(int i);	
	
	int findFirst(const TextFragment& frag, const utf::codepoint_type c);
	int findLast(const TextFragment& frag, const utf::codepoint_type c);

	// Return a new TextFragment consisting of the codepoints from indices start to (end - 1) in the input frag.
	TextFragment subText(const TextFragment& frag, int start, int end);
	
	// Return the prefix of the input frag as a new TextFragment, stripping the last dot and any codepoints after it. 
	TextFragment stripFileExtension(const TextFragment& frag);	
	
	// If the input fragment contains a slash, return a new TextFragment containing any characters 
	// after the final slash. Else return the input.
	TextFragment getShortFileName(const TextFragment& frag);

	// Return a new TextFragment containing any characters up to a final slash. 
	TextFragment getPath(const TextFragment& frag);

	bool isASCII(codepoint_type c);
	bool isLatin(codepoint_type c);
	bool isSpace(codepoint_type c);
	bool isCJK(codepoint_type c);
		
	Symbol bestScriptForTextFragment(const TextFragment& frag);
	
	// ----------------------------------------------------------------
	// Symbol utilities
	
	Symbol addFinalNumber(Symbol sym, int n);
	Symbol stripFinalNumber(Symbol sym);
	int getFinalNumber(Symbol sym);	

	Symbol stripFinalCharacter(Symbol sym);
	
	std::vector< Symbol > vectorOfNonsenseSymbols( int len );

	// ----------------------------------------------------------------
	// NameMaker
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
