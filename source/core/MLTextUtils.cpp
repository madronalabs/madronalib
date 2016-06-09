//
//  MLTextUtils.cpp
//  madronalib
//
//  Created by Randy Jones on 12/3/14.
//
//

#include "MLTextUtils.h"
#include "../DSP/MLDSPGens.h" // for RandomSource TODO replace

#pragma mark string utilities

namespace ml { namespace textUtils {
	
	using namespace utf;
	
	const size_t countCodePoints(const TextFragment& frag)
	{
		utf::stringview<const char*> sv(frag.text, frag.text + frag.lengthInBytes);
		return sv.codepoints();
	}
	
	int findFirst(const TextFragment& frag, const codepoint_type c)
	{
		auto first = codepoint_iterator<const char*>(frag.text);
		auto last = codepoint_iterator<const char*>(frag.text + frag.lengthInBytes);
		int i=0;
		int r = -1;
		for (auto it = first; it != last; ++it) 
		{
			if(c == *it)
			{
				r = i;
				break;
			}
			i++;
		}	
		return r;
	}
	
	int findLast(const TextFragment& frag, const codepoint_type c)
	{
		auto first = codepoint_iterator<const char*>(frag.text);
		auto last = codepoint_iterator<const char*>(frag.text + frag.lengthInBytes);
		int i=0;
		int r = -1;
		for (auto it = first; it != last; ++it) 
		{
			if(c == *it)
			{
				r = i;
			}
			i++;
		}	
		return r;
	}
	
	TextFragment subText(const TextFragment& frag, int start, int end)
	{
		// this impl does an unneccesary copy, to keep TextFragment very simple for now.
		
		if(start >= end) return TextFragment("");
		
		// temp buffer on stack big enough to hold whole input fragment if needed.
		// we won't know the output fragment size in bytes until iterating the code points. 
		char buf[frag.lengthInBytes];
		std::fill(buf, buf+frag.lengthInBytes, 0);
		char* pb = buf;
		
		auto first = codepoint_iterator<const char*>(frag.text);
		
		auto it = first;
		for(int i=0; i<start; ++i)
		{
			++it;
		}
		
		for (int i=0; i<end - start; ++i) 
		{
			// write the codepoint as UTF-8 to the buffer
			pb = utf::internal::utf_traits<utf::utf8>::encode(*it, pb);
			++it;
		}	
		
		return TextFragment(buf);
	}
	
	TextFragment stripExtension(const TextFragment& frag)
	{
		int dotLoc = findLast(frag, '.');
		if(dotLoc >= 0)
		{
			return subText(frag, 0, dotLoc);
		}
		
		return frag;
	}
	
	TextFragment getShortName(const TextFragment& frag)
	{
		int slashLoc = findLast(frag, '/');
		if(slashLoc >= 0)
		{
			return subText(frag, slashLoc + 1, countCodePoints(frag));
		}
		
		return frag;
	}
	
	TextFragment getPath(const TextFragment& frag)
	{
		int slashLoc = findLast(frag, '/');
		if(slashLoc >= 0)
		{
			return subText(frag, 0, slashLoc);
		}
		
		return frag;
	}
	
	bool beginsWith(TextFragment fa, TextFragment fb)
	{
		int lenA = fa.lengthInBytes;
		int lenB = fb.lengthInBytes;
		
		if(lenB > lenA) return false;
		bool r = true;
		
		for(int i=0; i<lenB; ++i)
		{
			if(fa.text[i] != fb.text[i])
			{
				r = false;
				break;
			}
		}

		return r;
	}
	
	bool endsWith(TextFragment fa, TextFragment fb)
	{
		int lenA = fa.lengthInBytes;
		int lenB = fb.lengthInBytes;
		
		if(lenB > lenA) return false;
		bool r = true;
		
		for(int i=0; i<lenB; ++i)
		{
			if(fa.text[lenA - lenB + i] != fb.text[i])
			{
				r = false;
				break;
			}
		}
		
		return r;
	}


	
	
	
	
	std::vector< std::string > parsePath(const std::string& pathStr)
	{
		std::vector<std::string> path;
		std::string workingStr = pathStr;
		size_t segmentLength;
		do
		{
			// found a segment
			segmentLength = workingStr.find_first_of("/");
			if(segmentLength)
			{
				// iterate
				path.push_back(workingStr.substr(0, segmentLength));
				workingStr = workingStr.substr(segmentLength + 1, workingStr.length() - segmentLength);
			}
			else
			{
				// leading slash or null segment
				workingStr = workingStr.substr(1, workingStr.length());
			}
		}
		while(segmentLength != std::string::npos);
		return path;
	}
	
	static const char kLetters[33] = "aabcdeefghijklmnnoopqrssttuvwxyz";
	std::vector< std::string > vectorOfNonsenseWords( int len )
	{
		ml::RandomSource randSource;
		std::vector< std::string > words;
		for(int i = 0; i < len; ++i)
		{
			std::string newStr;
			uint32_t r32 = randSource.getIntSample() >> 16;
			int wordLen = (r32 & 7) + 3;
			
			for(int j=0; j<wordLen; ++j)
			{
				r32 = randSource.getIntSample() >> 16;
				int idx = (r32 & 31);
				newStr += (kLetters[idx]);
			}
			words.push_back(newStr);
		}
		return words;
	}
	
	char * spaceStr( int numIndents )
	{
		static char * pBuf = (char *)"                                                   ";
		static int len = (int)strlen(pBuf);
		int n = numIndents*2;
		if (n > len) n = len;
		return &pBuf[len - n];
	}
	

} } // ml:textUtils
