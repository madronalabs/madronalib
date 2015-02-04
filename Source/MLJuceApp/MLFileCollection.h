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

class MLFileCollection : public MLPropertySet
{
friend class SearchThread;
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

 	MLFileCollection(MLSymbol name, const File startDir, String extension);
    ~MLFileCollection();
	
    void clear();
    int getSize() const { return mFilesByIndex.size(); }
    MLSymbol getName() const { return mName; }
    //const MLFile* getRoot() const { return (const_cast<const MLFile *>(&mRoot)); }
    
    void addListener (Listener* listener);
	void removeListener(Listener* pToRemove);

    void searchForFilesImmediate(int delay = 0);
    void searchForFilesInBackground(int delay = 0);
    void cancelSearch();
  
    // return a file by its path + name relative to our starting directory.
    const MLFile& getFileByName(const std::string& name);
	
    std::string getFilePathByIndex(int idx);
    const MLFile& getFileByIndex(int idx);
    const int getFileIndexByPath(const std::string& fullName);

    // make a new file. TODO return const MLFile &
    const MLFilePtr createFile(const std::string& relativePath);

    // given a full system file path, get its path relative to our starting directory.
    std::string getRelativePath(const std::string& name);
    
    MLMenuPtr buildMenu(bool flat = false) const;
	
	// build a menu of only the files in top-level directories starting with the given prefix.
	// this adds only directories, not files. Made for adding "factory" presets separately.
	void buildMenuIncludingPrefix(MLMenuPtr m, std::string prefix) const;

	// build a menu of only the files not starting with the prefix.
	void buildMenuExcludingPrefix(MLMenuPtr m, std::string prefix) const;
	
    void dump() const;
    
private:
    int beginProcessFiles();
    void buildTree();
    void processFileInTree(int i);
	
	void sendActionToListeners(MLSymbol action, int fileIndex = -1);

    class SearchThread : public Thread
    {
    public:
        SearchThread(MLFileCollection& c) :
            Thread(String(c.getName().getString() + "_search")),
            mCollection(c),
            mDelay(0)
        {
        }
        
        ~SearchThread()
        {
			stopThread(100);
        }
        
        void setDelay(int d) { mDelay = d; }
        void run();
        
    private:
        MLFileCollection& mCollection;
        int mDelay;
    };
	
	// TODO this looks a lot like MLMenu::Node and should use the same Node template
	
	class TreeNode;
    typedef std::tr1::shared_ptr<TreeNode> TreeNodePtr;
    typedef std::map<std::string, TreeNodePtr, MLStringCompareFn> StringToNodeMapT;
    class TreeNode
    {
	public:
		TreeNode(MLFilePtr f);
		~TreeNode();
		
		void clear();
		
		// insert a file into the tree, routing by path name relative to collection root.
		void insertFile(const std::string& relPath, MLFilePtr f);
		
		// find a file by relative path. TODO this should use symbols.
		// Would require an unambiguous mapping from UTF-8 to symbols.
		const MLFile& find(const std::string& path);
		
		void buildMenu(MLMenuPtr m, int level = 0) const;
		void buildMenuIncludingPrefix(MLMenuPtr m, std::string prefix) const;
		void buildMenuExcludingPrefix(MLMenuPtr m, std::string prefix) const;

		StringToNodeMapT mChildren;
		MLFilePtr mFile;
	};

    TreeNodePtr mRoot;
	
	// leaf files in collection stored by index.
    std::vector<MLFilePtr> mFilesByIndex;
	
    MLSymbol mName;
    String mExtension;
	std::list<Listener*> mpListeners;
    
    // temp storage for processing files
    std::vector <juce::File> mFilesToProcess;
    std::tr1::shared_ptr<SearchThread> mSearchThread;
};

#endif 
