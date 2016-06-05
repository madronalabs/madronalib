//
//  MLTextFragment.h
//  madronalib
//
//  Created by Randy Jones on 5/21/16.
//

#pragma once

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
			return r;
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
		TextFragment(const char* pChars) : length(strlen(pChars)), text(theTextFragmentPool().add(pChars, length)){}
		
		const int length;
		const char* text; 
	};
	
	inline std::ostream& operator<< (std::ostream& out, const TextFragment & r)
	{
		std::cout << r.text << "(" << r.length << ")"; // MLTEST
		return out;
	}
	
	// Compare the TextFragment to a null-terminated character array. 
	//
	inline bool compareTextFragmentToChars(TextFragment txf, const char* pCharsB)
	{
		bool r = true;
		
		int len = txf.length; 

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