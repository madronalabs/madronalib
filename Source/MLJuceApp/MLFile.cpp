//
//  MLFile.cpp
//  madronalib
//
//  Created by Randy Jones on 10/31/13.
//
//

#include "MLFile.h"

const MLFile MLFile::nullObject;

MLFile::MLFile(){}

MLFile::MLFile(const std::string& path) :
    mJuceFile(path)
{
	mShortName = std::string(mJuceFile.getFileNameWithoutExtension().toUTF8());
	mLongName = std::string(mJuceFile.getFullPathName().toUTF8());
}

MLFile::~MLFile()
{
}

std::string MLFile::getParentDirectoryName() const
{
	// temp
	return std::string(mJuceFile.getParentDirectory().getFileNameWithoutExtension().toUTF8());
}
