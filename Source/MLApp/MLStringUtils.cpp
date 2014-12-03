//
//  MLStringUtils.cpp
//  Aalto
//
//  Created by Randy Jones on 12/3/14.
//
//

#include "MLStringUtils.h"

#pragma mark string utilities

const std::string MLStringUtils::stripExtension(const std::string& str)
{
    std::string r(str);
    int b = r.find_last_of(".");
    if(b != std::string::npos)
    {
        r = r.substr(0, b);
    }
    return r;
}

const std::string MLStringUtils::getShortName(const std::string& str)
{
    std::string r(str);
    int b = r.find_last_of("/");
    if(b != std::string::npos)
    {
        int len = r.length();
        r = r.substr(b + 1, len - b);
    }
    return r;
}

const std::string MLStringUtils::getPath(const std::string& str)
{
    std::string r;
    int b = str.find_last_of("/");
    if(b != std::string::npos)
    {
        r = str.substr(0, b);
    }
    return r;
}

