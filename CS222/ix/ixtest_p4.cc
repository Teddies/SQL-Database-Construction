#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "ix.h"
#include "ix_test_util.h"

IndexManager * indexManager;

int testCase_p4(const string &indexFileName1, const Attribute &attribute1, const string &indexFileName2, const Attribute &attribute2)
{
    // Checks whether varchar key is handled properly.
    cerr << endl << "***** In IX Test Private Case 4 *****" << endl;

    RID rid;
    IXFileHandle ixfileHandle1;
    IX_ScanIterator ix_ScanIterator1;
    IXFileHandle ixfileHandle2;
    IX_ScanIterator ix_ScanIterator2;

    unsigned  readPage1 = 0;
    unsigned  writePage1 = 0;
    unsigned  appendPage1 = 0;
    unsigned  readPage2 = 0;
    unsigned  writePage2 = 0;
    unsigned  appendPage2 = 0;

    char key[100];
    int numOfTuples = 50000;
    int i = 0;
    *(int*)key = 5;
    int count = 0;

    char lowKey[100];
    char highKey[100];

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


    // insert entries
    for(i = 1; i <= numOfTuples; i++)
    {
        sprintf(key + 4, "%05d", i);
        rid.pageNum = i;
        rid.slotNum = i % PAGE_SIZE;

        rc = indexManager->insertEntry(ixfileHandle1, attribute1, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");

        rc = indexManager->insertEntry(ixfileHandle2, attribute2, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");
    }
    
    // collect counters
    rc = ixfileHandle1.collectCounterValues(readPage1, writePage1, appendPage1);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");

    rc = ixfileHandle2.collectCounterValues(readPage2, writePage2, appendPage2);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");
    
    if (writePage1 < 1){
        cerr << "Did not use disk at all. Test failed." << endl;
        goto error_close_index;
    }

    // Actually, there should be no difference.
    if (writePage2 + appendPage2 - writePage1 - appendPage1 > 10) {
           cerr << "Failed to handle space nicely for VarChar keys..." << endl;
        goto error_close_index;
    }

    *(int*)lowKey = 5;
    sprintf(lowKey+4, "%05d", 30801);
    *(int*)highKey = 5;
    sprintf(highKey+4, "%05d", 30900);

    rc = indexManager->scan(ixfileHandle1, attribute1, lowKey, highKey, true, true, ix_ScanIterator1);
    assert(rc == success && "indexManager::scan() should not fail.");

    rc = indexManager->scan(ixfileHandle2, attribute2, lowKey, highKey, true, true, ix_ScanIterator2);
    assert(rc == success && "indexManager::scan() should not fail.");

    //iterate
    count = 0;
    while(ix_ScanIterator1.getNextEntry(rid, &key) != IX_EOF)
    {
        if (ix_ScanIterator2.getNextEntry(rid, &key) != success) {
            cerr << "Wrong entries output...failure" << endl;
            goto error_close_index;
        }
        //key[9] = '\0';
        //printf("output: %s\n", key+4);
        count++;
    }
    if (count != 100) {
        cerr << "Wrong output count! expected: 100, actual: " << count << " ...Failure" << endl;
        goto error_close_index;
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

error_close_index:
    indexManager->closeFile(ixfileHandle1);
    indexManager->closeFile(ixfileHandle2);
    indexManager->destroyFile(indexFileName1);
    indexManager->destroyFile(indexFileName2);

    return fail;
}

int main()
{
    indexManager = IndexManager::instance();
    const string indexEmpNameFileName1 = "private_empname_shortidx";
    const string indexEmpNameFileName2 = "private_empname_longidx";
    Attribute attrShortEmpName;
    attrShortEmpName.length = 10;
    attrShortEmpName.name = "ShortEmpName";
    attrShortEmpName.type = TypeVarChar;
    Attribute attrLongEmpName;
    attrLongEmpName.length = 100;
    attrLongEmpName.name = "LongEmpName";
    attrLongEmpName.type = TypeVarChar;

    remove("private_empname_shortidx");
    remove("private_empname_longidx");

    int rcmain = testCase_p4(indexEmpNameFileName1, attrShortEmpName, indexEmpNameFileName2, attrLongEmpName);
    if (rcmain == success) {
        cerr << "***** IX Test Private Case 4 finished. The result will be examined. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Private Case 4 failed. *****" << endl;
        return fail;
    }
}
