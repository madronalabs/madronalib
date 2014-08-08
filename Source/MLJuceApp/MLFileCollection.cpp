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
    setProperty("progress", -1);
}

MLFileCollection::~MLFileCollection()
{
    if(mSearchThread)
    {
        mSearchThread->stopThread(1000);
    }
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

// count the number of files in the collection, and begin making the collection.
// returns the number of found files, or -1 if the file root is not usable.
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
        }
        found = mFiles.size();
    }
    else
    {
        found = -1;
    }
    
    return found;
}

// examine all files in mFiles and build indexed tree
//
void MLFileCollection::buildTree()
{
    int found = mFiles.size();
    for(int i=0; i<found; i++)
    {
        juce::File f = mFiles[i];
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
            
            MLFile* nf = new MLFile(f, sName, longName);
            MLFilePtr newFile(nf);
            
            // insert file into file tree
            mRoot.insert(longName, newFile);
            
            // push to index
            mFilesByIndex.push_back(newFile);
            int newIdx = mFilesByIndex.size() - 1;
            newFile->mIndex = newIdx;
        }
    }
}

// Allow the listener to process the file from the tree.
// takes zero-based index. sends one-based index and total count to the listener.
//
void MLFileCollection::processFileInTree(int i)
{
    MLFilePtr f = getFileByIndex(i);
    int size = mFilesByIndex.size();
    if(i < size)
    {
        if(mpListener)
        {
            mpListener->processFile (mName, *f, i + 1, size);
        }
    }
}

void MLFileCollection::searchForFilesNow(int delay)
{
    mSearchThread = std::tr1::shared_ptr<SearchThread>(new SearchThread(*this));
    mSearchThread->setDelay(delay);
    mSearchThread->startThread();
}

void MLFileCollection::cancelSearch()
{
    if(mSearchThread)
    {
        mSearchThread->stopThread(1000);
    }
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

// TODO intelligent re-index can be done after this.
// for now we are re-searching for all files
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

// MLFileCollection::SearchThread

// search for all files as quickly as possible
void MLFileCollection::SearchThread::run()
{
    mCollection.beginProcessFiles();
    mCollection.buildTree();
    int t = mCollection.size();
    for(int i=0; i<t; i++)
    {
        if (threadShouldExit())
            return;
        mCollection.setProperty("progress", (float)(i) / (float)t);
        mCollection.processFileInTree(i);
        wait(mDelay);
    }
    mCollection.setProperty("progress", 1.);
}

