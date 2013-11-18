//
//  MLFile.cpp
//  Kaivo
//
//  Created by Randy Jones on 10/31/13.
//
//

#include "MLFile.h"

// no file pointer, just a directory name 
MLFile::MLFile(const std::string& dirName) :
    mFile(File::nonexistent),
    mIsDirectory(true),
    mShortName(dirName),
    mIndex(-1)
{}

MLFile::MLFile(const File startDir) :
mFile(startDir), mIsDirectory(startDir.isDirectory()), mIndex(-1)
{}

MLFile::MLFile(const File f, const std::string& shortName, const std::string& longName) :
    mFile(f), mIsDirectory(f.isDirectory()), mShortName(shortName), mLongName(longName), mIndex(-1)
{}

MLFile::~MLFile()
{
    // debug() << "DELETING file " << mShortName << "\n";
}

void MLFile::clear()
{
    mFiles.clear();
}

void MLFile::insert(const std::string& path, MLFilePtr f)
{
    // debug() << "INSERTING: " << path << "\n";
    int len = path.length();
    if(len)
    {
        int b = path.find_first_of("/");
        if(b == std::string::npos)
        {
            // leaf, insert file here creating dir if needed
            // debug() << "        LEAF: " << path << "\n\n" ;
            
            // add leaf file to map
            mFiles[path] = f;
        }
        else
        {
            std::string firstDir = path.substr(0, b);
            std::string restOfDirs = path.substr(b + 1, len - b);
            
            // debug() << "    FIRST: " << firstDir << ", REST " << restOfDirs << "\n";
            
            // find or add first dir
            if(firstDir == "")
            {
                MLError() << "MLFile::insert: empty directory name!\n";
            }
            else
            {
                if(mFiles.find(firstDir) == mFiles.end())
                {
                    mFiles[firstDir] = MLFilePtr(new MLFile(firstDir));
                }
                mFiles[firstDir]->insert(restOfDirs, f);
            }
        }
    }
    else
    {
        MLError() << "MLFile::insert: empty file name!\n";
    }
}

MLFilePtr MLFile::find(const std::string& path)
{
  debug() << "FINDING: " << path << "\n";
    int len = path.length();
    if(len)
    {
        int b = path.find_first_of("/");
        if(b == std::string::npos)
        {
            // end of path, find short name here or return fail.
         debug() << "        path end: " << path << "\n\n" ;
            
            std::map<std::string, MLFilePtr>::const_iterator it;
            it = mFiles.find(path);
            if(it != mFiles.end())
            {
                // return the found file.
                return it->second;
            }
            else
            {
                debug() << "did not find " << path << " in :\n";
                
                std::map<std::string, MLFilePtr>::const_iterator it2;
                for(it2 = mFiles.begin(); it2 != mFiles.end(); ++it2)
                {
                        debug() << it2->first << ", ";
                }
                debug() << "\n";
                
                return MLFilePtr();
            }
        }
        else
        {
            std::string firstDir = path.substr(0, b);
            std::string restOfDirs = path.substr(b + 1, len - b);
            
    debug() << "    FIRST: " << firstDir << ", REST " << restOfDirs << "\n";
            
            // find file matching first dir
            if(firstDir == "")
            {
                MLError() << "MLFile::find: empty directory name!\n";
            }
            else if(mFiles.find(firstDir) != mFiles.end())
            {
                // look for rest of dirs in found non-leaf file
                return mFiles[firstDir]->find(restOfDirs);
            }
            else
            {
                return MLFilePtr();
            }
        }
    }
    else
    {
        MLError() << "MLFile::find: empty file name!\n";
    }
    return MLFilePtr();
}

std::string MLFile::getAbsolutePath()
{
    return std::string(mFile.getFullPathName().toUTF8());
}

void MLFile::buildMenu(MLMenuPtr m)
{
    m->clear();
    std::map<std::string, MLFilePtr>::const_iterator it;
    for(it = mFiles.begin(); it != mFiles.end(); ++it)
    {
        const MLFilePtr f = it->second;
        if(f->mIsDirectory)
        {
            // debug() << "ADDING SUBMENU: " << f->mShortName << "\n";
            MLMenuPtr subMenu(new MLMenu());
            f->buildMenu(subMenu);
            m->addSubMenu(subMenu, f->mShortName);
        }
        else
        {
            // debug() << "ADDING ITEM: " << f->mShortName << "\n";
            m->addItem(f->mShortName);
        }
    }
}
