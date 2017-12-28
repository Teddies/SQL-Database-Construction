#include <iostream>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "ix.h"
#include "ix_test_util.h"

IndexManager *indexManager;

int testCase_12(const string &indexFileName, const Attribute &attribute)
{
    // Checks whether deleting an entry after getNextEntry() in a scan is handled properly or not.
    //    An example:
    //    IX_ScanIterator ix_ScanIterator;
    //    indexManager->scan(ixfileHandle, ..., ix_ScanIterator);
    //    while ((rc = ix_ScanIterator.getNextEntry(rid, &key)) != IX_EOF)
    //    {
    //       indexManager->deleteEntry(ixfileHandle, attribute, &key, rid);
    //    }

    // Functions tested
    // 1. Create Index File
    // 2. OpenIndex File
    // 3. Insert entry
    // 4. Scan entries - EXACT MATCH, and delete entries
    // 5. Scan close
    // 6. CloseIndex File
    // 7. DestroyIndex File
    // NOTE: "**" signifies the new functions being tested in this test case.
    cerr << endl << "***** In IX Test Case 12 *****" << endl;

    RID rid;
    IXFileHandle ixfileHandle;
    IX_ScanIterator ix_ScanIterator;
    float compVal = 100.0;
    unsigned numOfTuples = 200;
    float key;

    // create index file
    RC rc = indexManager->createFile(indexFileName);
    assert(rc == success && "indexManager::createFile() should not fail.");

    // open index file
    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");

    // Insert entries
    key = compVal;

    for(unsigned i = 1; i <= numOfTuples; i++)
    {
        rid.pageNum = i;
        rid.slotNum = i;

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");
    }

    // scan - EXACT MATCH
    rc = indexManager->scan(ixfileHandle, attribute, &compVal, &compVal, true, true, ix_ScanIterator);
    assert(rc == success && "indexManager::scan() should not fail.");

    // Delete entries in IndexScan Iterator
    unsigned count = 0;
    while(ix_ScanIterator.getNextEntry(rid, &key) == success)
    {
        count++;

        if (count % 100 == 0) {
            cerr << count << " - Returned rid: " << rid.pageNum << " " << rid.slotNum << endl;
        }
        RC rc = indexManager->deleteEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::deleteEntry() should not fail.");
    }
    cerr << endl;

    // close scan and open scan again
    rc = ix_ScanIterator.close();
    assert(rc == success && "IX_ScanIterator::close() should not fail.");

    rc = indexManager->scan(ixfileHandle, attribute, &compVal, &compVal, true, true, ix_ScanIterator);
    assert(rc == success && "indexManager::scan() should not fail.");

    // iterate - should fail
    while(ix_ScanIterator.getNextEntry(rid, &key) == success)
    {
        cerr << "Wrong entry returned: " << rid.pageNum << " " << rid.slotNum << " --- The test failed." << endl;
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

    const string indexFileName = "height_idx";
    Attribute attrHeight;
    attrHeight.length = 4;
    attrHeight.name = "height";
    attrHeight.type = TypeReal;

    remove("height_idx");

    RC result = testCase_12(indexFileName, attrHeight);
    if (result == success) {
        cerr << "***** IX Test Case 12 finished. The result will be examined. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Case 12 failed. *****" << endl;
        return fail;
    }

}

