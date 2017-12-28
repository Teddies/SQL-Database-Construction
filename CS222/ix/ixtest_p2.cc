#include <iostream>
#include <cstdio>
#include <algorithm>

#include "ix.h"
#include "ix_test_util.h"

IndexManager *indexManager;

int testCase_p2(const string &indexFileName1, const string &indexFileName2, 
        const Attribute &attribute){

    // insert 30,000 entries to two indexes
    // scan and delete
    // insert 20,000 entries to two indexes
    // scan

    cerr << endl << "***** In IX Test Private Case 2 *****" << endl;

    RID rid;
    RID rid2;
    IXFileHandle ixfileHandle1;
    IXFileHandle ixfileHandle2;
    IX_ScanIterator ix_ScanIterator1;
    IX_ScanIterator ix_ScanIterator2;
    int compVal;
    int numOfTuples;
    int A[20000];
    int B[30000];
    int count = 0;
    int key;
    int key2;

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


    // Prepare key entries
    numOfTuples = 20000;
    for(int i = 0; i < numOfTuples; i++)
    {
        A[i] = i;
    }

    // Randomly shuffle the entries
    random_shuffle(A, A+numOfTuples);

    // Insert entries
    for(int i = 0; i < numOfTuples; i++)
    {
        key = A[i];
        rid.pageNum = i+1;
        rid.slotNum = i+1;

        rc = indexManager->insertEntry(ixfileHandle1, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");

        rc = indexManager->insertEntry(ixfileHandle2, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");
    }

    compVal = 5000;

    // Conduct a scan
    rc = indexManager->scan(ixfileHandle1, attribute, NULL, &compVal, true, true, ix_ScanIterator1);
    assert(rc == success && "indexManager::scan() should not fail.");

    rc = indexManager->scan(ixfileHandle2, attribute, NULL, &compVal, true, true, ix_ScanIterator2);
    assert(rc == success && "indexManager::scan() should not fail.");

    // scan & delete
    count = 0;
    while(ix_ScanIterator1.getNextEntry(rid, &key) == success)
    {
        if (ix_ScanIterator2.getNextEntry(rid2, &key2) != success 
                || rid.pageNum != rid2.pageNum) {
            cerr << "Wrong entries output...failure" << endl;
            goto error_close_scan;
        }

        // delete entry
        rc = indexManager->deleteEntry(ixfileHandle1, attribute, &key, rid);
        assert(rc == success && "indexManager::deleteEntry() should not fail.");


        rc = indexManager->deleteEntry(ixfileHandle2, attribute, &key, rid);
        assert(rc == success && "indexManager::deleteEntry() should not fail.");

        count++;
    }
    if (count != 5001)
    {
        cerr << count << " - Wrong entries output...failure" << endl;
        goto error_close_scan;
    }

    // close scan
    rc = ix_ScanIterator1.close();
    assert(rc == success && "IX_ScanIterator::close() should not fail.");

    rc = ix_ScanIterator2.close();
    assert(rc == success && "IX_ScanIterator::close() should not fail.");

    
    // insert more entries Again
    numOfTuples = 30000;
    for(int i = 0; i < numOfTuples; i++)
    {
        B[i] = 20000+i;
    }
    random_shuffle(B, B+numOfTuples);

    for(int i = 0; i < numOfTuples; i++)
    {
        key = B[i];
        rid.pageNum = i+20001;
        rid.slotNum = i+20001;

        rc = indexManager->insertEntry(ixfileHandle1, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");

        rc = indexManager->insertEntry(ixfileHandle2, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");
    }

    // scan
    compVal = 35000;

    rc = indexManager->scan(ixfileHandle1, attribute, NULL, &compVal, true, true, ix_ScanIterator1);
    assert(rc == success && "indexManager::scan() should not fail.");

    rc = indexManager->scan(ixfileHandle2, attribute, NULL, &compVal, true, true, ix_ScanIterator2);
    assert(rc == success && "indexManager::scan() should not fail.");

    count = 0;
    while(ix_ScanIterator1.getNextEntry(rid, &key) == success)
    {
        if (ix_ScanIterator2.getNextEntry(rid2, &key) != success) {
            cerr << "Wrong entries output...failure" << endl;
            goto error_close_scan;
        }
        if(rid.pageNum > 20000 && B[rid.pageNum-20001] > 35000)
        {
            cerr << "Wrong entries output...failure" << endl;
            goto error_close_scan;
        }
        count ++;
    }
    if (count != 30000)
    {
        cerr << count << " - Wrong entries output...failure" << endl;
        goto error_close_scan;
    }

    //close scan
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
    const string indexAgeFileName1 = "private_age_idx1";
    const string indexAgeFileName2 = "private_age_idx2";
    Attribute attrAge;
    attrAge.length = 4;
    attrAge.name = "Age";
    attrAge.type = TypeInt;

    remove("private_age_idx1");
    remove("private_age_idx2");

    int rcmain = testCase_p2(indexAgeFileName1, indexAgeFileName2, attrAge);

    if (rcmain == success) {
        cerr << "***** IX Test Private Case 2 finished. The result will be examined. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Private Case 2 failed. *****" << endl;
        return fail;
    }


}
 
