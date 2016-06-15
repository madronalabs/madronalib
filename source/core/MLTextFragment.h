//
//  MLTextFragment.h
//  madronalib
//
//  Created by Randy Jones on 5/21/16.
//

#pragma once

#include "MLLocks.h"

static const int kPoolSizeBits = 17; // 1 M 
static const int kPoolSize = 1 << kPoolSizeBits; 

namespace ml
{
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
	
	class TextFragment
	{
	public:
		// copies the null-terminated character array pointed to by pChars into
		// the text fragment pool and creates a new immutable object based on it. 
		TextFragment(const char* pChars) : lengthInBytes(strlen(pChars)), text(theTextFragmentPool().add(pChars, lengthInBytes)) { }
		
		// this ctor can be used to save the work of counting the length if we have a length already.
		TextFragment(const char* pChars, int len) : lengthInBytes(len), text(theTextFragmentPool().add(pChars, lengthInBytes)) { }
		
		// TODO maybe do a little deallocation for recently used fragments.
		// maybe in the future we make a distinction between temporary and persistent fragments. 
		// Look at use once apps are running.
		~TextFragment() { }
		
		const int lengthInBytes;
		const char* text; 
	};

	inline TextFragment operator+(TextFragment f1, TextFragment f2)
	{
		// this impl does an unneccesary copy, to keep TextFragment very simple for now.
		
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
	
} // namespace ml