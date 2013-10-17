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
    mStartDir(startDir),
    mExtension(extension),
    mpListener(nullptr)
{
    if(mStartDir != File::nonexistent)
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
    mFiles.clear();
	if (mStartDir.exists() && mStartDir.isDirectory())
    {
        Array<File> allFilesFound;
        const int whatToLookFor = File::findFilesAndDirectories | File::ignoreHiddenFiles;
        const String& wildCard = "*";
        bool recurse = true;

        DirectoryIterator di (mStartDir, recurse, wildCard, whatToLookFor);
        while (di.next())
        {
            allFilesFound.add (di.getFile());
            found++;
        }
         
        // iterate all found files, including directories
        for(int i=0; i<found; ++i)
        {
            File f = allFilesFound[i];
            if (f.existsAsFile() && f.hasFileExtension(mExtension))
            {
                String shortName = f.getFileName();                
                String relativePath;
                File parentDir = f.getParentDirectory();
                if(parentDir == mStartDir)
                {
                    relativePath = "";
                }
                else
                {
                    relativePath = parentDir.getRelativePathFrom(mStartDir);
                }
                mFiles.push_back(FileInfo(f, relativePath, shortName));
                if(mpListener)
                {
                    // give file and index to listener for processing
                    mpListener->processFile (mName, f, processed++);
                }
            }
        }
    }
    return found;
}

const File& MLFileCollection::getFileByIndex(int idx)
{
    return mFiles[idx].mFile;
}


MLMenuPtr MLFileCollection::buildMenu(bool flat)
{
    // make a new menu named after this collection. 
    MLMenuPtr m(new MLMenu(mName));
    int size = mFiles.size();
    if(flat)
    {
        for(int i=0; i<size; ++i)
        {
            
            FileInfo& f = mFiles[i];
         debug() << "buildMenu: adding " << (const char *)(f.mShortName.toUTF8()) << "\n";
            m->addItem(f.mShortName.toUTF8());
        }
    }
    else
    {
        
    }
    return m;
}

void MLFileCollection::dump()
{
 	std::vector<FileInfo>::const_iterator it;
    
    debug() << "MLFileCollection " << mName << ":\n";
    
    // add each element of submenu in turn to our flat item vector
    int idx = 0;
	for(it = mFiles.begin(); it != mFiles.end(); it++)
	{
        
		debug() << "    " << idx++ << ": " << it->mRelativePath << " " << it->mShortName << "\n";
	}
    
}

