//
//  MLStringUtils.h
//  madronalib
//
//  Created by Randy Jones on 12/3/14.
//
//

#ifndef __MLStringUtils__
#define __MLStringUtils__

#include <iostream>

class MLStringUtils
{
public:
	static const std::string stripExtension(const std::string& str);
	static const std::string getShortName(const std::string& str);
	static const std::string getPath(const std::string& str);
};

#endif /* defined(__MLStringUtils__) */
