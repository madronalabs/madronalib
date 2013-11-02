//
//  MLFileCollection.cpp
//  Soundplane
//
//  Created by Randy Jones on 10/10/13.
//
//

#include "MLFileCollection.h"

MLFileCollection::MLFileCollection(MLSymbol name, const File startDir, String extension): //, int maxDepth) :
    mName(name),
    mExtension(extension),
    mpListener(nullptr),
    mRoot(startDir)
{
    if(mRoot.mFile != File::nonexistent)
    {
        //startTimer(100);
    }
    else
    {
        
    }
}

MLFileCollection::~MLFileCollection()
{
}

void MLFileCollection::clear()
{
    mRoot.clear();
    mFilesByIndex.clear();
}

void MLFileCollection::setListener (Listener* listener)
{
    mpListener = listener;
}

void MLFileCollection::timerCallback()
{
    // TODO look for files in the start directory. After the whole directory has been scanned,
    // keep an eye on the directory, look for changed files, and add / process them when needed.
    //ScopedPointer<DirectoryContentsList> list;
    //TimeSliceThread thread;
}

// TODO list of extensions to look for, list of directories to ignore
//
int MLFileCollection::findFilesImmediate()
{
    int found = 0;
    int processed = 0;
    clear();
	if (mRoot.mFile.exists() && mRoot.mFile.isDirectory())
    {
        Array<File> allFilesFound;
        const int whatToLookFor = File::findFilesAndDirectories | File::ignoreHiddenFiles;
        const String& wildCard = "*";
        bool recurse = true;

        DirectoryIterator di (mRoot.mFile, recurse, wildCard, whatToLookFor);
        while (di.next())
        {
            allFilesFound.add (di.getFile());
            found++;
        }
         
        // iterate all found files, including directories
        for(int i=0; i<found; ++i)
        {
            File f = allFilesFound[i];
            String shortName = f.getFileName();
            String relativePath;
            File parentDir = f.getParentDirectory();
            if(parentDir == mRoot.mFile)
            {
                relativePath = "";
            }
            else
            {
                relativePath = parentDir.getRelativePathFrom(mRoot.mFile);
            }

            if (f.existsAsFile() && f.hasFileExtension(mExtension))
            {
                std::string rPath(relativePath.toUTF8());
                std::string delimiter = (rPath == "" ? "" : "/");
                std::string sName(shortName.toUTF8());
                
                MLFilePtr newFile(new MLFile(f, rPath, sName)); // TODO no path in file
                
                // insert file into file tree
                mRoot.insert(rPath + delimiter + sName, newFile);
                
                // push to index
                mFilesByIndex.push_back(newFile);
                
                if(mpListener)
                {
                    // give file and index to listener for processing
                    mpListener->processFile (mName, f, processed);
                }
                processed++;
            }
        }
    }
    return processed;
}

const MLFile& MLFileCollection::getFileByIndex(int idx)
{
    idx = clamp(idx, 0, (int)mFilesByIndex.size() - 1);
    return *(mFilesByIndex[idx]);
}

MLMenuPtr MLFileCollection::buildMenu(bool flat)
{
    // make a new menu named after this collection and containing all of the files in it.
    MLMenuPtr m(new MLMenu(mName));
    int size = mFilesByIndex.size();
    if(flat)
    {
        for(int i=0; i<size; ++i)
        {
            MLFilePtr f = mFilesByIndex[i];
            debug() << "buildMenu: adding " << f->mShortName << "\n";
            m->addItem(f->mShortName);
        }
    }
    else
    {
        mRoot.buildMenu(m);
    }
    return m;
}

void MLFileCollection::addToMenu(MLMenu* m, bool flat)
{
    int size = mFilesByIndex.size();
    if(flat)
    {
        for(int i=0; i<size; ++i)
        {
            MLFilePtr f = mFilesByIndex[i];
            debug() << "buildMenu: adding " << f->mShortName << "\n";
            m->addItem(f->mShortName);
        }
    }
    else
    {
        mRoot.addToMenu(m);
    }
    m->renumber();
}

void MLFileCollection::dump()
{
 	std::vector<MLFilePtr>::const_iterator it;
    
    debug() << "MLFileCollection " << mName << ":\n";
    
    // add each element of submenu in turn to our flat item vector
    int idx = 0;
	for(it = mFilesByIndex.begin(); it != mFilesByIndex.end(); it++)
	{
        const MLFilePtr f = *it;
		debug() << "    " << idx++ << ": " << f->mRelativePath << " " << f->mShortName << "\n";
	}
}

