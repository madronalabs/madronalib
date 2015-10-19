//
//  MLFileCollection.h
//  Soundplane
//
//  Created by Randy Jones on 10/10/13.
//
//

#ifndef __MLFileCollection__
#define __MLFileCollection__

#include <functional>

#include "JuceHeader.h"
#include "MLFile.h"
#include "MLDefaultFileLocations.h"
#include "MLMenu.h"
#include "MLProperty.h"
#include "MLStringUtils.h"
#include "MLResourceMap.h"

// a collection of files matching some kind of criteria. Uses the PropertySet interface
// to report progress for searches.

class MLFileCollection :
	public MLPropertySet,
	private Thread
{
public:
    class Listener
	{
		friend class MLFileCollection;
	public:
		Listener(){}
		virtual ~Listener();
		void addCollection(MLFileCollection* pC);
		void removeCollection(MLFileCollection* pCollectionToRemove);
		
		// process a file from the Collection. In an immediate search, first the files will be counted, then
		// this will be called for each file. So idx and size can be used to display progress, or take action
		// after the last file is processed. In a background search, idx may equal size more often as files
		// are discovered, and so the post-processing steps (building menus for example) may take place
		// more often.
		//
		// Possible actions are:
		// begin: Collection is about to send all files.
		// process: Called for each file.
		// update: Called in a background search when a file's content has been changed.
		// end: All files recently changed have been transmitted.
		//
		// Note that idx is one-based.
		virtual void processFileFromCollection (MLSymbol action, const MLFile file, const MLFileCollection& collection, int idx, int size) = 0;

	private:
		std::list<MLFileCollection*> mpCollections;
	};
	
	MLFileCollection(MLSymbol name, const File startDir, String extension);
    ~MLFileCollection();

	void clear();
    int getSize() const { return mFilesByIndex.size(); }
    MLSymbol getName() const { return mName; }
    //const MLFile* getRoot() const { return (const_cast<const MLFile *>(&mRoot)); }
    
    void addListener(Listener* listener);
	void removeListener(Listener* pToRemove);

	// search for files in the collection, traversing the entire directory tree before
	// returning. returns the number of files found. 
	int searchForFilesImmediate();

	// search for and process all files on the calling thread, with the given delay between files.
	int processFilesImmediate(int delay = 0);
	
	// blocks while discovering all files in the collection, then starts the process thread with
	// the given delay between files. returns the number of files found.
	int processFiles(int delay = 0);
	
	// runs process thread in the background, to monitor any changes to the collection.
	// the process thread keeps running and any changes are processed until cancelProcess()
	// is called.
	// UNIMPLEMENTED
    void processFilesInBackground(int delay = 0);
	
	// will cancel the process thread started by either processFiles() or processFilesInBackground().
    void cancelProcess();

	// insert a file into the collection, routing by path name relative to collection root.
	const MLFile& insertFile(const std::string& relPath, const MLFile& f);
	
    // return a file by its path relative to our starting directory.
    const MLFile getFileByPath(const std::string& path);
    const int getFileIndexByPath(const std::string& path);
	
    std::string getFilePathByIndex(int idx);
    const MLFile getFileByIndex(int idx);

    // make a new file. 
    const MLFile createFile(const std::string& relativePath);

    // given a full system file name, get its path relative to our starting directory.
    std::string getRelativePathFromName(const std::string& name) const;
    
	// build a menu of all the files. 
	// TODO no reason to know about menus here. We should be returning a raw tree structure.
	// or better, the menu just has a reference to a resourceMap, that can be constantly updating itself.
	MLMenuPtr buildMenu() const;
	
	// build a menu of the files for which the function returns true.
	MLMenuPtr buildMenu(std::function<bool(MLResourceMap<std::string, MLFile>::const_iterator)>) const;
	
    void dump() const;
    
private:
	
	MLResourceMap<std::string, MLFile>* insertFileIntoMap(juce::File f);
	void buildIndex();
    void processFileInMap(int i);
	void sendActionToListeners(MLSymbol action, int fileIndex = -1);
	void run();
	
	MLResourceMap<std::string, MLFile> mRoot;
	
	// leaf files in collection stored by index.
    std::vector<MLFile> mFilesByIndex;
	
    MLSymbol mName;
    String mExtension;
	std::list<Listener*> mpListeners;
	int mProcessDelay;
};

#endif 
