//
//  MLFileCollection.cpp
//  madronalib
//
//  Created by Randy Jones on 10/10/13.
//
//

#include "MLFileCollection.h"

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

#pragma mark MLFileCollection

MLFileCollection::MLFileCollection(MLSymbol name, const File startDir, String extension):
	Thread(name.getString()),
	mRoot(MLFile(std::string(startDir.getFullPathName().toUTF8()))),
	mName(name),
	mExtension(extension),
	mProcessDelay(0)
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

void MLFileCollection::addListener(Listener* pL)
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

// count the number of files in the collection, and build the node tree.
// returns the number of found files, or -1 if the file root is not usable.
//
int MLFileCollection::searchForFilesImmediate()
{
    int found = 0;
	clear();
	
	if (mRoot.getValue().exists() && mRoot.getValue().isDirectory())
    {		
        const int whatToLookFor = File::findFilesAndDirectories | File::ignoreHiddenFiles;
        const String& wildCard = "*";
        bool recurse = true;
        
		// TODO searching directories like / by mistake can take unacceptably long. Make this more
		// robust against this kind of problem. Move to our own file code.
		juce::File root = mRoot.getValue().getJuceFile();
		
        DirectoryIterator di (root, recurse, wildCard, whatToLookFor);
        while (di.next())
        {
			insertFileIntoTree(di.getFile());
			found++;
        }
    }
    else
    {
        found = -1;
    }
	return found;
}

void MLFileCollection::insertFileIntoTree(juce::File f)
{
	String shortName = f.getFileNameWithoutExtension();		
	String relativePath;
	juce::File parentDir = f.getParentDirectory();
	if(parentDir == mRoot.getValue().getJuceFile())
	{
		relativePath = "";
	}
	else
	{
		relativePath = parentDir.getRelativePathFrom(mRoot.getValue().getJuceFile());
	}

	if (f.isDirectory() || f.hasFileExtension(mExtension))
	{
		// insert file or directory into file tree relative to collection root
		std::string fullName(f.getFullPathName().toUTF8());
		std::string relativePath = getRelativePathFromName(fullName);

		mRoot.addValue(relativePath, MLFile(fullName));
	}
}

