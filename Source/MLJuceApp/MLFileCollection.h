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
	
	class Node
	{
		
	};
     
 	MLFileCollection(MLSymbol name, const File startDir, String extension);
    ~MLFileCollection();
	
    void clear();
    int getSize() const { return mFilesByIndex.size(); }
    MLSymbol getName() const { return mName; }
    const MLFile* getRoot() const { return (const_cast<const MLFile *>(&mRoot)); }
    
    void addListener (Listener* listener);
	void removeListener(Listener* pToRemove);

    void searchForFilesImmediate(int delay = 0);
    void searchForFilesInBackground(int delay = 0);
    void cancelSearch();
  
    // return a file by its path + name relative to our starting directory.
    const MLFile& getFileByName(const std::string& name);
	
    std::string getFileNameByIndex(int idx);
    const MLFile& getFileByIndex(int idx);
    const int getFileIndexByName(const std::string& fullName);

    // make a new file. TODO return const MLFile &
    const MLFilePtr createFile(const std::string& relativePath);

    // given a full system file path, get its path relative to our starting directory.
    std::string getRelativePath(const std::string& name);
    
    MLMenuPtr buildMenu(bool flat = false) const;
    void dump();
    
private:
    
    int beginProcessFiles();
    void buildTree();
    void processFileInTree(int i);
	void sendActionToListeners(MLSymbol action);

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
        }
        
        void setDelay(int d) { mDelay = d; }
        void run();
        
    private:
        int mDelay;
        MLFileCollection& mCollection;
    };
    
    // the file tree TODO will become a tree of MLFileCollection::fileNode or some such thing.
    MLFile mRoot;
	
	// files by index. TODO we own these MLFile objects, and can keep track of them more intelligently
	// without just relying on shared_ptr.
    std::vector<MLFilePtr> mFilesByIndex;
	
    MLSymbol mName;
    String mExtension;
	std::list<Listener*> mpListeners;
    
    // temp storage for processing files
    std::vector <File> mFiles;
    std::shared_ptr<SearchThread> mSearchThread;
};

typedef std::shared_ptr<MLFileCollection> MLFileCollectionPtr;

#endif 
