//
//  MLText.h
//  madronalib
//
//  Created by Randy Jones on 5/21/16.
//

#pragma once

#include "MLLocks.h"
#include "utf.hpp/utf.hpp"

static const int kPoolSizeBits = 17; // 1 M 
static const int kPoolSize = 1 << kPoolSizeBits; 

namespace ml
{
	// ----------------------------------------------------------------
	// TextFragmentPool
	
	class TextFragmentPool
	{
	public:
		TextFragmentPool() : mpData(0), mpNext(0)
		{
			mpData = new char[kPoolSize];
			mpNext = mpData;
		}
		
		~TextFragmentPool()
		{
			delete mpData;	
		}
		
		// Add a new null-terminated text fragment to the pool. Return its starting address. 
		inline const char* add(const char* pChars, size_t lengthInBytes)
		{
			// TODO test scopedLock vs. compareAndSwap result
			MLScopedLock lock(mLock);
			
			std::copy(pChars, pChars + lengthInBytes, mpNext);
			char* r = mpNext;
			mpNext[lengthInBytes] = 0;
			mpNext += lengthInBytes + 1;	
			
			if(mpNext >= mpData + kPoolSize)
			{
				// TODO something. Try allocating more pool space.
			}
			
			return r;
		}

		inline void dump()
		{
			char * p = mpData;
			std::cout << "text fragments: -----------------\n";
			int n = 0;
			while(p < mpNext)
			{
				int len = strlen(p);
				std::cout << p << "(" << std::dec << len << ")\n";
				p += len + 1;
				n++;
			}
			std::cout << "total: " << n << " text fragments, " << (mpNext - mpData) << " bytes.\n";

		}

		char * mpData;
		char * mpNext;
		MLSpinLock mLock;
	};
	
	inline TextFragmentPool& theTextFragmentPool()
	{
		static std::unique_ptr<TextFragmentPool> t (new TextFragmentPool());
		return *t;
	}
	
	// ----------------------------------------------------------------
	// TextFragment

	class TextFragment;
	inline bool compareTextFragmentToChars(TextFragment txf, const char* pCharsB);
	
	class TextFragment
	{
	public:
		TextFragment() : lengthInBytes(0), text(nullptr) { }
		
		// copies the null-terminated character array pointed to by pChars into
		// the text fragment pool and creates a new immutable object based on it. 
		TextFragment(const char* pChars) : lengthInBytes(strlen(pChars)), text(theTextFragmentPool().add(pChars, lengthInBytes)) { }
		
		// this ctor can be used to save the work of counting the length if we have a length already.
		TextFragment(const char* pChars, int len) : lengthInBytes(len), text(theTextFragmentPool().add(pChars, lengthInBytes)) { }
		
		// MLTEST TODO deallocate fragments!
		// maybe in the future we make a distinction between temporary and persistent fragments. 
		// Look at use once apps are running.
		~TextFragment() { }
		
		int lengthInCodePoints() const
		{
			utf::stringview<const char*> sv(text, text + lengthInBytes);
			return sv.codepoints();
		}
		
		inline bool operator==(TextFragment b) const
		{
			return compareTextFragmentToChars(*this, b.text);
		}
		
		inline bool operator!=(TextFragment b) const
		{
			return !(*this == b);
		}
		
		const int lengthInBytes;
		const char* text; 
	};

	inline TextFragment operator+(TextFragment f1, TextFragment f2)
	{
		// TODO this impl does an unneccesary copy to the stack, to keep TextFragment very simple for now.
		
		int len1 = f1.lengthInBytes;
		int len2 = f2.lengthInBytes;
		int totalLength = len1 + len2 + 1; // f1 + f2 + terminating 0
		char buf[totalLength];
		std::fill(buf, buf+totalLength, 0);
		std::copy(f1.text, f1.text + len1, buf);
		std::copy(f2.text, f2.text + len2, buf + len1);
		return TextFragment(buf, strlen(buf));
	}

