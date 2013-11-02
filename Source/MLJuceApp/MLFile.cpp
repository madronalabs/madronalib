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
    mShortName(dirName)
{}

MLFile::MLFile(const File startDir) :
    mFile(startDir), mIsDirectory(startDir.isDirectory())
{}

MLFile::MLFile(const File f, const std::string& p, const std::string& n) :
    mFile(f), mIsDirectory(f.isDirectory()), mRelativePath(p), mShortName(n)
{}

MLFile::~MLFile()
{
    debug() << "DELETING file " << mShortName << "\n";
}

void MLFile::clear()
{
    mFiles.clear();
}

void MLFile::insert(const std::string& path, MLFilePtr f)
{
    debug() << "INSERTING: " << path << "\n";
    int len = path.length(); 
    if(len)
    {
        int b = path.find_first_of("/");
        if(b == std::string::npos)
        {
            // leaf, insert file here creating dir if needed
            debug() << "        LEAF: " << path << "\n\n" ;
           
            // add leaf file to map
            mFiles[path] = f;
        }
        else
        {
            std::string firstDir = path.substr(0, b);
            std::string restOfDirs = path.substr(b + 1, len - b);
            
            debug() << "    FIRST: " << firstDir << ", REST " << restOfDirs << "\n";
            
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

void MLFile::buildMenu(MLMenuPtr m)
{
    m->clear();
    std::map<std::string, MLFilePtr>::const_iterator it;
    for(it = mFiles.begin(); it != mFiles.end(); ++it)
    {
        const MLFilePtr f = it->second;
        if(f->mIsDirectory)
        {
            debug() << "ADDING SUBMENU: " << f->mShortName << "\n";
            MLMenuPtr subMenu(new MLMenu());
//            subMenu->setItemOffset(m->getNumItems());
            f->buildMenu(subMenu);
            m->addSubMenu(subMenu, f->mShortName);
        }
        else
        {
            debug() << "ADDING ITEM: " << f->mShortName << "\n";
            m->addItem(f->mShortName);
        }
    }
}

// TODO remove this and use buildMenu instead, when we can implement adding recursive menus together
void MLFile::addToMenu(MLMenu* m)
{
    std::map<std::string, MLFilePtr>::const_iterator it;
    for(it = mFiles.begin(); it != mFiles.end(); ++it)
    {
        const MLFilePtr f = it->second;
        if(f->mIsDirectory)
        {
            debug() << "ADDING SUBMENU: " << f->mShortName << "\n";
            MLMenuPtr subMenu(new MLMenu());
            subMenu->setItemOffset(m->getNumItems());
            f->buildMenu(subMenu);
            m->addSubMenu(subMenu, f->mShortName);
        }
        else
        {
            debug() << "ADDING ITEM: " << f->mShortName << "\n";
            m->addItem(f->mShortName);
        }
    }
}

/*
MLFilePtr findFileByName(const std::string& name)
{
    
    
}
*/


