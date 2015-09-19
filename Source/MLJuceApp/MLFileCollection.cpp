//
//  MLFileCollection.cpp
//  madronalib
//
//  Created by Randy Jones on 10/10/13.
//
//

#include "MLFileCollection.h"

//const MLFileCollection MLFileCollection::nullObject;

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

// MLFileCollection::TreeNode

MLFileCollection::TreeNode::TreeNode() :
mFile()
{
}

MLFileCollection::TreeNode::TreeNode(const MLFile& f) :
mFile(f)
{
}

MLFileCollection::TreeNode::~TreeNode()
{
}

void MLFileCollection::TreeNode::clear()
{
    mChildren.clear();
}

void MLFileCollection::TreeNode::insertFile(const std::string& path, const MLFile& f)
{
    int len = path.length();
    if(len)
    {
        int b = path.find_first_of("/");
        if(b == std::string::npos)
        {
            // add file node to map
            mChildren[path] = MLFileCollection::TreeNode(f);
        }
        else
        {
            std::string firstDir = path.substr(0, b);
            std::string restOfDirs = path.substr(b + 1, len - b);
            
            // find or add first dir
            if(firstDir == "")
            {
                debug() << "MLFile::insert: empty directory name!\n";
            }
            else
            {
                if(mChildren.find(firstDir) == mChildren.end())
                {
                    mChildren[firstDir] = MLFileCollection::TreeNode(f);
                }
                mChildren[firstDir].insertFile(restOfDirs, f);
            }
        }
    }
    else
    {
        debug() << "MLFile::insert: empty file name!\n";
    }
}

// TODO use MLPath
const MLFile& MLFileCollection::TreeNode::find(const std::string& path)
{
    int len = path.length();
    if(len)
    {
        int b = path.find_first_of("/");
        if(b == std::string::npos)
        {
			// end of path, this should be the file name.
			StringToNodeMapT::const_iterator it = mChildren.find(path);
			if(it != mChildren.end())
			{
				// return the found file.
				return (it->second.mFile);
			}
			else
			{
				// something went wrong
				debug() << "ERROR: MLFileCollection::TreeNode::find: " << path << " not found in file tree.\n";
				return MLFile::nullObject;
			}
        }
        else
        {
            std::string firstDir = path.substr(0, b);
            std::string restOfDirs = path.substr(b + 1, len - b);
			
            // find file matching first dir
            if(firstDir == "")
            {
                debug() << "MLFileCollection::TreeNode::find: empty directory name!\n";
            }
            else if(mChildren.find(firstDir) != mChildren.end())
            {
                // look for rest of dirs in found non-leaf file
                return mChildren[firstDir].find(restOfDirs);
            }
            else
            {
				debug() << "MLFileCollection: file not found.\n";
                return MLFile::nullObject;
            }
        }
    }
    else
    {
        debug() << "MLFile::find: empty file name!\n";
    }
    return MLFile::nullObject;
}

// TODO all these routines have similar traversal code. factor that out into an iterator for TreeNode. 
// then callers can use the iterator instead to get all these things done.

void MLFileCollection::TreeNode::buildMenu(MLMenuPtr m) const
{
	m->clear();
	StringToNodeMapT::const_iterator it;
	for(it = mChildren.begin(); it != mChildren.end(); ++it)
	{
		const TreeNode& node(it->second);
		const MLFile& f (node.mFile);
		
		if(f.exists())
		{
			if(f.isDirectory())
			{
				MLMenuPtr subMenu(new MLMenu());
				node.buildMenu(subMenu);
				
				// TODO menu should be based on path, not file name?
				m->addSubMenu(subMenu, f.getShortName());
			}
			else
			{
				m->addItem(f.getShortName());
			}
		}
	}
}

void MLFileCollection::TreeNode::buildIndex(std::vector<MLFile>& index) const
{
	StringToNodeMapT::const_iterator it;
	
	for(it = mChildren.begin(); it != mChildren.end(); ++it)
	{
		const TreeNode& node = it->second;
		const MLFile& f = node.mFile;
		
		if(f.exists())
		{
			if(f.isDirectory())
			{
				node.buildIndex(index);
			}
			else
			{
				// push any leaves into node
				index.push_back(f);
			}
		}
	}
}

