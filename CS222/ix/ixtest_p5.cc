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

int testCase_p5(const string &indexFileName, 
        const Attribute &attribute){

    // Checks whether leaves are linked and the way of conducting search is correct.
    cerr << endl << "***** In IX Test Private Case 5 *****" << endl;

    RID rid;
    IXFileHandle ixfileHandle;
    IX_ScanIterator ix_ScanIterator;
    unsigned numOfTuples = 7;
    char key[PAGE_SIZE];
    unsigned count = attribute.length;

    // create index files
    RC rc = indexManager->createFile(indexFileName);
    assert(rc == success && "indexManager::createFile() should not fail.");

    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");

    // insert entry
    unsigned i = 1;
    for(; i <= numOfTuples; i++)
    {
        prepareKeyAndRid(count, i * 10, key, rid);

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");
    }

    // insert the 8th
    prepareKeyAndRid(count, i++ * 10 , key, rid);
    rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
    assert(rc == success && "indexManager::insertEntry() should not fail.");
    
    // print BTree, by this time the BTree should have 2 or 3 level 
    // depend on the design of your root
    indexManager->printBtree(ixfileHandle, attribute);

    // insert the 9th
    prepareKeyAndRid(count, i++ * 10, key, rid);
    rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
    assert(rc == success && "indexManager::insertEntry() should not fail.");
    
    unsigned readPageCountInsert = 0;
    unsigned writePageCountInsert = 0;
    unsigned appendPageCountInsert = 0;

    unsigned readPageCountScan = 0;
    unsigned writePageCountScan = 0;
    unsigned appendPageCountScan = 0;

    // collect counters
    rc = ixfileHandle.collectCounterValues(readPageCountInsert, writePageCountInsert, appendPageCountInsert);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");

    cerr << "After Insertion - R:" << readPageCountInsert << " W:" << writePageCountInsert << " A:" << appendPageCountInsert << endl;

    rc = indexManager->scan(ixfileHandle, attribute, NULL, NULL, true, true, ix_ScanIterator);
    assert(rc == success && "indexManager::scan() should not fail.");

    rc = ixfileHandle.collectCounterValues(readPageCountScan, writePageCountScan, appendPageCountScan);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");

    cerr << "After Initialization of Scan - R:" << readPageCountScan << " W:" << writePageCountScan << " A:" << appendPageCountScan << endl;
 
    count = 0;
    while(ix_ScanIterator.getNextEntry(rid, &key) == success)
    {
        cerr << "Returned rid:" << rid.pageNum << "," << rid.slotNum << endl;
        count++;
    }

    rc = ixfileHandle.collectCounterValues(readPageCountScan, writePageCountScan, appendPageCountScan);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");

    cerr << "After Iteration - R:" << readPageCountScan << " W:" << writePageCountScan << " A:" << appendPageCountScan << endl;

    unsigned roughLeafReadCount = readPageCountScan - readPageCountInsert;
    // If the B+Tree index is 3 level: 3 I/O + 9 scan I/O per entry at maximum  = 12 
    // If the B+Tree index is 2 level: 2 I/O + 9 scan I/O per entry at maximum  = 11 
    if (roughLeafReadCount > 12){
        cerr << "Too many read I/Os for scan: " << roughLeafReadCount << ", the leaf nodes should be linked." << endl;
        cerr << "Check the print out B+ Tree to validate the pages" << endl;
        indexManager->printBtree(ixfileHandle, attribute);
        goto error_close_index;
    }
    

    // Close index file
    rc = indexManager->closeFile(ixfileHandle);
    assert(rc == success && "indexManager::closeFile() should not fail.");

    // Destroy Index
    rc = indexManager->destroyFile(indexFileName);
    assert(rc == success && "indexManager::destroyFile() should not fail.");

    return success;

    error_close_index:
    indexManager->closeFile(ixfileHandle);
    indexManager->destroyFile(indexFileName);

    return fail;
}

int main()
{
    //Global Initializations
    indexManager = IndexManager::instance();

    const string indexEmpNameFileName = "private_empname_idx";

    Attribute attrEmpName;
    attrEmpName.length = PAGE_SIZE / 4;  // each node could only have 3 children
    attrEmpName.name = "EmpName";
    attrEmpName.type = TypeVarChar;

    remove("private_empname_idx");

    RC rcmain = testCase_p5(indexEmpNameFileName, attrEmpName);
    if (rcmain == success) {
        cerr << "***** IX Test Private Case 5 finished. The result will be examined. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Private Case 5 failed. *****" << endl;
        return fail;
    }

}
