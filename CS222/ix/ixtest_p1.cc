#include <iostream>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "ix.h"
#include "ix_test_util.h"

IndexManager *indexManager;

int testCase_p1(const string &indexFileName1, const string &indexFileName2, const Attribute &attribute)
{
    // Check whether multiple indexes can be used at the same time.
    cerr << endl << "***** In IX Test Private Case 1 *****" << endl;

    RID rid;
    RID rid2;
    IXFileHandle ixfileHandle1;
    IXFileHandle ixfileHandle2;
    IX_ScanIterator ix_ScanIterator1;
    IX_ScanIterator ix_ScanIterator2;
    unsigned numOfTuples = 2000;
    float key;
    float key2;
    float compVal = 6500;
    int inRidPageNumSum = 0;
    int outRidPageNumSum = 0;

    // create index files
    RC rc = indexManager->createFile(indexFileName1);
    assert(rc == success && "indexManager::createFile() should not fail.");

    rc = indexManager->createFile(indexFileName2);
    assert(rc == success && "indexManager::createFile() should not fail.");

    // open the index files
    rc = indexManager->openFile(indexFileName1, ixfileHandle1);
    assert(rc == success && "indexManager::openFile() should not fail.");

    rc = indexManager->openFile(indexFileName2, ixfileHandle2);
    assert(rc == success && "indexManager::openFile() should not fail.");

    // insert entry
    for(unsigned i = 1; i <= numOfTuples; i++)
    {
        key = (float)i + 87.6;
        rid.pageNum = i;
        rid.slotNum = i;

        rc = indexManager->insertEntry(ixfileHandle1, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");

        rc = indexManager->insertEntry(ixfileHandle2, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");

        if (key < compVal){
            inRidPageNumSum += rid.pageNum;
        }
    }

    // insert more entries
    for (unsigned i = 6000; i <= numOfTuples + 6000; i++)
    {
        key = (float)i + 87.6;
        rid.pageNum = i;
        rid.slotNum = i-(unsigned)500;

        // insert entry
        rc = indexManager->insertEntry(ixfileHandle1, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");

        // insert entry
        rc = indexManager->insertEntry(ixfileHandle2, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");

        if (key < compVal)
        {
            inRidPageNumSum += rid.pageNum;
        }
    }

    // Conduct a scan
    rc = indexManager->scan(ixfileHandle1, attribute, NULL, &compVal, true, false, ix_ScanIterator1);
    assert(rc == success && "indexManager::scan() should not fail.");

    // Conduct a scan
    rc = indexManager->scan(ixfileHandle2, attribute, NULL, &compVal, true, false, ix_ScanIterator2);
    assert(rc == success && "indexManager::scan() should not fail.");

    int returnedCount = 0;
    while (ix_ScanIterator1.getNextEntry(rid, &key) == success)
    {
        returnedCount++;

        if (ix_ScanIterator2.getNextEntry(rid2, &key2) != success){
            cerr << "Wrong entries output...failure" << endl;
            goto error_close_scan;
        }
        if (rid.pageNum != rid2.pageNum) {
            cerr << "Wrong entries output...failure" << endl;
            goto error_close_scan;
        }
        if(rid.pageNum % 1000 == 0){
            cerr << returnedCount << " - returned entries: " << rid.pageNum << " " << rid.slotNum << endl;
        }
        outRidPageNumSum += rid.pageNum;
    }

    if (inRidPageNumSum != outRidPageNumSum)
    {
        cerr << "Wrong entries output...failure" << endl;
        goto error_close_scan;
    }

    // Close Scan
    rc = ix_ScanIterator1.close();
    assert(rc == success && "IX_ScanIterator::close() should not fail.");

    rc = ix_ScanIterator2.close();
    assert(rc == success && "IX_ScanIterator::close() should not fail.");


    // Close index file
    rc = indexManager->closeFile(ixfileHandle1);
    assert(rc == success && "indexManager::closeFile() should not fail.");

    rc = indexManager->closeFile(ixfileHandle2);
    assert(rc == success && "indexManager::closeFile() should not fail.");


    // Destroy Index
    rc = indexManager->destroyFile(indexFileName1);
    assert(rc == success && "indexManager::destroyFile() should not fail.");

    rc = indexManager->destroyFile(indexFileName2);
    assert(rc == success && "indexManager::destroyFile() should not fail.");

    return success;

error_close_scan: //close scan
    ix_ScanIterator1.close();
    ix_ScanIterator2.close();

    indexManager->closeFile(ixfileHandle1);
    indexManager->closeFile(ixfileHandle2);

    indexManager->destroyFile(indexFileName1);
    indexManager->destroyFile(indexFileName2);

    return fail;
}

int main(){
    indexManager = IndexManager::instance();
    const string indexHeightFileName1 = "private_height_idx1";
    const string indexHeightFileName2 = "private_height_idx2";
    Attribute attrHeight;
    attrHeight.length = 4;
    attrHeight.name = "Height";
    attrHeight.type = TypeReal;

    remove("private_height_idx1");
    remove("private_height_idx2");

    int rcmain = testCase_p1(indexHeightFileName1, indexHeightFileName2, attrHeight);

    if (rcmain == success) {
        cerr << "***** IX Test Private Case 1 finished. The result will be examined. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Private Case 1 failed. *****" << endl;
        return fail;
    }

}
