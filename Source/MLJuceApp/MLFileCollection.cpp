//
//  MLFileCollection.cpp
//  madronalib
//
//  Created by Randy Jones on 10/10/13.
//
//

#include "MLFileCollection.h"

// MLFileCollection::Listener

MLFileCollection::Listener::~Listener()
{
	for(std::list<MLFileCollection*>::iterator it = mpCollections.begin(); it != mpCollections.end(); it++)
	{
		MLFileCollection* pC = *it;
		pC->removeListener(this);
	}
}

void MLFileCollection::Listener::addCollection(MLFileCollection* pC)
{
	mpCollections.push_back(pC);
}

void MLFileCollection::Listener::removeCollection(MLFileCollection* pCollectionToRemove)
{
	std::list<Listener*>::iterator it;
	for(std::list<MLFileCollection*>::iterator it = mpCollections.begin(); it != mpCollections.end(); it++)
	{
		MLFileCollection* pC = *it;
		if(pC == pCollectionToRemove)
		{
			mpCollections.erase(it);
			return;
		}
	}
}

// MLFileCollection

MLFileCollection::MLFileCollection(MLSymbol name, const File startDir, String extension):
	mRoot(startDir),
    mName(name),
    mExtension(extension),
	mSearchThread(new SearchThread(*this))
{
    setProperty("progress", 0.);
}

MLFileCollection::~MLFileCollection()
{
	for(std::list<Listener*>::iterator it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		Listener* pL = *it;
		pL->removeCollection(this);
	}
}

void MLFileCollection::clear()
{
    mRoot.clear();
    mFilesByIndex.clear();
}

void MLFileCollection::addListener (Listener* pL)
{
    mpListeners.push_back(pL);
	pL->addCollection(this);
}

void MLFileCollection::removeListener(Listener* pToRemove)
{
	std::list<Listener*>::iterator it;
	for(it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		Listener* pL = *it;
		if(pL == pToRemove)
		{
			mpListeners.erase(it);
			return;
		}
	}
}

// count the number of files in the collection, and begin making the collection.
// returns the number of found files, or -1 if the file root is not usable.
//
int MLFileCollection::beginProcessFiles()
{
    int found = 0;
	
	if (mRoot.getJuceFile().exists() && mRoot.getJuceFile().isDirectory())
    {		
		mFiles.clear();
        const int whatToLookFor = File::findFilesAndDirectories | File::ignoreHiddenFiles;
        const String& wildCard = "*";
        bool recurse = true;
        
		// TODO searching directories like / by mistake can take unacceptably long. Make this more
		// robust against this kind of problem.
        DirectoryIterator di (mRoot.getJuceFile(), recurse, wildCard, whatToLookFor);
        while (di.next())
        {
            mFiles.push_back(di.getFile());
        }
        found = mFiles.size();
    }
    else
    {
        found = -1;
    }

	// MLTEST
	debug() << "MLFileCollection: found " << found << "files.\n";
    return found;
}

// examine all files in mFiles and build indexed tree
//
void MLFileCollection::buildTree()
{
    int found = mFiles.size();
	mFilesByIndex.clear();
	
    for(int i=0; i<found; i++)
    {
        juce::File f = mFiles[i];
        String shortName = f.getFileName();
        String relativePath;
        File parentDir = f.getParentDirectory();
        if(parentDir == mRoot.getJuceFile())
        {
            relativePath = "";
        }
        else
        {
            relativePath = parentDir.getRelativePathFrom(mRoot.getJuceFile());
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
            mRoot.insertFile(longName, newFile);
            
            // push to index
            mFilesByIndex.push_back(newFile);
        }
    }
}

// Allow the listener to process the file from the tree.
// takes zero-based index. sends one-based index and total count to the listener.
//
void MLFileCollection::processFileInTree(int i)
{
    int size = mFilesByIndex.size();
    if(within(i, 0, size))
    {
		sendActionToListeners(MLSymbol("process"), i);
    }
}

void MLFileCollection::sendActionToListeners(MLSymbol action, int fileIndex)
{
    int size = mFilesByIndex.size();
    const MLFile& f = getFileByIndex(fileIndex);
	
	std::list<Listener*>::iterator it;
	for(it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		Listener* pL = *it;
		pL->processFileFromCollection (action, f, *this, fileIndex + 1, size);
	}
}

