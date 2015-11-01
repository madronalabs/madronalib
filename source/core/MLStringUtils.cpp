//
//  MLStringUtils.cpp
//  madronalib
//
//  Created by Randy Jones on 12/3/14.
//
//

#include "MLStringUtils.h"

#pragma mark string utilities

namespace ml { namespace stringUtils {
	
const std::string stripExtension(const std::string& str)
{
    std::string r(str);
    size_t b = r.find_last_of(".");
    if(b != std::string::npos)
    {
        r = r.substr(0, b);
    }
    return r;
}

const std::string getShortName(const std::string& str)
{
    std::string r(str);
    size_t b = r.find_last_of("/");
    if(b != std::string::npos)
    {
        size_t len = r.length();
        r = r.substr(b + 1, len - b);
    }
    return r;
}

const std::string getPath(const std::string& str)
{
    std::string r;
    size_t b = str.find_last_of("/");
    if(b != std::string::npos)
    {
        r = str.substr(0, b);
    }
    return r;
}

char * spaceStr( int numIndents )
{
	static char * pBuf = (char *)"                                                   ";
	static int len = (int)strlen(pBuf);
	int n = numIndents*2;
	if (n > len) n = len;
	return &pBuf[len - n];
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
	std::vector< std::string > words;
	for(int i = 0; i < len; ++i)
	{
		std::string newStr;
		uint32_t r32 = MLRand32() >> 16;
		int wordLen = (r32 & 7) + 3;
		
		for(int j=0; j<wordLen; ++j)
		{
			r32 = MLRand32() >> 16;
			int idx = (r32 & 31);
			newStr += (kLetters[idx]);
		}
		words.push_back(newStr);
	}
	return words;
}

} } // ml:stringUtils
