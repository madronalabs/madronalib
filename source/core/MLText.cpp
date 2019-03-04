//
//  MLText.cpp
//  madronalib
//
//  Created by Randy Jones on 5/21/16.
//

#include "MLText.h"

namespace ml
{
	TextFragment::TextFragment() noexcept
	{
		mSize = 0;
		mpText = mLocalText;
		nullTerminate();
	}
	
	TextFragment::TextFragment(const char* pChars) noexcept : mSize(strlen(pChars))
	{
		create();
		// a bad alloc will result in this being a null object.
		// copy the input string into local storage
		if(mpText)
		{
			std::copy(pChars, pChars + mSize, mpText);
			nullTerminate();
		}
	}
	
	// this ctor can be used to save the work of counting the length if we have a length already, as with static HashedCharArrays.
	TextFragment::TextFragment(const char* pChars, size_t len) noexcept : mSize(len)
	{
		create();
		if(mpText)
		{
			std::copy(pChars, pChars + mSize, mpText);
			nullTerminate();
		}		
	}
	
	TextFragment::TextFragment(utf::codepoint_type c) noexcept
	{
		if(!utf::internal::validate_codepoint(c))
		{
			c = 0x2639; // sad face
		}
		// all possible codepoints fit into local text
		char* end = utf::internal::utf_traits<utf::utf8>::encode(c, mLocalText);
		mSize = end - mLocalText;
		mpText = mLocalText;	
		nullTerminate();
	}
	
	TextFragment subText(const TextFragment& frag, int start, int end)
	{		
		// this impl does an unneccesary copy, to keep TextFragment very simple for now.
		if(start >= end) return TextFragment();
		
		// we won't know the output fragment size in bytes until iterating the code points. 
		int len = frag.lengthInBytes();
		SmallStackBuffer<char, kShortFragmentSizeInChars> temp(len);
		char* buf = temp.data();
		char* pb = buf;
		
		auto first = utf::codepoint_iterator<const char*>(frag.getText());		
		auto it = first;
		for(int i=0; i<start; ++i)
		{
			++it;
		}
		
		for (int i=0; i<end - start; ++i) 
		{
			// write the codepoint as UTF-8 to the buffer
			if(!utf::internal::validate_codepoint(*it)) return TextFragment();
			pb = utf::internal::utf_traits<utf::utf8>::encode(*it, pb);
			++it;
		}	
		
		return TextFragment(buf, pb - buf);
	}
	
	TextFragment::TextFragment(const TextFragment& a) noexcept
	{
		construct(a.getText(), a.lengthInBytes());
	}
	
	utf::codepoint_iterator<const char*> TextFragment::begin() const
	{
		return utf::codepoint_iterator<const char*>(getText());
	}
	
	utf::codepoint_iterator<const char*> TextFragment::end() const
	{
		return utf::codepoint_iterator<const char*>(getText() + lengthInBytes());
	}
	
	// just copy the data. If we want to optimize and use reference-counted strings at some point,
	// look at fix_str for ideas.
	TextFragment& TextFragment::operator=(const TextFragment& b) noexcept
	{
		if(this != &b)
		{
			dispose();
			mSize = b.mSize;
			create();
			if(mpText)
			{
				const char* bText = b.mpText;
				std::copy(bText, bText + mSize, mpText);
				nullTerminate();
			}
		}
		return *this;
	}
	
	TextFragment::TextFragment(TextFragment&& b) noexcept
	{
		moveDataFromOther(b);
	}
	
	TextFragment& TextFragment::operator=(TextFragment&& b) noexcept
	{
		dispose();
		moveDataFromOther(b);
		return *this;
	}
	
	// multiple-fragment constructors, used instead of operator+
	TextFragment::TextFragment(const TextFragment& a, const TextFragment& b) noexcept
	{
		construct(a.getText(), a.lengthInBytes(), b.getText(), b.lengthInBytes());
	}
	
	TextFragment::TextFragment(const TextFragment& t1, const TextFragment& t2, const TextFragment& t3) noexcept
	{
		construct(t1.getText(), t1.lengthInBytes(), t2.getText(), t2.lengthInBytes(), t3.getText(), t3.lengthInBytes());
	}
	
	TextFragment::TextFragment(const TextFragment& t1, const TextFragment& t2, const TextFragment& t3, const TextFragment& t4) noexcept
	{
		construct(t1.getText(), t1.lengthInBytes(), t2.getText(), t2.lengthInBytes(), 
							t3.getText(), t3.lengthInBytes(), t4.getText(), t4.lengthInBytes());
	}
	
	TextFragment::~TextFragment() noexcept
	{  
		dispose();
	}	
	
	void TextFragment::construct (const char* s1, size_t len1,
																const char* s2 , size_t len2 ,
																const char* s3 , size_t len3 ,
																const char* s4 , size_t len4 
																) noexcept 
	{
		mSize = (len1 + len2 + len3 + len4);
		create();
		if(mpText)
		{
			if(len1) std::copy (s1, s1 + len1, mpText);
			if(len2) std::copy (s2, s2 + len2, mpText + len1);
			if(len3) std::copy (s3, s3 + len3, mpText + len1 + len2);
			if(len4) std::copy (s4, s4 + len4, mpText + len1 + len2 + len3);
			nullTerminate();
		}
	}
	
	void TextFragment::create() noexcept
	{
		if(mSize >= kShortFragmentSizeInChars)
		{
			mpText = static_cast<char *>(malloc(mSize + 1));
		}
		else
		{
			mpText = mLocalText;
		}
	}
	
	void TextFragment::nullTerminate() noexcept
	{
		mpText[mSize] = 0;
	}
	
	void TextFragment::dispose() noexcept
	{
		if(mpText)
		{
			assert(mpText[mSize] == 0);
			if(mpText != mLocalText)
			{
				// free an external text. If the alloc has failed the ptr might be 0, which is OK
				free(mpText);
			}
			mpText = 0;
		}
	}
	
	void TextFragment::moveDataFromOther(TextFragment& b)
	{
		mSize = b.mSize;
		if(mSize >= kShortFragmentSizeInChars)
		{
			// move the data
			mpText = b.mpText; 
		}
		else
		{
			/*
			 TODO use SmallStackBuffer! and test
			 */
			
			// point to local storage and copy data
			mpText = mLocalText; 
			std::copy(b.mLocalText, b.mLocalText + mSize, mLocalText);
			nullTerminate();
		}
		
		// mark b as empty, nothing to dispose
		b.mpText = b.mLocalText;
		b.mSize = 0;
		b.nullTerminate();
	}
	
} // namespace ml

