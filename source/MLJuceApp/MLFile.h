//
//  MLFile.h
//  madronalib
//
//  Created by Randy Jones on 10/31/13.
//
//

#ifndef __MLFile__
#define __MLFile__

#include "JuceHeader.h"
#include "MLDefaultFileLocations.h"
#include "MLMenu.h"
#include "MLStringUtils.h"

class MLFile
{
public:
    MLFile();
    MLFile(const std::string& path);
    ~MLFile();
    
	bool exists() const;
    bool isDirectory() const { return mJuceFile.isDirectory(); }
	bool operator== (const MLFile& b) const { return getJuceFile().getFullPathName() == b.getJuceFile().getFullPathName(); }
	bool operator!= (const MLFile& b) const { return !(operator==(b)); }
	
	std::string getShortName() const;
	std::string getLongName() const;
	
	std::string getParentDirectoryName() const;
	const juce::File& getJuceFile() const { return mJuceFile; }
	
	static const MLFile nullObject;
	
private:
	juce::File mJuceFile;

};

#endif /* defined(__MLFile__) */
