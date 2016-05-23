//
//  MLTextFragment.h
//  madronalib
//
//  Created by Randy Jones on 5/21/16.
//
//

#pragma once

static const int kCharsPerTextChunkBits = 4;
static const int kCharsPerTextChunk = 1 << kCharsPerTextChunkBits;
static const int kCharsPerTextChunkMask = kCharsPerTextChunk - 1;
static const int kPoolSizeBits = 17; // 1 M 
static const int kPoolSize = 1 << kPoolSizeBits; 

namespace ml
{
	class TextFragmentPool
	{
	public:

		inline char* alignData(const char* p)
		{
			const int kCharsPerSSEVector = 16;
			uintptr_t pM = (uintptr_t)p;
			pM += (uintptr_t)(kCharsPerSSEVector - 1);
			pM &= (~(kCharsPerSSEVector - 1));	
			return(char*)pM;
		} 

		TextFragmentPool() : pData(0), pNext(0)
		{
			pData = new char[kPoolSize];
			pDataAligned = alignData(pData);			
			pNext = pDataAligned;
		}
		
		~TextFragmentPool()
		{
			delete pData;	
		}
		
		const char* add(const char* pChars, size_t lengthInBytes)
		{
			// TODO test scopedLock vs. compareAndSwap result
			MLScopedLock lock(mLock);
			
			// get smallest chunk size that will hold lengthInBytes
			size_t len = (lengthInBytes + kCharsPerTextChunkMask) & (~kCharsPerTextChunkMask);
			
			// copy the data
			for(int n=0; n<len; ++n)
			{
				pNext[n] = pChars[n];
			}
			
			char* r = pNext;
			pNext += len;
			
			// MLTEST
			// std::cout << std::hex << (unsigned long)pNext << std::dec << "\n";
			
			return r;
		}
		
		char * pData;
		char * pDataAligned;
		char * pNext;
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
		// copies the null-terminated text fragment pointed to by pChars into
		// the text fragment pool and creates a new immutable object based on it. 
		TextFragment(const char* pChars) : mLength(strlen(pChars)), mpText(theTextFragmentPool().add(pChars, mLength)){}
		
		const size_t mLength;
		const char* mpText;
	};
	
	inline std::ostream& operator<< (std::ostream& out, const TextFragment & r)
	{
		std::cout << r.mpText;
		return out;
	}

	
} // namespace ml