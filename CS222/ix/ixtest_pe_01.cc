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
    cerr << endl << "***** In IX Private Test Extra Case 1 *****" << endl;

    RID rid;
    unsigned numOfTuples = 10000;
    unsigned numExtra = 5000;
    unsigned key;
    IXFileHandle ixfileHandle;
    IX_ScanIterator ix_ScanIterator;
    int compVal1 = 9, compVal2= 15;
    int count = 0;

    //create index file(s)
    RC rc = indexManager->createFile(indexFileName);
    assert(rc == success && "indexManager::createFile() should not fail.");

    //open index file
    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");

    // insert entry
    for(unsigned i = 1; i <= numOfTuples; i++)
    {
        key = i % 10;
        rid.pageNum = i;
        rid.slotNum = i;

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");
    }

    for(unsigned i = numOfTuples; i < numOfTuples + numExtra; i++)
    {
        key = i % 10 + 10;
        rid.pageNum = i;
        rid.slotNum = i+10;

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

        if (rid.pageNum != rid.slotNum || key != compVal1) {
            cerr << "Wrong entries output... The test failed" << endl;
        }

        if(count % 100 == 0) {
            cerr << count << " - Returned rid: " << rid.pageNum << " " << rid.slotNum << endl;
        }
    }

    cerr << "Number of scanned entries: " << count << endl << endl;
    if (count != 1000)
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

    // scan
    rc = indexManager->scan(ixfileHandle, attribute, &compVal2, &compVal2, true, true, ix_ScanIterator);
    assert(rc == success && "indexManager::scan() should not fail.");

    count = 0;
    while(ix_ScanIterator.getNextEntry(rid, &key) == success)
    {
        count++;

        if (rid.pageNum != (rid.slotNum - 10) || key != compVal2) {
            cerr << "Wrong entries output... The test failed" << endl;
        }

        if(count % 100 == 0) {
            cerr << count << " - Returned rid: " << rid.pageNum << " " << rid.slotNum << endl;
        }
    }

    cerr << "Number of scanned entries: " << count << endl;
    if (count != 500)
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

    const string indexFileName = "private_extra_age_idx";
    Attribute attrAge;
    attrAge.length = 4;
    attrAge.name = "age";
    attrAge.type = TypeInt;

    remove("private_extra_age_idx");

    RC result = testCase_extra_1(indexFileName, attrAge);
    if (result == success) {
        cerr << "IX_Test Private Extra Case 1 finished. The result will be examined." << endl;
        return success;
    } else {
        cerr << "IX_Test Private Extra Case 1 failed." << endl;
        return fail;
    }

}

