#include <iostream>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "ix.h"
#include "ix_test_util.h"

IndexManager *indexManager;

int testCase_7(const string &indexFileName, const Attribute &attribute)
{
    // Functions tested
    // 1. Open Index File that created by test case 6
    // 2. Scan entries NO_OP -- open
    // 3. Scan close
    // 4. Close Index File
    // 5. Destroy Index File
    // NOTE: "**" signifies the new functions being tested in this test case.
    cerr << endl << "***** In IX Test Case 7 *****" << endl;

    RID rid;
    IXFileHandle ixfileHandle;
    IX_ScanIterator ix_ScanIterator;
    unsigned key;
    int inRidSlotNumSum = 0;
    int outRidSlotNumSum = 0;
    unsigned numOfTuples = 1000;

    // open index file
    RC rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");

    // compute inRidPageNumSum without inserting entries
    for(unsigned i = 0; i <= numOfTuples; i++)
    {
        key = i;
        rid.pageNum = key;
        rid.slotNum = key * 3;

        inRidSlotNumSum += rid.slotNum;
    }

    // scan
    rc = indexManager->scan(ixfileHandle, attribute, NULL, NULL, true, true, ix_ScanIterator);
    assert(rc == success && "indexManager::scan() should not fail.");

    // Fetch all entries
    int count = 0;
    while(ix_ScanIterator.getNextEntry(rid, &key) == success)
    {
        count++;

        if (rid.pageNum % 200 == 0) {
            cerr << count << " - Returned rid: " << rid.pageNum << " " << rid.slotNum << endl;
        }
        outRidSlotNumSum += rid.slotNum;
    }

    // scan fail?
    if (inRidSlotNumSum != outRidSlotNumSum)
    {
        cerr << "Wrong entries output... The test failed." << endl;
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

    RC result = testCase_7(indexFileName, attrAge);
    if (result == success) {
        cerr << "***** IX Test Case 7 finished. The result will be examined. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Case 7 failed. *****" << endl;
        return fail;
    }

}

