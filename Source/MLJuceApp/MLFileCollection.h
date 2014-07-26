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

// a collection of files matching some kind of criteria.
class MLFileCollection
{
public:
    
    // a Listener class must have a processFile routine to do something
    // with each file as it is found.
    class Listener
	{
	public:
		virtual ~Listener() {}
		virtual void processFile (const MLSymbol collection, const File& f, int idx) = 0;
	};
     
 	MLFileCollection(MLSymbol name, const File startDir, String extension);
    ~MLFileCollection();
    void clear();
    int size() { return mFilesByIndex.size(); }
    MLSymbol getName() { return mName; }
    const MLFile* getRoot() { return (const_cast<const MLFile *>(&mRoot)); }
    
    void setListener (Listener* listener);

    int beginProcessFiles();
    void iterateProcessFiles(int i);
    void searchForFilesNow();
    float getSearchProgress();
  
    // return a file by its path + name relative to our starting directory.
    const MLFilePtr getFileByName(const std::string& name);

    std::string getFileNameByIndex(int idx);
    MLFilePtr getFileByIndex(int idx);
    const int getFileIndexByName(const std::string& fullName);

    // make a new file.
    const MLFilePtr createFile(const std::string& relativePath);

    // given a full system file path, get its path relative to our starting directory.
    std::string getRelativePath(const std::string& name);
    
    MLMenuPtr buildMenu(bool flat = false);
    MLMenuPtr buildMenuMatchingPrefix(std::string prefix);
    void dump();
    
private:
    
    class SearchThread : public Thread
    {
    public:
        SearchThread(MLFileCollection& c) :
            Thread(String(c.getName().getString() + "_search")),
            mCollection(c),
            mProgress(0)
        {
        }
        
        ~SearchThread()
        {
            stopThread(-1);
        }
        
        void run();
        float getProgress() { return mProgress; }
        void setProgress(float p) { mProgress = p; }
        
    private:
        MLFileCollection& mCollection;
        float mProgress;
    };

    
    // the file tree
    MLFile mRoot;
    std::vector<MLFilePtr> mFilesByIndex;    
    MLSymbol mName;
    String mExtension;
    Listener* mpListener;
    
    // temp storage for processing files
    std::vector <File> mFiles;
    std::tr1::shared_ptr<SearchThread> mSearchThread;

};

typedef std::tr1::shared_ptr<MLFileCollection> MLFileCollectionPtr;

#endif 
