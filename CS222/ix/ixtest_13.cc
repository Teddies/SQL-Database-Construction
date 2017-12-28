#include <iostream>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "ix.h"
#include "ix_test_util.h"

IndexManager *indexManager;

int testCase_13(const string &indexFileName, const Attribute &attribute)
{
    // Checks whether VARCHAR type is handled properly or not.
    //
    // Functions Tested:
    // 1. Create Index
    // 2. Open Index
    // 3. Insert Entry
    // 4. Get Insert IO count
    // 5. Scan
    // 6. Get Scan IO count
    // 7. Close Scan
    // 8. Close Index
    // 9. Destroy Index
    cerr << endl << "***** In IX Test Case 13 *****" << endl;

    RID rid;
    IXFileHandle ixfileHandle;
    IX_ScanIterator ix_ScanIterator;
    unsigned offset;
    unsigned numOfTuples = 1000;
    unsigned numOfMoreTuples = 5;
    char key[100];
    unsigned count;
    unsigned tested_ascii = 20;

    // create index file
    RC rc = indexManager->createFile(indexFileName);
    assert(rc == success && "indexManager::createFile() should not fail.");

    // open index file
    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");

    int numOfTuplesTobeScanned = 0;

    // insert entries
    for(unsigned i = 1; i <= numOfTuples; i++)
    {
        count = ((i - 1) % 26) + 1;
        *(int *)key = count;
        for(unsigned j = 0; j < count; j++)
        {
            key[4 + j] = 'a' + count - 1;
        }

        rid.pageNum = i;
        rid.slotNum = i;

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");

        if (count == tested_ascii) {
            numOfTuplesTobeScanned++;
        }
    }
    
    // insert more entries
    *(int *)key = tested_ascii;
    for (unsigned j = 0; j < tested_ascii; j++)
    {
        key[4 + j] = 'a' + tested_ascii - 1;
    }
    for (unsigned i = 1; i < numOfMoreTuples; i++){
        rid.pageNum = 26 * (50 + i) + tested_ascii;
        rid.slotNum = 26 * (50 + i) + tested_ascii;

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");
        
        numOfTuplesTobeScanned++;
    }

    // collect counter
    // we will eyeball those numbers to see if they are reasonable.
    unsigned readPageCount = 0;
    unsigned writePageCount = 0;
    unsigned appendPageCount = 0;
    rc = ixfileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");

    cerr << "IO count after insertion: R W A - "
        << readPageCount
        << " " << writePageCount
        << " " << appendPageCount << endl;

    //scan
    offset = tested_ascii;
    *(int *)key = offset;
    for(unsigned j = 0; j < offset; j++)
    {
        key[4 + j] = 'a' + offset - 1;
    }

    rc = indexManager->scan(ixfileHandle, attribute, &key, &key, true, true, ix_ScanIterator);
    assert(rc == success && "indexManager::scan() should not fail.");

    int count1 = 0;
    //iterate
    cerr << endl;
    while(ix_ScanIterator.getNextEntry(rid, &key) == success)
    {
        if (((rid.pageNum - 1) % 26 + 1) != offset) {
            cerr << "Wrong entry output... " << rid.pageNum << " " << rid.slotNum << " " << " - The test failed..." << endl;
            return fail;
        }
        count1++;
        if (count1 % 20 == 0) {
            cerr << count1 << " scanned - returned rid: " << rid.pageNum << " " << rid.slotNum << endl;
        }
    }
    cerr << endl;

    if (count1 != numOfTuplesTobeScanned) {
        cerr << "Wrong entry output... The test failed..." << endl;
        rc = ix_ScanIterator.close();
        rc = indexManager->closeFile(ixfileHandle);
        rc = indexManager->destroyFile(indexFileName);
        return fail;
    }

    // collect counter
    rc = ixfileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");

    cerr << "IO count after scan: R W A - "
        << readPageCount
        << " " << writePageCount
        << " " << appendPageCount << endl;

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
    // Global Initialization
    indexManager = IndexManager::instance();

    const string indexEmpNameFileName = "EmpName_idx";

    Attribute attrEmpName;
    attrEmpName.length = 100;
    attrEmpName.name = "EmpName";
    attrEmpName.type = TypeVarChar;

    remove("EmpName_idx");

    RC result = testCase_13(indexEmpNameFileName, attrEmpName);
    if (result == success) {
        cerr << "***** IX Test Case 13 finished. The result will be examined. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Case 13 failed. *****" << endl;
        return fail;
    }

}

