#include <iostream>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "ix.h"
#include "ix_test_util.h"

IndexManager *indexManager;

int testCase_9(const string &indexFileName, const Attribute &attribute) {
    // Functions tested
    // 1. Create Index File
    // 2. Open Index File
    // 3. Insert entry
    // 4. Scan entries using LT_OP operator and checking if the values returned are correct.
    //    Returned values are part of two separate insertions. **
    // 5. Scan close
    // 6. Close Index File
    // 7. Destroy Index File
    // NOTE: "**" signifies the new functions being tested in this test case.
    cerr << endl << "***** In IX Test Case 9 *****" << endl;

    RID rid;
    IXFileHandle ixfileHandle;
    IX_ScanIterator ix_ScanIterator;
    unsigned numOfTuples = 2000;
    unsigned numOfMoreTuples = 6000;
    float key;
    float compVal = 6500;
    int inRidSlotNumSum = 0;
    int outRidSlotNumSum = 0;

    // create index file
    RC rc = indexManager->createFile(indexFileName);
    assert(rc == success && "indexManager::createFile() should not fail.");

    // open index file
    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");

    // insert entries
    for (unsigned i = 1; i <= numOfTuples; i++) {
        key = (float) i + 87.6;
        rid.pageNum = i;
        rid.slotNum = i;

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");

        if (key < compVal) {
            inRidSlotNumSum += rid.slotNum;
        }
    }

    // insert more entries
    for (unsigned i = 6000; i <= numOfTuples + numOfMoreTuples; i++) {
        key = (float) i + 87.6;
        rid.pageNum = i;
        rid.slotNum = i - (unsigned) 500;

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");

        if (key < compVal) {
            inRidSlotNumSum += rid.slotNum;
        }
    }

    // scan
    rc = indexManager->scan(ixfileHandle, attribute, NULL, &compVal, true, false, ix_ScanIterator);
    assert(rc == success && "indexManager::scan() should not fail.");

    // iterate
    unsigned count = 0;
    while (ix_ScanIterator.getNextEntry(rid, &key) == success) {
        count++;
        if (rid.pageNum % 500 == 0) {
            cerr << count << " - Returned rid: " << rid.pageNum << " " << rid.slotNum << endl;
        }
        outRidSlotNumSum += rid.slotNum;
    }

    // Inconsistency between input and output?
    if (inRidSlotNumSum != outRidSlotNumSum) {
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

int main() {
    //Global Initializations
    indexManager = IndexManager::instance();

    const string indexFileName = "height_idx";
    Attribute attrHeight;
    attrHeight.length = 4;
    attrHeight.name = "height";
    attrHeight.type = TypeReal;

    remove("height_idx");

    RC result = testCase_9(indexFileName, attrHeight);
    if (result == success) {
        cerr << "***** IX Test Case 9 finished. The result will be examined. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Case 9 failed. *****" << endl;
        return fail;
    }

}

