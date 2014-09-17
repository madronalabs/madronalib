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

// a collection of files matching some kind of criteria. Uses the PropertySet interface
// to report progress for searches.

class MLFileCollection : public MLPropertySet
{
friend class SearchThread;
public:
    // a Listener class must have a processFileFromCollection routine to do something
    // with each file as it is found.
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
		// Note that idx is one-based.
		virtual void processFileFromCollection (const MLFile& file, const MLFileCollection& collection, int idx, int size) = 0;
		
	private:
		std::list<MLFileCollection*> mpCollections;
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
    const MLFilePtr getFileByName(const std::string& name);
    std::string getFileNameByIndex(int idx);
    MLFilePtr getFileByIndex(int idx);
    const int getFileIndexByName(const std::string& fullName);

    // make a new file.
    const MLFilePtr createFile(const std::string& relativePath);

    // given a full system file path, get its path relative to our starting directory.
    std::string getRelativePath(const std::string& name);
    
    MLMenuPtr buildMenu(bool flat = false) const;
    void dump();
    
private:
    
    int beginProcessFiles();
    void buildTree();
    void processFileInTree(int i);

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
    
    // the file tree
    MLFile mRoot;
    std::vector<MLFilePtr> mFilesByIndex;    
    MLSymbol mName;
    String mExtension;
	std::list<Listener*> mpListeners;
    
    // temp storage for processing files
    std::vector <File> mFiles;
    std::tr1::shared_ptr<SearchThread> mSearchThread;

};

typedef std::tr1::shared_ptr<MLFileCollection> MLFileCollectionPtr;

#endif 
