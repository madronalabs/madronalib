//
//  MLFileCollection.cpp
//  madronalib
//
//  Created by Randy Jones on 10/10/13.
//
//

#include "MLFileCollection.h"

using namespace ml;

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

MLFileCollection::MLFileCollection(ml::Symbol name, const File startDir, ml::TextFragment extension):
	mName(name),
	mExtension(extension),
	mProcessDelay(0)
{
	mRoot.setValue(MLFile(std::string(startDir.getFullPathName().toUTF8())));
	setProperty("progress", 0.);

	//mRunThread = std::thread(&MLFileCollection::runThread, this);
}


void MLFileCollection::runThread()
{
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
		
		juce::DirectoryIterator di (root, recurse, wildCard, whatToLookFor);
        while (di.next())
        {
			const juce::File& f = di.getFile(); 
			insertFileIntoMap(f);
			found++;
        }
    }
    else
    {
        found = -1;
    }
	return found;
}

void MLFileCollection::insertFileIntoMap(juce::File f)
{
	String shortName = f.getFileNameWithoutExtension();		
	juce::File parentDir = f.getParentDirectory();
	
	if (f.hasFileExtension(mExtension.getText()))
	{
		// insert file or directory into file tree relative to collection root
		TextFragment fullName(f.getFullPathName().toUTF8());
		TextFragment relativePath = getRelativePathFromName(fullName);
		
		// MLTEST
		juce::String fStr = f.getFileNameWithoutExtension();
		TextFragment shortName (fStr.toUTF8());
		if (shortName.lengthInCodePoints() == 1)
		{
			debug() << "insertFileIntoMap :one char: " << shortName << "\n";			
		}
		
		// MLTEST verbose
		// returnNode = mRoot.addValue(ml::Path(relativePath), MLFile(fullName.toString()));
		
		ml::Path p(relativePath);
		MLFile f(fullName.toString());
		
		mRoot.addValue(p, f);
	}
	else if (f.isDirectory())
	{
		// insert file or directory into file tree relative to collection root
		TextFragment fullName(f.getFullPathName().toUTF8());
		TextFragment relativePath = getRelativePathFromName(fullName);

		// add a null File to represent a (possibly empty) directory.
		mRoot.addValue(ml::Path(relativePath), MLFile()); 
	}
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
			const std::string depthStr = ml::textUtils::spaceStr(depth);
			const ml::Symbol itemName = it.getLeafName();
			
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
 	if(ml::within(static_cast<size_t>(i), size_t(0), mFilesByIndex.size()))
    {
		sendActionToListeners(ml::Symbol("process"), i);
    }
}

void MLFileCollection::sendActionToListeners(ml::Symbol action, int fileIndex)
{
    const MLFile& f = getFileByIndex(fileIndex);
	
	std::list<Listener*>::iterator it;
	for(it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		Listener* pL = *it;
		pL->processFileFromCollection(action, f, *this, fileIndex + 1, mFilesByIndex.size());
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
		
//		Thread::wait(mProcessDelay);
		// MLTEST
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
	
	// MLTEST
	buildIndex();
	
	sendActionToListeners("begin");
	int t = getSize();
	for(int i=0; i<t; i++)
	{
		//       if (threadShouldExit())
		
		
		// return;
		setProperty("progress", (float)(i) / (float)t);
		processFileInMap(i);
		//        wait(mProcessDelay);
	}
	setProperty("progress", 1.);
	sendActionToListeners("end");
	
	return found;
}

void MLFileCollection::processFilesInBackground(int delay)
{
    // TODO
}

void MLFileCollection::cancelProcess()
{
//	stopThread(1000);
}

std::string MLFileCollection::getFilePathByIndex(int idx)
{
    int size = mFilesByIndex.size();
	if(ml::within(idx, 0, size))
    {
		ml::TextFragment fullName = mFilesByIndex[idx].getLongName();
		return getRelativePathFromName(fullName).toString();
    }
    return std::string();
}

const MLFile MLFileCollection::getFileByIndex(int idx)
{
    int size = mFilesByIndex.size();
	if(ml::within(idx, 0, size))
    {
        return (mFilesByIndex[idx]);
    }
    return MLFile::nullObject;
}

const MLFile MLFileCollection::getFileByPath(const std::string& path)
{
	return mRoot.findValue(ml::Path(path.c_str()));
}

const int MLFileCollection::getFileIndexByPath(const std::string& path)
{
    int r = -1;
    const MLFile& f = mRoot.findValue(ml::Path(path.c_str()));

	for(int i = 0; i<mFilesByIndex.size(); ++i)
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
	std::string fullPath = std::string(mRoot.getValue().getLongName().getText()) + "/" + relativePathAndName;
    
    // insert file into file tree at relative path
	MLFile f(fullPath);
	insertFileIntoMap(f.getJuceFile());
	return f;
}

// get part of absolute path p, if any, relative to our root path, without extension.
ml::TextFragment MLFileCollection::getRelativePathFromName(const ml::TextFragment& f) const
{
	//MLTEST aagh! TODO write find and replace for TextFragments
	std::string rootName = (mRoot.getValue().getLongName().toString());
	std::string fullName(f.toString());
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
        relPath = fullName.substr(rootName.length() + 1, fullName.length() - rootName.length() - 1);
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
	
	ml::TextFragment pf(relPath.c_str());
	return (ml::textUtils::stripFileExtension(pf));
}

MLMenuPtr MLFileCollection::buildMenu(std::function<bool(FileTree::const_iterator)> includeFn) const
{
	MLMenuPtr root(new MLMenu());
	std::vector< MLMenuPtr > menuStack;
	menuStack.push_back(root);
	for(auto it = mRoot.begin(); it != mRoot.end(); ++it)
	{
		if(!it.atEndOfMap())
		{
			ml::Symbol itemName = it.getLeafName();
			if(it->isLeaf())
			{
				if(includeFn(it))
				{
					menuStack.back()->addItem(itemName.toString(), it.nodeHasValue());
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

MLMenuPtr MLFileCollection::buildMenu() const
{
	return buildMenu([=](ml::FileTree::const_iterator it){ return true; });
}