// build the linear index of files in the tree.
//
void MLFileCollection::buildIndex()
{	
	mRoot.buildLeafIndex(mFilesByIndex);
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

int MLFileCollection::processFilesImmediate(int delay)
{
	cancelProcess();
	int found = searchForFilesImmediate();
	mProcessDelay = delay;
	buildIndex();
	
	sendActionToListeners("begin");
	int t = getSize();
	for(int i=0; i<t; i++)
	{
		setProperty("progress", (float)(i) / (float)t);
		processFileInTree(i);
		Thread::wait(mProcessDelay);
	}
	setProperty("progress", 1.);
	sendActionToListeners("end");
	return found;
}

int MLFileCollection::processFiles(int delay)
{
	cancelProcess();
	int found = searchForFilesImmediate();
	mProcessDelay = delay;
	startThread(); // calls run()
	return found;
}

void MLFileCollection::processFilesInBackground(int delay)
{
    // TODO
}

void MLFileCollection::cancelProcess()
{
	stopThread(1000);
}

std::string MLFileCollection::getFilePathByIndex(int idx)
{
    int size = mFilesByIndex.size();
    if(within(idx, 0, size))
    {
		std::string fullName = mFilesByIndex[idx].getLongName();
		return getRelativePathFromName(fullName);
    }
    return std::string();
}

const MLFile MLFileCollection::getFileByIndex(int idx)
{
    int size = mFilesByIndex.size();
    if(within(idx, 0, size))
    {
        return (mFilesByIndex[idx]);
    }
    return MLFile::nullObject;
}

const MLFile MLFileCollection::getFileByPath(const std::string& path)
{
    return mRoot.findValue(path);
}

const int MLFileCollection::getFileIndexByPath(const std::string& path)
{
    int r = -1;
    const MLFile& f = mRoot.findValue(path);

	int len = mFilesByIndex.size();
	for(int i = 0; i<len; ++i)
	{
		const MLFile& g = (mFilesByIndex[i]);
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
const MLFile MLFileCollection::createFile(const std::string& relativePathAndName)
{
    std::string sName = MLStringUtils::getShortName(relativePathAndName);
    
    // need absolute path to make the Juce file
	std::string fullPath = mRoot.getValue().getLongName() + "/" + relativePathAndName;
    
    // insert file into file tree at relative path
	insertFileIntoTree(MLFile(fullPath).getJuceFile());

	// MLTEST 
	// TODO return from insertFile
    return getFileByPath(fullPath);
}

// get part of absolute path p, if any, relative to our root path, without extension.
std::string MLFileCollection::getRelativePathFromName(const std::string& f) const
{
    std::string rootName = mRoot.getValue().getLongName();
	std::string fullName = f;
    std::string relPath;
	
	// convert case in the weird scenario the user has the home directory renamed.
	// this should only do anything on English MacOS systems.
	// quick hack. If needed add lower / upper stuff to our own UTF-8 string class later.
	if(fullName.find("/Users/") == 0)
	{
		if(rootName.find("/Users/") == 0)
		{
			char cr = (rootName.c_str())[7];
			char cf = (fullName.c_str())[7];
			char crl = tolower(cr);
			char cfl = tolower(cf);
			rootName.replace(7, 1, &crl, 1);
			fullName.replace(7, 1, &cfl, 1);
		}
	}
 
    // p should begin with root. if this is true, the relative path is the
	// part of p after root.
    size_t rootPos = fullName.find(rootName);
    if(rootPos == 0)
    {
        int rLen = rootName.length();
        int pLen = fullName.length();
        relPath = fullName.substr(rLen + 1, pLen - rLen - 1);
    }
	
#ifdef ML_WINDOWS
	// convert into path format.
	// TODO make a different data type for paths! a vector of (New UTF-8) symbols.
	// all this path vs. name stuff has been confusing.
	int s = relPath.length();
	for(int c = 0 ; c < s; ++c)
	{
		if (relPath[c] == '\\')
		{
			char fwdSlash = '/';
			relPath.replace(c, 1, &fwdSlash, 1);
		}
	}
	
#endif
	
    return MLStringUtils::stripExtension(relPath);
}

// TODO all these routines have similar traversal code. factor that out into an iterator for TreeNode. 
// then callers can use the iterator instead to get all these things done.

MLMenuPtr MLFileCollection::buildMenu(MLResourceMap<MLFile> node, MLMenuPtr m) const
{
	int kids = node.getNumChildren();
	for(int i=0; i<kids; ++i)
	{
		const MLResourceMap< MLFile >& childNode = node.getChild(i);
		int grandKids = childNode.getNumChildren();
		if(grandKids)
		{
			MLMenuPtr subMenu(new MLMenu());
			buildMenu(childNode, subMenu);
			m->addSubMenu(subMenu, childNode.getValue().getShortName());
		}
		else
		{
			m->addItem(childNode.getValue().getShortName(), true);
		}
	}

	return m;
}

MLMenuPtr MLFileCollection::buildRootMenu() const
{
	MLMenuPtr m(new MLMenu());
	return buildMenu(mRoot, m);
}


// build a menu of only the files in top-level directories starting with the given prefix.
// this adds only directories, not files. Made for adding "factory" presets separately.
void MLFileCollection::buildMenuIncludingPrefix(MLMenuPtr m, std::string prefix) const
{
	int prefixLen = prefix.length();
	m->clear();
	
	int rootKids = mRoot.getNumChildren();
	for(int i=0; i<rootKids; ++i)
	{
		const MLResourceMap< MLFile >& childNode = mRoot.getChild(i);
		const MLFile& f = childNode.getValue();
		std::string filePrefix = f.getShortName().substr(0, prefixLen);
		if(filePrefix.compare(prefix) == 0)
		{
			if(f.isDirectory())
			{
				MLMenuPtr subMenu(new MLMenu());
				buildMenu(childNode, subMenu);
				m->addSubMenu(subMenu, f.getShortName());
			}
			else
			{
				m->addItem(f.getShortName());
			}
		}
	}
}

// build a menu of only the files not starting with the prefix.
void MLFileCollection::buildMenuExcludingPrefix(MLMenuPtr m, std::string prefix) const
{
	int prefixLen = prefix.length();
	m->clear();
	
	int rootKids = mRoot.getNumChildren();
	for(int i=0; i<rootKids; ++i)
	{
		const MLResourceMap< MLFile >& childNode = mRoot.getChild(i);
		const MLFile& f = childNode.getValue();
		std::string filePrefix = f.getShortName().substr(0, prefixLen);
		if(filePrefix.compare(prefix) != 0)
		{
			if(f.isDirectory())
			{
				MLMenuPtr subMenu(new MLMenu());
				buildMenu(childNode, subMenu);
				m->addSubMenu(subMenu, f.getShortName());
			}
			else
			{
				m->addItem(f.getShortName());
			}
		}
	}
}

/*
void MLFileCollection::dump() const
{
 	std::vector<MLFile>::const_iterator it;
	// mRoot->dump();
    debug() << "MLFileCollection " << mName << ":\n";
    
    int len = mFilesByIndex.size();
	for(int i = 0; i<len; ++i)
	{
        const MLFile& f = mFilesByIndex[i];
		debug() << "    " << i << ": " << f.getLongName() << "\n";
	}
}
*/

void MLFileCollection::run()
{
	buildIndex();

	sendActionToListeners("begin");
    int t = getSize();
    for(int i=0; i<t; i++)
    {
        if (threadShouldExit())
            return;
        setProperty("progress", (float)(i) / (float)t);
        processFileInTree(i);
        wait(mProcessDelay);
    }
    setProperty("progress", 1.);
	sendActionToListeners("end");
}

