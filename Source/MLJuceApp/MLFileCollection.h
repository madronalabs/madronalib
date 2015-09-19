//
//  MLFileCollection.h
//  Soundplane
//
//  Created by Randy Jones on 10/10/13.
//
//

#ifndef __MLFileCollection__
#define __MLFileCollection__

#include "JuceHeader.h"
#include "MLFile.h"
#include "MLDefaultFileLocations.h"
#include "MLMenu.h"
#include "MLProperty.h"
#include "MLStringUtils.h"

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
		virtual void processFileFromCollection (MLSymbol action, const MLFile& file, const MLFileCollection& collection, int idx, int size) = 0;

	private:
		std::list<MLFileCollection*> mpCollections;
	};
	
	// TODO this looks a lot like MLMenu::Node and should use the same Node template or object
	class TreeNode;
	
	typedef std::map<std::string, TreeNode, MLStringCompareFn> StringToNodeMapT;
	class TreeNode
	{
	public:
		TreeNode();
		TreeNode(const MLFile& f);
		~TreeNode();
		
		void clear();
		
		// insert a file into the tree, routing by path name relative to collection root.
		void insertFile(const std::string& relPath, const MLFile& f);
		
		// find a file by relative path. TODO this should use symbols.
		// Would require an unambiguous mapping from UTF-8 to symbols.
		const MLFile& find(const std::string& path);
		
		void buildMenu(MLMenuPtr m) const;
		void buildMenuIncludingPrefix(MLMenuPtr m, std::string prefix) const;
		void buildMenuExcludingPrefix(MLMenuPtr m, std::string prefix) const;

		void buildIndex(std::vector<MLFile>& index) const;

		void dump(int level = 0) const;
		
		StringToNodeMapT mChildren;
		MLFile mFile;
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
  
    // return a file by its path relative to our starting directory.
    const MLFile& getFileByPath(const std::string& path);
    const int getFileIndexByPath(const std::string& path);
	
    std::string getFilePathByIndex(int idx);
    const MLFile& getFileByIndex(int idx);

    // make a new file. TODO return const MLFile &
    const MLFile& createFile(const std::string& relativePath);

    // given a full system file name, get its path relative to our starting directory.
    std::string getRelativePathFromName(const std::string& name) const;
    
    MLMenuPtr buildMenu() const;
	
	// build a menu of only the files in top-level directories starting with the given prefix.
	// this adds only directories, not files. Made for adding "factory" presets separately.
	void buildMenuIncludingPrefix(MLMenuPtr m, std::string prefix) const;

	// build a menu of only the files not starting with the prefix.
	void buildMenuExcludingPrefix(MLMenuPtr m, std::string prefix) const;
	
    void dump() const;
    
private:
	void insertFileIntoTree(juce::File f);
	void buildIndex();
    void processFileInTree(int i);
	void sendActionToListeners(MLSymbol action, int fileIndex = -1);
	void run();
	
    TreeNode mRoot;
	
	// leaf files in collection stored by index.
    std::vector<MLFile> mFilesByIndex;
	
    MLSymbol mName;
    String mExtension;
	std::list<Listener*> mpListeners;
	int mProcessDelay;
};

typedef std::unique_ptr<MLFileCollection> MLFileCollectionPtr;

#endif 
