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

// TODO this is confusing and bad. Some weird stuff with names is enabling trees.
// Files that can contain other files should go in MLFileCollection::Node or similar.
MLFile::MLFile(const std::string& dirName) :
	mShortName(dirName)
{}

MLFile::MLFile(const File startDir) :
	mJuceFile(startDir)
{}

MLFile::MLFile(const File f, const std::string& shortName, const std::string& longName) :
    mJuceFile(f), mShortName(shortName), mLongName(longName)
{}

MLFile::~MLFile()
{
}

void MLFile::clear()
{
    mFiles.clear();
}

void MLFile::insertFile(const std::string& path, MLFilePtr f)
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
            
			// debug() << "    MLFile::insert: FIRST: " << firstDir << ", REST " << restOfDirs << "\n";
            
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
                mFiles[firstDir]->insertFile(restOfDirs, f);
            }
        }
    }
    else
    {
        MLError() << "MLFile::insert: empty file name!\n";
    }
}

const MLFile& MLFile::find(const std::string& path)
{
    // debug() << "FINDING: " << path << "\n";
    int len = path.length();
    if(len)
    {
        int b = path.find_first_of("/");
        if(b == std::string::npos)
        {
            // end of path, find short name here or return fail.
            //debug() << "        path end: " << path << "\n\n" ;
            
            nameToFileMap::const_iterator it;
            it = mFiles.find(path);
            if(it != mFiles.end())
            {
                // return the found file.
                return *(it->second);
            }
            else
            {
                return MLFile::nullObject;
            }
        }
        else
        {
            std::string firstDir = path.substr(0, b);
            std::string restOfDirs = path.substr(b + 1, len - b);
            
            //debug() << "    FIRST: " << firstDir << ", REST " << restOfDirs << "\n";
            
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
                return MLFile::nullObject;
            }
        }
    }
    else
    {
        MLError() << "MLFile::find: empty file name!\n";
    }
    return MLFile::nullObject;
}

std::string MLFile::getAbsolutePath() const
{
    return std::string(mJuceFile.getFullPathName().toUTF8());
}

void MLFile::buildMenu(MLMenuPtr m) const
{
    m->clear();
    
    nameToFileMap::const_iterator it;
    for(it = mFiles.begin(); it != mFiles.end(); ++it)
    {
        const MLFilePtr f = it->second;

		// MLTEST
		juce::File jf = f->getJuceFile();
		String jfn = jf.getFullPathName();
	//	debug() << " [ checking " << jfn << ": ";

		if(!(f->exists()))
		{
			debug() << "NO EXIST?! ";
		}
		else if(f->isDirectory())
        {
    // debug() << "ADDING SUBMENU: " << f->mShortName << "\n";
            MLMenuPtr subMenu(new MLMenu());
            f->buildMenu(subMenu);
            m->addSubMenu(subMenu, f->mShortName);
        }
        else
        {
    //debug() << "ADDING ITEM: " << f->mShortName << "\n";
            m->addItem(f->mShortName);
        }

	//	debug() << "]\n";
    }
}

// build a menu of only the files in top-level directories starting with the given prefix.
// this adds only directories, not files. Made for adding "factory" presets separately.
void MLFile::buildMenuIncludingPrefix(MLMenuPtr m, std::string prefix) const
{
    int prefixLen = prefix.length();
    m->clear();
    
    nameToFileMap::const_iterator it;
    for(it = mFiles.begin(); it != mFiles.end(); ++it)
    {
        const MLFilePtr f = it->second;
        std::string filePrefix = f->mShortName.substr(0, prefixLen);
        if(filePrefix.compare(prefix) == 0)
        {
            if(f->isDirectory())
            {
                MLMenuPtr subMenu(new MLMenu());
                f->buildMenu(subMenu);
                m->addSubMenu(subMenu, f->mShortName);
            }
            else
            {
                m->addItem(f->mShortName);
            }
        }
    }
}

// build a menu of only the files not starting with the prefix.
void MLFile::buildMenuExcludingPrefix(MLMenuPtr m, std::string prefix) const
{
    int prefixLen = prefix.length();
    m->clear();
    
    nameToFileMap::const_iterator it;
    for(it = mFiles.begin(); it != mFiles.end(); ++it)
    {
        const MLFilePtr f = it->second;
        std::string filePrefix = f->mShortName.substr(0, prefixLen);
        if(filePrefix.compare(prefix) != 0)
        {
            if(f->isDirectory())
            {
                MLMenuPtr subMenu(new MLMenu());
                f->buildMenu(subMenu);
                m->addSubMenu(subMenu, f->mShortName);
            }
            else
            {
                m->addItem(f->mShortName);
            }
        }
    }
}
