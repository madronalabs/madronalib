//
//  MLStringUtils.h
//  Aalto
//
//  Created by Randy Jones on 12/3/14.
//
//

#ifndef __Aalto__MLStringUtils__
#define __Aalto__MLStringUtils__

#include <iostream>

class MLStringUtils
{
public:
	static const std::string stripExtension(const std::string& str);
	static const std::string getShortName(const std::string& str);
	static const std::string getPath(const std::string& str);
};

#endif /* defined(__Aalto__MLStringUtils__) */
