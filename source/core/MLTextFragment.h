//
//  MLTextFragment.h
//  madronalib
//
//  Created by Randy Jones on 5/21/16.
//
//

#pragma once

#include "../DSP/MLDSPMath.h"

static const int kBytesPerTextChunkBits = 4; // must be <= 4 to force SSE alignment
static const int kBytesPerTextChunk = 1 << kBytesPerTextChunkBits;
static const int kPoolSizeBits = 17; // 1 M 
static const int kPoolSize = 1 << kPoolSizeBits; 

namespace ml
{
	class TextFragmentPool
	{
		inline char* alignData(const char* p)
		{
			const int kCharsPerSSEVector = 16;
			uintptr_t pM = (uintptr_t)p;
			pM += (uintptr_t)(kCharsPerSSEVector - 1);
			pM &= (~(kCharsPerSSEVector - 1));	
			return(char*)pM;
		} 
		
		inline size_t chunkSizeToContain(size_t sizeInBytes, size_t chunkSizeBits)
		{
			int sizeMask = (1 << chunkSizeBits) - 1;
			return (sizeInBytes + sizeMask) & (~sizeMask);
		}

	public:

		TextFragmentPool() : mpData(0), mpNext(0)
		{
			mpData = new char[kPoolSize];
			mpDataAligned = alignData(mpData);			
			mpNext = mpDataAligned;
		}
		
		~TextFragmentPool()
		{
			delete mpData;	
		}
		
		inline const char* add(const char* pChars, size_t lengthInBytes)
		{
			// TODO test scopedLock vs. compareAndSwap result
			MLScopedLock lock(mLock);
			
			size_t len = chunkSizeToContain(lengthInBytes, kBytesPerTextChunkBits);
			std::copy(pChars, pChars + lengthInBytes, mpNext);
			
			char* r = mpNext;
			mpNext += len;			
			return r;
		}
		
		// MLTEST
		inline void dump()
		{
			size_t len = mpNext - mpDataAligned;
			size_t frags = len / kBytesPerTextChunk;
			
			for(int i=0; i < frags; ++i)
			{
				for(int n=0; n < kBytesPerTextChunk; ++n)
				{
				std::cout << mpDataAligned[i*kBytesPerTextChunk + n];
				}
				std::cout << "\n";
			}
		}
		
		char * mpData;
		char * mpDataAligned;
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
		// copies the null-terminated characters pointed to by pChars into
		// the text fragment pool and creates a new immutable object based on it. 
		TextFragment(const char* pChars) : length(strlen(pChars)), text(theTextFragmentPool().add(pChars, length)){}
		
		const int length;
		const char* text; // guaranteed 16-byte aligned
	};
	
	inline std::ostream& operator<< (std::ostream& out, const TextFragment & r)
	{
		std::cout << r.text << "(" << r.length << ")";
		return out;
	}
	
	inline bool compareTextFragmentToChars(TextFragment txf, const char* pChars)
	{
		int len = txf.length; 
		if(len > 0)
		{
			// TODO use aligned text
			for(int n=0; n<len; ++n)
			{
				if(txf.text[n] != pChars[n])
				{
					return false;
				}
			}
			return true;
		}
		else
		{
			return (pChars[0] == 0);
		}
	}
} // namespace ml