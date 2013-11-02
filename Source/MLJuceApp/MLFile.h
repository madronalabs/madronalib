//
//  MLFile.h
//  Kaivo
//
//  Created by Randy Jones on 10/31/13.
//
//

#ifndef __Kaivo__MLFile__
#define __Kaivo__MLFile__

#include "JuceHeader.h"
#include "MLDefaultFileLocations.h"
#include "MLMenu.h"

class MLFile;
typedef std::tr1::shared_ptr<MLFile> MLFilePtr;

class MLFile
{
public:
    MLFile(const std::string& dirName);
    MLFile(const File startDir);
    MLFile(const File f, const std::string& p, const std::string& n);
    ~MLFile();
    
    void clear();
    void insert(const std::string& path, MLFilePtr f);
    void buildMenu(MLMenuPtr m);
    void addToMenu(MLMenu* m); // TEMP
    
    File mFile;
    bool mIsDirectory;
    std::string mRelativePath;
    std::string mShortName;
    
private:
    std::map<std::string, MLFilePtr> mFiles;
//    MLFilePtr findFileByName(const std::string& name);

    
};

#endif /* defined(__Kaivo__MLFile__) */