void MLFileCollection::TreeNode::dump(int level) const
{	
	StringToNodeMapT::const_iterator it;

	for(it = mChildren.begin(); it != mChildren.end(); ++it)
    {
        const std::string& p = it->first;
        const TreeNode& n = it->second;

		debug() << level << ": " << spaceStr(level) << p << "\n";

		n.dump(level + 1);
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
	
	if (mRoot.mFile.exists() && mRoot.mFile.isDirectory())
    {		
        const int whatToLookFor = File::findFilesAndDirectories | File::ignoreHiddenFiles;
        const String& wildCard = "*";
        bool recurse = true;
        
		// TODO searching directories like / by mistake can take unacceptably long. Make this more
		// robust against this kind of problem. Move to our own file code.
		juce::File root = mRoot.mFile.getJuceFile();
		
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
	if(parentDir == mRoot.mFile.getJuceFile())
	{
		relativePath = "";
	}
	else
	{
		relativePath = parentDir.getRelativePathFrom(mRoot.mFile.getJuceFile());
	}

	if (f.isDirectory() || f.hasFileExtension(mExtension))
	{
		// insert file or directory into file tree relative to collection root
		std::string fullName(f.getFullPathName().toUTF8());
		std::string relativeName = getRelativePathFromName(fullName);

		mRoot.insertFile(relativeName, MLFile(fullName));
	}
}

// build the linear index of files in the tree.
//
void MLFileCollection::buildIndex()
{	
	mRoot.buildIndex(mFilesByIndex);
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

const MLFile& MLFileCollection::getFileByIndex(int idx)
{
    int size = mFilesByIndex.size();
    if(within(idx, 0, size))
    {
        return (mFilesByIndex[idx]);
    }
    return MLFile::nullObject;
}

const MLFile& MLFileCollection::getFileByPath(const std::string& path)
{
    return mRoot.find(path);
}

const int MLFileCollection::getFileIndexByPath(const std::string& path)
{
    int r = -1;
    const MLFile& f = mRoot.find(path);

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
const MLFile& MLFileCollection::createFile(const std::string& relativePathAndName)
{
    std::string sName = MLStringUtils::getShortName(relativePathAndName);
    
    // need absolute path to make the Juce file
	std::string fullPath = mRoot.mFile.getLongName() + "/" + relativePathAndName;
    
    // insert file into file tree at relative path
    mRoot.insertFile(relativePathAndName, MLFile(fullPath));

	// TODO return from insertFile
    return getFileByPath(fullPath);
}

// get part of absolute path p, if any, relative to our root path, without extension.
std::string MLFileCollection::getRelativePathFromName(const std::string& f) const
{
    std::string rootName = mRoot.mFile.getLongName();
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

// make a new menu named after this collection and containing all of the files in it.
MLMenuPtr MLFileCollection::buildMenu() const
{
    MLMenuPtr m(new MLMenu(mName));
	mRoot.buildMenu(m);
    return m;
}

// build a menu of only the files in top-level directories starting with the given prefix.
// this adds only directories, not files. Made for adding "factory" presets separately.
void MLFileCollection::buildMenuIncludingPrefix(MLMenuPtr m, std::string prefix) const
{
    int prefixLen = prefix.length();
    m->clear();
    
    StringToNodeMapT::const_iterator it;
    for(it = mRoot.mChildren.begin(); it != mRoot.mChildren.end(); ++it)
    {
        const TreeNode& n = it->second;
        const MLFile& f = n.mFile;
        std::string filePrefix = f.getShortName().substr(0, prefixLen);
        if(filePrefix.compare(prefix) == 0)
        {
            if(f.isDirectory())
            {
                MLMenuPtr subMenu(new MLMenu());
                n.buildMenu(subMenu);
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
    
    StringToNodeMapT::const_iterator it;
    for(it = mRoot.mChildren.begin(); it != mRoot.mChildren.end(); ++it)
    {
        const TreeNode& n = it->second;
        const MLFile& f = n.mFile;
        std::string filePrefix = f.getShortName().substr(0, prefixLen);
        if(filePrefix.compare(prefix) != 0)
        {
            if(f.isDirectory())
            {
                MLMenuPtr subMenu(new MLMenu());
                n.buildMenu(subMenu);
                m->addSubMenu(subMenu, f.getShortName());
            }
            else
            {
                m->addItem(f.getShortName());
            }
        }
    }
}

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