	inline std::ostream& operator<< (std::ostream& out, const TextFragment & r)
	{
		std::cout << r.text;
		return out;
	}
	
	// Compare the TextFragment to a null-terminated character array. 
	//
	inline bool compareTextFragmentToChars(TextFragment txf, const char* pCharsB)
	{
		bool r = true;
		
		int len = txf.lengthInBytes; 

		if(len > 0)
		{
			const char* pCharsA = txf.text;
			for(int n=0; n<len; ++n)
			{
				char ca = pCharsA[n];
				char cb = pCharsB[n];
				
				// We know our text fragment's length so we only need to test for pCharsB ending.
				if((cb == 0) || (cb != ca))
				{
					r = false;
					break;
				}
			}
		}
		else
		{
			r = (pCharsB[0] == 0);
		}
		return r;		
	}
	
	// ----------------------------------------------------------------
	// Text

	class Text
	{
	public:		
		Text() : delimiter(' ') {}
		
		// TODO parse into fragments using spaces or other delimiter
		Text(const char* inputStr, char dc = ' ') : delimiter(dc) { parse(inputStr); }
		
		inline int lengthInFragments() const
		{
			return fragments.size();
		}
		
		inline int lengthInBytes() const
		{
			int r = 0;
			for(auto f : fragments)
			{
				r += f.lengthInBytes;
			}
			
			// add delimiters
			r += fragments.size() - 1;
			return r;
		}
		
		inline int lengthInCodePoints() const
		{
			int r = 0;
			for(auto f : fragments)
			{
				r += f.lengthInCodePoints();
			}
			
			// add delimiters
			r += fragments.size() - 1;
			return r;
		}
		
		inline void parse(const char* inputStr)
		{
			fragments.clear();
			
			int inputStrBytes = strlen(inputStr);	
			
			// MLTEST TODO don't make this buffer
			char UTF8buf[inputStrBytes];
			std::fill(UTF8buf, UTF8buf+inputStrBytes, 0);
			
			char* beginPoint = UTF8buf;
			char* beginSymbol = UTF8buf;
			char* endPoint = UTF8buf;
			
			auto first = utf::codepoint_iterator<const char*>(inputStr);		
			auto it = first;
			int pointSizeAsUTF8;
			char c = 0;
			do
			{
				do
				{
					// write the codepoint as UTF-8 to the buffer and advance pb
					endPoint = utf::internal::utf_traits<utf::utf8>::encode(*it, beginPoint);				
					pointSizeAsUTF8 = endPoint - beginPoint;
					
					// if we have a one-byte character, see if it's the delimiter
					c = (pointSizeAsUTF8 == 1) ? *it : -1;
					beginPoint = endPoint;
					++it;
				}	
				while((c != delimiter) && (c != 0));
				
				// add the new fragment
				int newSymbolBytes = (endPoint - beginSymbol) - 1;				
				add(TextFragment(beginSymbol, newSymbolBytes));
				beginSymbol = endPoint;
			}
			while(c != 0);
		}
			
		inline void add(TextFragment f)
		{
			fragments.emplace_back(f);
		}
		
		std::vector<ml::TextFragment> fragments; 
		const char delimiter;
	};

	inline Text operator+(Text t1, Text t2)
	{
		Text r;
		for(TextFragment f : t1.fragments)
		{
			r.add(f);
		}
		for(TextFragment f : t2.fragments)
		{
			r.add(f);
		}
		return r;
	}
	
	inline std::ostream& operator<< (std::ostream& out, const Text & t)
	{
		int c = t.fragments.size();
		for(int n=0; n<c; ++n)
		{
			std::cout << t.fragments[n];
			if(n < c-1)
			{
				std::cout << (char)t.delimiter;
			}
		}
		return out;
	}
	
	inline std::string makeStdString (const Text t)
	{
		std::string r;
		int c = t.fragments.size();
		for(int n=0; n<c; ++n)
		{
			r += (t.fragments[n].text);
			if(n < c-1)
			{
				r += (char)t.delimiter;
			}
		}
		return r;
	}

} // namespace ml