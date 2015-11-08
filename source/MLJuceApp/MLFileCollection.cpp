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
	mName(name),
	mExtension(extension),
	mProcessDelay(0)
{
	mRoot.setValue(MLFile(std::string(startDir.getFullPathName().toUTF8())));
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
			insertFileIntoMap(di.getFile());
			found++;
        }
    }
    else
    {
        found = -1;
    }
	return found;
}

MLResourceMap<std::string, MLFile>* MLFileCollection::insertFileIntoMap(juce::File f)
{
	MLResourceMap<std::string, MLFile>* returnNode = nullptr; 
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

	if (f.hasFileExtension(mExtension))
	{
		// insert file or directory into file tree relative to collection root
		std::string fullName(f.getFullPathName().toUTF8());
		std::string relativePath = getRelativePathFromName(fullName);
		returnNode = mRoot.addValue(relativePath, MLFile(fullName));
	}
	else if (f.isDirectory())
	{
		// insert file or directory into file tree relative to collection root
		std::string fullName(f.getFullPathName().toUTF8());
		std::string relativePath = getRelativePathFromName(fullName);

		// add a value-less node to represent a (possibly empty) directory.
		returnNode = mRoot.addNode(relativePath); 
	}
	return returnNode;
}

// build the linear index of files in the tree.
//
void MLFileCollection::buildIndex()
{	
	mFilesByIndex.clear();

	for (auto it = mRoot.begin(); it != mRoot.end(); ++it)
	{
		if(it.nodeHasValue())
		{
		//	std::cout << "buildIndex adding " << it->getValue().getShortName() << "\n";
			mFilesByIndex.push_back(it->getValue());
		}
	}
}

void MLFileCollection::dump() const
{	
	for(auto it = mRoot.begin(); it != mRoot.end(); ++it)
	{
		if(!it.atEndOfMap())
		{
			int depth = it.getDepth();
			const std::string depthStr = ml::stringUtils::spaceStr(depth);
			const std::string& itemName = it.getLeafName();
			
			if(it.nodeHasValue())
			{
				debug() << depthStr << "file: " << itemName << "\n";
			}
			else
			{
				debug() << depthStr << "dir level " << depth << ": " << itemName << "\n";
			}
		}
		else
		{
			debug() << "end\n";
		}
	}
}

// Allow the listener to process the file from the tree.
// takes zero-based index. sends one-based index and total count to the listener.
//
void MLFileCollection::processFileInMap(int i)
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
		processFileInMap(i);
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
    // for now, need absolute path to make the Juce file
	std::string fullPath = mRoot.getValue().getLongName() + "/" + relativePathAndName;
    
    // insert file into file tree at relative path
	return insertFileIntoMap(MLFile(fullPath).getJuceFile())->getValue();
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
	
    return ml::stringUtils::stripExtension(relPath);
}

MLMenuPtr MLFileCollection::buildMenu() const
{
	return buildMenu([=](MLResourceMap<std::string, MLFile>::const_iterator it){ return true; });
}

MLMenuPtr MLFileCollection::buildMenu(std::function<bool(MLResourceMap<std::string, MLFile>::const_iterator)> includeFn) const
{
	MLMenuPtr root(new MLMenu());
	std::vector<MLMenuPtr> menuStack;
	menuStack.push_back(root);
	for(auto it = mRoot.begin(); it != mRoot.end(); ++it)
	{
		if(!it.atEndOfMap())
		{
			const std::string& itemName = it.getLeafName();
			if(it->isLeaf())
			{
				if(includeFn(it))
				{
					menuStack.back()->addItem(itemName, it.nodeHasValue());
				}
			}
			else
			{
				// add submenu at current depth
				MLMenuPtr newMenu (new MLMenu(itemName));
				if(includeFn(it))
				{
					menuStack.back()->addSubMenu(newMenu);
				}
				menuStack.push_back(newMenu);
			}
		}
		else
		{
			// note that *it will not have a valid value here!
			MLMenuPtr popped = menuStack.back();
			menuStack.pop_back();
		}
	}
	return root;
}

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
        processFileInMap(i);
        wait(mProcessDelay);
    }
    setProperty("progress", 1.);
	sendActionToListeners("end");
}