void MLFileCollection::searchForFilesImmediate(int delay)
{
    mSearchThread->setDelay(delay);
    mSearchThread->startThread();
}

void MLFileCollection::searchForFilesInBackground(int delay)
{
    // TODO
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
        return mFilesByIndex[idx]->getLongName();
    }
    return std::string();
}

const MLFile& MLFileCollection::getFileByIndex(int idx)
{
    int size = mFilesByIndex.size();
    if(within(idx, 0, size))
    {
        return *(mFilesByIndex[idx]);
    }
    return MLFile::nullObject;
}

const MLFile& MLFileCollection::getFileByName(const std::string& fullName)
{
    return mRoot.find(fullName);
}

const int MLFileCollection::getFileIndexByName(const std::string& fullName)
{
    int r = -1;
    const MLFile& f = mRoot.find(fullName);

	int len = mFilesByIndex.size();
	for(int i = 0; i<len; ++i)
	{
		const MLFile& g = *(mFilesByIndex[i]);
		if(f == g)
		{
			r = i;                
			break;
		}
	}
    return r;
}

// TODO intelligent re-index and update can be done after this.
// for now we are re-searching for all files
//
const MLFilePtr MLFileCollection::createFile(const std::string& relativePathAndName)
{
    std::string sName = MLStringUtils::getShortName(relativePathAndName);
    std::string fullPath = mRoot.getAbsolutePath() + "/" + relativePathAndName;
    
    // need absolute path to make the Juce file
    File *f = new File(String(CharPointer_UTF8(fullPath.c_str())));
    MLFilePtr newFile(new MLFile(*f, sName, relativePathAndName));
    
    // insert file into file tree at relative path
    mRoot.insertFile(relativePathAndName, newFile);

    return newFile;
}

// get part of absolute path p, if any, relative to our root path.
std::string MLFileCollection::getRelativePath(const std::string& p)
{
    std::string rootPath = mRoot.getAbsolutePath();
	std::string filePath = p;
    std::string relPath;
	
	// convert case in the weird scenario the user has the home directory renamed.
	// this should only do anything on English MacOS systems.
	// quick hack. If needed add lower / upper stuff to our own UTF-8 string class later.
	if(p.find("/Users/") == 0)
	{
		if(rootPath.find("/Users/") == 0)
		{
			char cr = (rootPath.c_str())[7];
			char cf = (filePath.c_str())[7];
			char crl = tolower(cr);
			char cfl = tolower(cf);
			rootPath.replace(7, 1, &crl, 1);
			filePath.replace(7, 1, &cfl, 1);
		}
	}
 
    // p should begin with root. if this is true, the relative path is the
	// part of p after root.
    size_t rootPos = filePath.find(rootPath);
    if(rootPos == 0)
    {
        int rLen = rootPath.length();
        int pLen = filePath.length();
        relPath = filePath.substr(rLen + 1, pLen - rLen - 1);
    }
    return relPath;
}

MLMenuPtr MLFileCollection::buildMenu(bool flat) const
{
    // make a new menu named after this collection and containing all of the files in it.
    MLMenuPtr m(new MLMenu(mName));
    int size = mFilesByIndex.size();
    if(flat)
    {
        for(int i=0; i<size; ++i)
        {
            MLFilePtr f = mFilesByIndex[i];
            m->addItem(f->getShortName());
        }
    }
    else
    {
        mRoot.buildMenu(m);
    }
    return m;
}

void MLFileCollection::dump() const
{
 	std::vector<MLFilePtr>::const_iterator it;
    
    debug() << "MLFileCollection " << mName << ":\n";
    
    int len = mFilesByIndex.size();
	for(int i = 0; i<len; ++i)
	{
        const MLFilePtr f = mFilesByIndex[i];
		debug() << "    " << i << ": " << f->getShortName() << "\n";
	}
}

// MLFileCollection::SearchThread

// search for all files as quickly as possible
void MLFileCollection::SearchThread::run()
{
    mCollection.beginProcessFiles();
    mCollection.buildTree();
	mCollection.sendActionToListeners("begin");
    int t = mCollection.getSize();
    for(int i=0; i<t; i++)
    {
        if (threadShouldExit())
            return;
        mCollection.setProperty("progress", (float)(i) / (float)t);
        mCollection.processFileInTree(i);
        wait(mDelay);
    }
    mCollection.setProperty("progress", 1.);
	mCollection.sendActionToListeners("end");
}

