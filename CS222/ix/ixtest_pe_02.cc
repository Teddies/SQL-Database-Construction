#include <iostream>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "ix.h"
#include "ix_test_util.h"

IndexManager *indexManager;

void prepareKeyAndRid(const unsigned count, const unsigned i, char* key, RID &rid){
    *(int *)key = count;
    for(unsigned j = 0; j < count; j++)
    {
        *(key+4+j) = 96+i;
    }
    rid.pageNum = i;
    rid.slotNum = i;
}

int testCase_Private_Extra_2(const string &indexFileName,
        const Attribute &attribute){
    // Checks whether the deletion is properly managed (non-lazy deletion)
    cerr << endl << "***** In IX Private Extra Test Case 2 *****" << endl;

    RID rid;
    IXFileHandle ixfileHandle;
    IX_ScanIterator ix_ScanIterator;
    unsigned numOfTuples = 6;
    char key[PAGE_SIZE];
    unsigned count = attribute.length;

    // create index file
    RC rc = indexManager->createFile(indexFileName);
    assert(rc == success && "indexManager::createFile() should not fail.");

    // open index file
    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");

    // insert entries
    unsigned i = 1;
    for(; i <= numOfTuples; i++)
    {
        prepareKeyAndRid(count, i, key, rid);

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");
    }

    // print BTree, by this time the BTree should have 2 level
    //    [d]         [c]
    // [abc def] or [ab cdef]
    indexManager->printBtree(ixfileHandle, attribute);

    // delete the fifth and sixth entry
    prepareKeyAndRid(count, 5, key, rid);
    rc = indexManager->deleteEntry(ixfileHandle, attribute, key, rid);
    assert(rc == success && "indexManager::deleteEntry() should not fail.");

    // After deleting two entries (e,f):
    // [abcd]
    //
    // If lazy-deletion is applied:
    //   [d]          [c]
    // [abc d]   or [ab cd]
    prepareKeyAndRid(count, 6, key, rid);
    rc = indexManager->deleteEntry(ixfileHandle, attribute, key, rid);
    assert(rc == success && "indexManager::deleteEntry() should not fail.");

    cerr << endl << endl << "/////////////////" << endl << endl;
    indexManager->printBtree(ixfileHandle, attribute);

    // Close Index
    rc = indexManager->closeFile(ixfileHandle);
    assert(rc == success && "indexManager::closeFile() should not fail.");

    // Destroy Index
    rc = indexManager->destroyFile(indexFileName);
    assert(rc == success && "indexManager::destroyFile() should not fail.");

    return success;
}

int main()
{
    //Global Initializations
    indexManager = IndexManager::instance();

    const string indexEmpNameFileName = "private_empname_idx";

    Attribute attrEmpName;
    attrEmpName.length = PAGE_SIZE / 5;  // Each node could only have 4 children
    attrEmpName.name = "EmpName";
    attrEmpName.type = TypeVarChar;

    remove("private_empname_idx");

    RC result = testCase_Private_Extra_2(indexEmpNameFileName, attrEmpName);
    if (result == success) {
        cerr << "IX_Test Private Extra Case 2 finished. The result will be examined." << endl;
        return success;
    } else {
        cerr << "IX_Test Private Extra Case 2 failed." << endl;
        return fail;
    }

}



