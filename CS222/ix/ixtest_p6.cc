#include <iostream>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "ix.h"
#include "ix_test_util.h"

IndexManager *indexManager;

int testCase_p6(const string &indexFileName, const Attribute &attribute)
{
    // Checks whether duplicated entries in a page are handled properly.

    cerr << endl << "***** In IX Test Private Case 6 *****" << endl;

    RID rid;
    IXFileHandle ixfileHandle;
    IX_ScanIterator ix_ScanIterator;
    unsigned numOfTuples = 90;
    char key[100];
    *(int*)key = 5;
    int count = 0;

    char lowKey[100];
    char highKey[100];

    int inRidPageNumSum = 0;
    int outRidPageNumSum = 0;

    // create index file
    RC rc = indexManager->createFile(indexFileName);
    assert(rc == success && "indexManager::createFile() should not fail.");

    // open index file
    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");


    // insert entries
    for(unsigned i = 0; i < numOfTuples; i++)
    {
        sprintf(key + 4, "%05d", i % 3);

        rid.pageNum = i;
        rid.slotNum = i % 3;

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");

        if (i % 3 == 1) {
            inRidPageNumSum += rid.pageNum;
        }
    }

    // eyeball check: a key only appears once in each node (both inner nodes and leaf nodes)
    // Actually, this should print out only one page.
    indexManager->printBtree(ixfileHandle, attribute);

    *(int*)lowKey = 5;
    sprintf(lowKey+4, "%05d", 1);
    *(int*)highKey = 5;
    sprintf(highKey+4, "%05d", 1);

    // scan
    rc = indexManager->scan(ixfileHandle, attribute, lowKey, highKey, true, true, ix_ScanIterator);
    assert(rc == success && "indexManager::scan() should not fail.");

    //iterate
    count = 0;
    while(ix_ScanIterator.getNextEntry(rid, &key) != IX_EOF)
    {
        if (rid.slotNum != 1) {
            cerr << "Wrong entries output...failure" << endl;
            ix_ScanIterator.close();
            rc = indexManager->closeFile(ixfileHandle);
            rc = indexManager->destroyFile(indexFileName);
            return fail;
        }
        outRidPageNumSum += rid.pageNum;
        count++;
    }
    cerr << "The number of scanned entries: " << count << endl;
    if (count != 30 || outRidPageNumSum != inRidPageNumSum || inRidPageNumSum == 0 || outRidPageNumSum == 0) {
        cerr << "Wrong entries output...failure " << endl;
        rc = indexManager->closeFile(ixfileHandle);
        rc = indexManager->destroyFile(indexFileName);
        return fail;
    }

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
    // Global Initialization
    indexManager = IndexManager::instance();

    const string indexEmpNameFileName = "private_empname_shortidx";
    Attribute attrShortEmpName;
    attrShortEmpName.length = 10;
    attrShortEmpName.name = "ShortEmpName";
    attrShortEmpName.type = TypeVarChar;

    remove("private_empname_shortidx");

    RC result = testCase_p6(indexEmpNameFileName, attrShortEmpName);
    if (result == success) {
        cerr << "***** IX Test Private Case 6 finished. The result will be examined. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Private Case 6 failed. *****" << endl;
        return fail;
    }

}

