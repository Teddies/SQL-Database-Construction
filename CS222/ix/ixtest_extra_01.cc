#include <iostream>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "ix.h"
#include "ix_test_util.h"

IndexManager *indexManager;

int testCase_extra_1(const string &indexFileName, const Attribute &attribute)
{
    // Checks whether duplicated entries spanning multiple page are handled properly or not.
    //
    // Functions tested
    // 1. Create Index
    // 2. OpenIndex
    // 3. Insert entry
    // 4. Scan entries (EXACT MATCH).
    // 5. Scan close
    // 6. CloseIndex
    // 7. DestroyIndex
    // NOTE: "**" signifies the new functions being tested in this test case.
    cerr << endl << "***** In IX Test Extra Case 1 *****" << endl;

    RID rid;
    unsigned numOfTuples = 2000;
    unsigned numExtra = 1000;
    unsigned key;
    IXFileHandle ixfileHandle;
    IX_ScanIterator ix_ScanIterator;
    int compVal1 = 1234, compVal2 = 4321;
    unsigned count = 0;

    //create index file(s)
    RC rc = indexManager->createFile(indexFileName);
    assert(rc == success && "indexManager::createFile() should not fail.");

    //open index file
    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");

    // insert entry
    key = compVal1;
    for(unsigned i = 1; i <= numOfTuples; i++)
    {
        rid.pageNum = i;
        rid.slotNum = i;

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");
    }

    key = compVal2;
    for(unsigned i = numOfTuples; i < numOfTuples+ numExtra; i++)
    {
        rid.pageNum = i;
        rid.slotNum = i - 5;

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");
    }

    // scan
    rc = indexManager->scan(ixfileHandle, attribute, &compVal1, &compVal1, true, true, ix_ScanIterator);
    assert(rc == success && "indexManager::scan() should not fail.");

    // iterate
    count = 0;
    while(ix_ScanIterator.getNextEntry(rid, &key) == success)
    {
        count++;

        if (rid.pageNum != rid.slotNum) {
            cerr << "Wrong entries output... The test failed" << endl;
        }

        if(count % 100 == 0) {
            cerr << count << " - Returned rid: " << rid.pageNum << " " << rid.slotNum << endl;
        }
    }

    cerr << "Number of scanned entries: " << count << endl;
    if (count != numOfTuples)
    {
        cerr << "Wrong entries output... The test failed" << endl;
        rc = ix_ScanIterator.close();
        rc = indexManager->closeFile(ixfileHandle);
        rc = indexManager->destroyFile(indexFileName);
        return fail;
    }

    // Close Scan
    rc = ix_ScanIterator.close();
    assert(rc == success && "IX_ScanIterator::close() should not fail.");

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

    const string indexFileName = "age_idx";
    Attribute attrAge;
    attrAge.length = 4;
    attrAge.name = "age";
    attrAge.type = TypeInt;

    RC result = testCase_extra_1(indexFileName, attrAge);
    if (result == success) {
        cerr << "IX_Test Case Extra 1 finished. The result will be examined." << endl;
        return success;
    } else {
        cerr << "IX_Test Case Extra 1 failed." << endl;
        return fail;
    }

}

