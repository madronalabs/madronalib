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
typedef std::tr1::shared_ptr<MLFile> MLFilePtr;
typedef std::map<std::string, MLFilePtr, MLStringCompareFn> nameToFileMap;

class MLFile
{
public:
    MLFile();
    MLFile(const std::string& dirName);
    MLFile(const File startDir);
    MLFile(const File f, const std::string& shortName, const std::string& longName);
    ~MLFile();
    
    bool exists() const { return mJuceFile.exists(); }
    bool isDirectory() const { return mJuceFile.isDirectory(); }
	bool operator== (const MLFile& b) const { return getJuceFile().getFullPathName() == b.getJuceFile().getFullPathName(); }
	
	std::string getAbsolutePath() const;
    const std::string & getShortName() const { return mShortName; }
    const std::string & getLongName() const { return mLongName; }
	juce::File getJuceFile() const { return mJuceFile; }
	
	static const MLFile nullObject;
	
	// recursive stuff that should go somewhere else
	void clear();
	
	// insert a file into the tree, routing by path name relative to collection root.
	void insertFile(const std::string& relPath, MLFilePtr f);
    const MLFile& find(const std::string& path);
    void buildMenu(MLMenuPtr m) const;
    void buildMenuIncludingPrefix(MLMenuPtr m, std::string prefix) const;
    void buildMenuExcludingPrefix(MLMenuPtr m, std::string prefix) const;

private:
	juce::File mJuceFile;

    // the file's name including prefix.
	std::string mShortName;
	
    // path relative to collection root including name
    std::string mLongName;
	
	// the recursive part
	nameToFileMap mFiles;
};

#endif /* defined(__MLFile__) */
