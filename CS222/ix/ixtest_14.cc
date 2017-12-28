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
        key[4 + j] = 'a' + i - 1;
    }
    rid.pageNum = i;
    rid.slotNum = i;
}

int testCase_14(const string &indexFileName,
        const Attribute &attribute){
    // Checks whether the insertion is implemented correctly (split should happen)
    // Functions tested
    // 1. CreateIndex
    // 2. OpenIndex
    // 3. Insert entries to make root full
    // 4. Print BTree 
    // 5. Insert one more entries to watch the shape of the BTree
    // 6. CloseIndex
    // 7. DestroyIndex
    cerr << endl << "***** In IX Test Case 14 *****" << endl;

    RID rid;
    IXFileHandle ixfileHandle;
    IX_ScanIterator ix_ScanIterator;
    unsigned numOfTuples = 13;
    char key[PAGE_SIZE];
    unsigned count = attribute.length;

    // create index file
    RC rc = indexManager->createFile(indexFileName);
    assert(rc == success && "indexManager::createFile() should not fail.");

    // open index file
    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");

    // insert entry
    unsigned i = 1;
    for(; i <= numOfTuples; i++)
    {
        // Prepare a key
        prepareKeyAndRid(count, i, key, rid);

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");

        if (i == 5) {
            // print BTree, by this time the BTree should have 2 level - one root (c*) with two leaf nodes (a*b*, c*d*e*)
            cerr << endl;
            indexManager->printBtree(ixfileHandle, attribute);
            cerr << endl;
        }
    }


    // print BTree, by this time the BTree should have 3 level
    cerr << endl << endl << "////////////////////////////" << endl << endl;
    indexManager->printBtree(ixfileHandle, attribute);
    cerr << endl;

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

    const string indexEmpNameFileName = "EmpName_idx";

    Attribute attrEmpName;
    attrEmpName.length = PAGE_SIZE / 5;  // each node can only occupy 4 keys
    attrEmpName.name = "EmpName";
    attrEmpName.type = TypeVarChar;

    remove("EmpName_idx");

    RC result = testCase_14(indexEmpNameFileName, attrEmpName);
    if (result == success) {
        cerr << "***** IX Test Case 14 finished. Please check the shape of the B+ Tree. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Case 14 failed. *****" << endl;
        return fail;
    }

}



