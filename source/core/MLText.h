//
//  MLText.h
//  madronalib
//
//  Created by Randy Jones on 5/21/16.
//

#pragma once

#include "MLLocks.h"
#include "utf.hpp/utf.hpp"

static const int kPoolSizeBits = 20; // 1 M 
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
			
			if(mpNext >= mpData + kPoolSize)
			{
				// TODO something. Try allocating more pool space.
				std::cout << "TextFragmentPool::add: pool full! (size = " << size_t(mpNext - mpData) << ") \n";
				return mpData;
			}
			
			std::copy(pChars, pChars + nBytes, mpNext);
			char* r = mpNext;
			mpNext[nBytes] = 0;
			mpNext += nBytes + 1;	
			
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
		
		inline size_t getSize()
		{
			return static_cast< size_t >(mpNext - mpData);
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
	inline bool compareSizedCharArrays(const char* pA, int lenA, const char* pB, int lenB);
	
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
		
		explicit operator bool() const { return mLengthInBytes > 0; }
		
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
		
		bool beginsWith(const TextFragment& fb) const
		{
			int lenA = lengthInBytes();
			int lenB = fb.lengthInBytes();			
			if(lenB > lenA) return false;
			for(int i=0; i<lenB; ++i)
			{
				if(getText()[i] != fb.getText()[i])
				{
					return false;
				}
			}			
			return true;
		}
		
		bool endsWith(const TextFragment& fb) const
		{
			int lenA = lengthInBytes();
			int lenB = fb.lengthInBytes();			
			if(lenB > lenA) return false;
			for(int i=0; i<lenB; ++i)
			{
				if(getText()[lenA - lenB + i] != fb.getText()[i])
				{
					return false;
				}
			}			
			return true;
		}
		
		// deprecated! MLTEST
		inline std::string toString() const { return std::string(mText); }

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
		
	private:
		int mLengthInBytes;
		char* mText; 
	};

	// TODO made operator== a free function-	do likewise for other classes
	
	inline bool operator==(const TextFragment& a, const TextFragment& b)
	{
		return compareSizedCharArrays(a.getText(), a.lengthInBytes(), b.getText(), b.lengthInBytes()); 
	}
	
	inline bool operator!=(TextFragment a, TextFragment b)
	{
		return !(a == b);
	}
	
	inline std::ostream& operator<< (std::ostream& out, const TextFragment & r)
	{
		std::cout << r.getText();
		return out;
	}
	
	inline bool compareSizedCharArrays(const char* pA, int lenA, const char* pB, int lenB)
	{
		if(lenA != lenB) return false;
		if((lenA == 0) && (lenB == 0)) return true;
		
		for(int n=0; n<lenA; ++n)
		{
			if(pA[n] != pB[n])
			{
				return false;
			}
		}
		
		return true;		
	}

	// ----------------------------------------------------------------
	// Text - a placeholder for more features later
	
	typedef TextFragment Text;

		
} // namespace ml

