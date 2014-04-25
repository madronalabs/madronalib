//
//  MLFile.h
//  MadronaLib
//
//  Created by Randy Jones on 10/31/13.
//
//

#ifndef __Kaivo__MLFile__
#define __Kaivo__MLFile__

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
    MLFile(const std::string& dirName);
    MLFile(const File startDir);
    MLFile(const File f, const std::string& shortName, const std::string& longName);
    ~MLFile();
    
    void clear();
    void insert(const std::string& relPath, MLFilePtr f);
    MLFilePtr find(const std::string& path);
    std::string getAbsolutePath();
    void buildMenu(MLMenuPtr m) const;
    void buildMenuIncludingPrefix(MLMenuPtr m, std::string prefix) const;
    void buildMenuExcludingPrefix(MLMenuPtr m, std::string prefix) const;
    
    File mFile;
    bool mIsDirectory;
    int mIndex;
    std::string mShortName;
    // path relative to collection root including name
    std::string mLongName;
    
private:
    nameToFileMap mFiles;
};

#endif /* defined(__Kaivo__MLFile__) */
