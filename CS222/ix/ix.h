#ifndef _ix_h_

#define _ix_h_


#include <vector>

#include <string>


#include "../rbf/rbfm.h"


# define IX_EOF (-1)  // end of the index scan


class IX_ScanIterator;

class IXFileHandle;


class IndexManager : PagedFileManager {


    public:

        static IndexManager* instance();


        // Create an index file.

        RC createFile(const string &fileName);


        // Delete an index file.

        RC destroyFile(const string &fileName);


        // Open an index and return an ixfileHandle.

        RC openFile(const string &fileName, IXFileHandle &ixfileHandle);


        // Close an ixfileHandle for an index.

        RC closeFile(IXFileHandle &ixfileHandle);


        // Conduct the comparison between two string, return true if the index on tree is greater than the inserted key

        RC varcharComparison(void *memory, const void *key, const short int beginOffset,

                            const short int endOffset, bool &comparison, bool &equal);


        // Search the appropriate leaf page number in the non-leaf index pages

        RC searchInNonLeaf(IXFileHandle &ixfileHandle, void *memory, PageNum &page, char &leafIndicator,

                           const AttrType type, const void *key, vector<PageNum> &trace, bool flag, int &index);

        RC searchInLeaf(void *memory, const AttrType &type, const void *key, short int &moveBegin, short int &moveEnd, int &index);


        // Split the leaf pages

        RC leafSplit(IXFileHandle &ixfileHandle, void *memory, void *newPage, AttrType type, vector<PageNum> &trace, void *midKey);


        // Split the non-leaf pages

        RC indexSplit(IXFileHandle &ixfileHandle, void *memory, void *newPage, AttrType type,

          vector<PageNum> &trace, void *leftPageLastKey, void *rightPageFirstKey);


        // Insert a new node on a given leaf page, the page is guaranteed to have the space

        RC insertOnLeafPage(void *memory, const void *key, AttrType type, const RID &rid);


        // Insert a new node on a given non-leaf page, the page is guaranteed to have the space

        RC insertOnIndexPage(IXFileHandle &ixfileHandle, void *memory, const void *key, AttrType type, PageNum newPageNumber);


        // Update a non-leaf page and delete a redundant node

        RC updateIndexPage(IXFileHandle &ixfileHandle, void *memory, const void *key, AttrType type, void *midKey, bool leftOrRight, PageNum newPageNumber);


        // Generate a new root page for the tree

        RC generateRoot(IXFileHandle &ixfileHandle, void *midKey, AttrType type, PageNum left, PageNum right);


        // Compare two keys with void* type

        bool comp(const void *m1, void *m2, AttrType type);


        // Insert an entry into the given index that is indicated by the given ixfileHandle.

        RC insertEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid);

        RC insertEntry2(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid, PageNum &p);


        RC deleteOnLeafPage(void *memory, short int moveBegin, short int moveEnd, int len, int index, AttrType type);


        // Delete an entry from the given index that is indicated by the given ixfileHandle.

        RC deleteEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid);


        // Initialize and IX_ScanIterator to support a range search

        RC scan(IXFileHandle &ixfileHandle,

                const Attribute &attribute,

                const void *lowKey,

                const void *highKey,

                bool lowKeyInclusive,

                bool highKeyInclusive,

                IX_ScanIterator &ix_ScanIterator);


        // Print the B+ tree in pre-order (in a JSON record format)

        void printBtree(IXFileHandle &ixfileHandle, const Attribute &attribute) const;


        string serializeTree(IXFileHandle &ixfileHandle, const AttrType type, PageNum root, void *memory) const;


    protected:

        IndexManager();

        ~IndexManager();


    private:

        static IndexManager *_index_manager;

};



class IX_ScanIterator {

    public:
		AttrType type;

// Constructor

        IX_ScanIterator();


        // Destructor

        ~IX_ScanIterator();


        //save the RID (heap file index) of each satisfied leaf entry

        vector<RID> returnedRID;

        //save the key of each satisfied leaf entry

        vector<vector<char> > returnedKey;


        // Get next matching entry

        RC getNextEntry(RID &rid, void *key);


        // Terminate index scan

        RC close();


    private:

        int index;

};




class IXFileHandle {

    public:


    // variables to keep counter for each operation

    unsigned ixReadPageCounter;

    unsigned ixWritePageCounter;

    unsigned ixAppendPageCounter;

    string indexFileName;

FILE *ifp;


    // Constructor

    IXFileHandle();


    // Destructor

    ~IXFileHandle();


// Put the current counter values of associated PF FileHandles into variables

RC collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount);


    RC readPage(PageNum pageNum, void *data);                             // Get a specific page

    RC writePage(PageNum pageNum, const void *data);                      // Write a specific page

    RC appendPage(const void *data);                                      // Append a specific page

    unsigned getNumberOfPages();

    FileHandle fileHandle;

};


#endif
