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

ml::TextFragment MLFile::getShortName() const 
{ 
	if(isDirectory())
	{
		std::cout << "dir\n";
		std::cout << mJuceFile.getFileName() << "\n";
		return(ml::TextFragment(mJuceFile.getFileName().toUTF8()));
	}
	else
	{
		return(ml::TextFragment(mJuceFile.getFileNameWithoutExtension().toUTF8()));
	}
}

// TODO return a path?
ml::TextFragment MLFile::getLongName() const 
{
	return(ml::TextFragment(mJuceFile.getFullPathName().toUTF8()));
}

ml::TextFragment MLFile::getParentDirectoryName() const
{
	return ml::TextFragment(mJuceFile.getParentDirectory().getFileNameWithoutExtension().toUTF8());
}
