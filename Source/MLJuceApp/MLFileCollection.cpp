//
//  MLFileCollection.cpp
//  madronalib
//
//  Created by Randy Jones on 10/10/13.
//
//

#include "MLFileCollection.h"

MLFileCollection::MLFileCollection(MLSymbol name, const File startDir, String extension):
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
                std::string longName(rPath + delimiter + sName);
                
                MLFilePtr newFile(new MLFile(f, sName, longName));
                
                // insert file into file tree
                mRoot.insert(longName, newFile);
                
                // push to index
                mFilesByIndex.push_back(newFile);
                newFile->mIndex = processed;
                
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


// count the number of files in the collection, and begin making the collection.
// returns the number of found files.
//
int MLFileCollection::beginProcessFiles()
{
    int found = 0;

	if (mRoot.mFile.exists() && mRoot.mFile.isDirectory())
    {
        mFiles.clear();
        const int whatToLookFor = File::findFilesAndDirectories | File::ignoreHiddenFiles;
        const String& wildCard = "*";
        bool recurse = true;
        
        DirectoryIterator di (mRoot.mFile, recurse, wildCard, whatToLookFor);
        while (di.next())
        {
            mFiles.push_back (di.getFile());
            found++;
        }
    }
    else
    {
        found = -1;
    }
    return found;
}

// iterate one step in making the collection. 
void MLFileCollection::iterateProcessFiles(int i)
{
    int r = 0;
    
    File f = mFiles[i];
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
        std::string longName(rPath + delimiter + sName);
        
        MLFilePtr newFile(new MLFile(f, sName, longName));
        
        // insert file into file tree
        mRoot.insert(longName, newFile);
        
        // push to index
        mFilesByIndex.push_back(newFile);
        int newIdx = mFilesByIndex.size() - 1;
        newFile->mIndex = newIdx;
        
        if(mpListener)
        {
            // give file and index to listener for processing
            mpListener->processFile (mName, f, newIdx);
        }
    }
}


class FileFinderThreadWithProgressWindow  : public ThreadWithProgressWindow
{
public:
    FileFinderThreadWithProgressWindow(MLFileCollection& m, Component* pLocationComp, String str) :
        mCollection(m),
        ThreadWithProgressWindow (String(""), true, false, 10000, String(), pLocationComp),
        mProcessMessage(str)
    {
    }
    
    ~FileFinderThreadWithProgressWindow()
    {
    }
    
    void run()
    {
        /*
        // count and display uncertain progress
        setStatusMessage ("Counting files...");
        setProgress (-1.0f);
        wait (1000); // TEST
        */
        
        // OK, assuming this can happen all in one chunk quickly.
        setStatusMessage (mProcessMessage);
        int filesFound = mCollection.beginProcessFiles();
        
        mCollection.clear();
        
        // iterate and display known progress with informative message
        for(int i=0; i<filesFound; ++i)
        {
            mCollection.iterateProcessFiles(i);
            if (threadShouldExit()) return;
            setProgress ((float)i/(float)filesFound);
        }
    }
    
private:
    MLFileCollection& mCollection;
    String mProcessMessage;
};


void MLFileCollection::findFilesWithProgressWindow(Component* pLocationComp, String str)
{
    FileFinderThreadWithProgressWindow finderThread(*this, pLocationComp, str);
    finderThread.runThread();
}

std::string MLFileCollection::getFileNameByIndex(int idx)
{
    int size = mFilesByIndex.size();
    if(within(idx, 0, size))
    {
        return mFilesByIndex[idx]->mLongName;
    }
    return std::string();
}

MLFilePtr MLFileCollection::getFileByIndex(int idx)
{
    int size = mFilesByIndex.size();
    if(within(idx, 0, size))
    {
        return mFilesByIndex[idx];
    }
    return MLFilePtr();
}

const MLFilePtr MLFileCollection::getFileByName(const std::string& fullName)
{
    return mRoot.find(fullName);
}

const int MLFileCollection::getFileIndexByName(const std::string& fullName)
{
    int r = -1;
    MLFilePtr f = mRoot.find(fullName);
    if(f != MLFilePtr())
    {
        int len = mFilesByIndex.size();
        for(int i = 0; i<len; ++i)
        {
            const MLFilePtr g = mFilesByIndex[i];
            if(f == g)
            {
                r = i;                
                break;
            }
        }        
    }
    return r;
}

// TODO re-index is needed after this, for now we are re-searching for all files!
//
const MLFilePtr MLFileCollection::createFile(const std::string& relativePathAndName)
{
    std::string sName = getShortName(relativePathAndName);
    std::string fullPath = mRoot.getAbsolutePath() + "/" + relativePathAndName;
    
    // need absolute path to make the Juce file
    File *f = new File(String(fullPath.c_str()));
    MLFilePtr newFile(new MLFile(*f, sName, relativePathAndName));
    
    // insert file into file tree at relative path
    mRoot.insert(relativePathAndName, newFile);

    // TODO repair index
    
    return newFile;
}

// get part of absolute path p, if any, relative to our root path.
std::string MLFileCollection::getRelativePath(const std::string& p)
{
    std::string rootPath = mRoot.getAbsolutePath();
    debug() << "root:" << rootPath << "\n";
    debug() << ", p: " << p << "\n";
    std::string relPath;
    
    // p should begin with root
    size_t rootPos = p.find(rootPath);
    if(rootPos == 0)
    {
        int rLen = rootPath.length();
        int pLen = p.length();
        relPath = p.substr(rLen + 1, pLen - rLen - 1);        
    }
    
    return relPath;
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

void MLFileCollection::dump()
{
 	std::vector<MLFilePtr>::const_iterator it;
    
    debug() << "MLFileCollection " << mName << ":\n";
    
    int len = mFilesByIndex.size();
	for(int i = 0; i<len; ++i)
	{
        const MLFilePtr f = mFilesByIndex[i];
		debug() << "    " << i << ": " << f->mShortName << "\n";
	}
}

