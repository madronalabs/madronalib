//
//  MLFile.cpp
//  madronalib
//
//  Created by Randy Jones on 10/31/13.
//
//

#include "MLFile.h"

const MLFile MLFile::nullObject;

MLFile::MLFile() 
{
}

MLFile::MLFile(const std::string& path)
{
	mJuceFile = juce::File(path);
}

MLFile::~MLFile()
{
}

bool MLFile::exists() const 
{ 
	return mJuceFile.exists(); 
}

std::string MLFile::getShortName() const 
{ 
	return(std::string(mJuceFile.getFileNameWithoutExtension().toUTF8()));
}

std::string MLFile::getLongName() const 
{
	return(std::string(mJuceFile.getFullPathName().toUTF8()));
}

std::string MLFile::getParentDirectoryName() const
{
	return std::string(mJuceFile.getParentDirectory().getFileNameWithoutExtension().toUTF8());
}
