#include <iostream>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include "ix.h"
#include "ix_test_util.h"

IndexManager *indexManager;

int testCase_3(const string &indexFileName, const Attribute &attribute)
{
    // Functions tested
    // 1. Open Index file
    // 2. Disk I/O check of Scan and getNextEntry - CollectCounterValues **
    // 3. Close Index file
    // NOTE: "**" signifies the new functions being tested in this test case.
    cerr << endl << "***** In IX Test Case 3 *****" << endl;

    RID rid;
    int key = 200;
    rid.pageNum = 500;
    rid.slotNum = 20;

    unsigned readPageCount = 0;
    unsigned writePageCount = 0;
    unsigned appendPageCount = 0;
    unsigned readPageCountAfter = 0;
    unsigned writePageCountAfter = 0;
    unsigned appendPageCountAfter = 0;
    unsigned readDiff = 0;
    unsigned writeDiff = 0;
    unsigned appendDiff = 0;

    IX_ScanIterator ix_ScanIterator;

    // open index file
    IXFileHandle ixfileHandle;
    RC rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");

    // collect counters
    rc = ixfileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");

    cerr << "Before scan - R W A: " << readPageCount << " " << writePageCount << " " << appendPageCount << endl;

    // Conduct a scan
    rc = indexManager->scan(ixfileHandle, attribute, NULL, NULL, true, true, ix_ScanIterator);
    assert(rc == success && "indexManager::scan() should not fail.");

    // There should be one record
    int count = 0;
    while(ix_ScanIterator.getNextEntry(rid, &key) == success)
    {
        cerr << "Returned rid from a scan: " << rid.pageNum << " " << rid.slotNum << endl;
        assert(rid.pageNum == 500 && "rid.pageNum is not correct.");
        assert(rid.slotNum == 20 && "rid.slotNum is not correct.");
        count++;
    }
    assert(count == 1 && "scan count is not correct.");

    // collect counters
    rc = ixfileHandle.collectCounterValues(readPageCountAfter, writePageCountAfter, appendPageCountAfter);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");

    cerr << "After scan - R W A: " << readPageCountAfter << " " << writePageCountAfter << " " << appendPageCountAfter << endl;

    readDiff = readPageCountAfter - readPageCount;
    writeDiff = writePageCountAfter - writePageCount;
    appendDiff = appendPageCountAfter - appendPageCount;

    cerr << "Page I/O count of scan - R W A: " << readDiff << " " << writeDiff << " " << appendDiff << endl;

    if (readDiff == 0 && writeDiff == 0 && appendDiff == 0) {
        cerr << "Scan should generate some page I/O. The implementation is not correct." << endl;
        rc = ix_ScanIterator.close();
        rc = indexManager->closeFile(ixfileHandle);
        return fail;
    } 

    // Close Scan
    rc = ix_ScanIterator.close();
    assert(rc == success && "IX_ScanIterator::close() should not fail.");

    // Close index file
    rc = indexManager->closeFile(ixfileHandle);
    assert(rc == success && "indexManager::closeFile() should not fail.");

    return success;

}

int main()
{
    // Global Initialization
    indexManager = IndexManager::instance();

    const string indexFileName = "age_idx";
    Attribute attrAge;
    attrAge.length = 4;
    attrAge.name = "age";
    attrAge.type = TypeInt;

    RC result = testCase_3(indexFileName, attrAge);
    if (result == success) {
        cerr << "***** IX Test Case 3 finished. The result will be examined. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Case 3 failed. *****" << endl;
        return fail;
    }

}

