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
		inline char* add(const char* pChars, size_t nBytes)
		{
			// TODO test scopedLock vs. compareAndSwap result
			MLScopedLock lock(mLock);
			
			std::copy(pChars, pChars + nBytes, mpNext);
			char* r = mpNext;
			mpNext[nBytes] = 0;
			mpNext += nBytes + 1;	
			
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
		TextFragment() : mLengthInBytes(0), mText(nullptr) { }
		
		// copies the null-terminated character array pointed to by pChars into
		// the text fragment pool and creates a new immutable object based on it. 
		TextFragment(const char* pChars) : mLengthInBytes(strlen(pChars)), mText(theTextFragmentPool().add(pChars, mLengthInBytes)) { }
		
		// this ctor can be used to save the work of counting the length if we have a length already, as with static HashedCharArrays.
		TextFragment(const char* pChars, int len) : mLengthInBytes(len), mText(theTextFragmentPool().add(pChars, mLengthInBytes)) { }
		
		// MLTEST TODO deallocate fragments!
		// maybe in the future we make a distinction between temporary and persistent fragments. 
		// Look at use once apps are running.
		~TextFragment() { }
		
		inline int lengthInBytes() const
		{
			return mLengthInBytes;
		}
		
		inline int lengthInCodePoints() const
		{
			utf::stringview<const char*> sv(mText, mText + mLengthInBytes);
			return sv.codepoints();
		}
		
		const char* getText() const { return mText; }
		
		// TextFragment is assignable but otherwise immutable.
		inline TextFragment& operator=(const TextFragment& b)
		{
			if(this != &b)
			{
				// TODO dispose
				mLengthInBytes = b.mLengthInBytes;
				mText = b.mText;
			}
			return *this;
		}
		
		inline bool operator==(TextFragment b) const
		{
			return compareTextFragmentToChars(*this, b.mText);
		}
		
		inline bool operator!=(TextFragment b) const
		{
			return !(*this == b);
		}
		

		friend inline TextFragment operator+(TextFragment f1, TextFragment f2)
		{
			// TODO this impl does an unneccesary copy to the stack, to keep TextFragment very simple for now.
			int len1 = f1.lengthInBytes();
			int len2 = f2.lengthInBytes();
			int totalLength = len1 + len2 + 1; // f1 + f2 + terminating 0
			char buf[totalLength];
			std::fill(buf, buf+totalLength, 0);
			std::copy(f1.mText, f1.mText + len1, buf);
			std::copy(f2.mText, f2.mText + len2, buf + len1);
			return TextFragment(buf, strlen(buf));
		}
		
		// Compare the TextFragment to a null-terminated character array. 
		//
		friend inline bool compareTextFragmentToChars(TextFragment txf, const char* pCharsB)
		{
			bool r = true;
			
			int len = txf.lengthInBytes(); 
			
			if(len > 0)
			{
				const char* pCharsA = txf.mText;
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
		
		friend inline std::ostream& operator<< (std::ostream& out, const TextFragment & r)
		{
			std::cout << r.mText;
			return out;
		}
		
		inline std::string toString() const { return std::string(mText); }
		
	private:
		int mLengthInBytes;
		char* mText; 
	};
	
	// ----------------------------------------------------------------
	// Text - a placeholder for more features later
	
	typedef TextFragment Text;

		
} // namespace ml

