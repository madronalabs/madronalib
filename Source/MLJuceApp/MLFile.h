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
#include "MLTypes.h"

class MLFile;
typedef std::shared_ptr<MLFile> MLFilePtr;

class MLFile
{
public:
    MLFile();
    MLFile(const std::string& path);
    ~MLFile();
    
    bool exists() const { return mJuceFile.exists(); }
    bool isDirectory() const { return mJuceFile.isDirectory(); }
	bool operator== (const MLFile& b) const { return getJuceFile().getFullPathName() == b.getJuceFile().getFullPathName(); }
	
	const std::string & getShortName() const { return mShortName; }
    const std::string & getLongName() const { return mLongName; }
	
	std::string getParentDirectoryName() const;
	juce::File getJuceFile() const { return mJuceFile; }
	
	static const MLFile nullObject;
	
private:
	juce::File mJuceFile;

    // the file's name including prefix.
	std::string mShortName;
	
    // path relative to collection root including name
    std::string mLongName;

};

#endif /* defined(__MLFile__) */
