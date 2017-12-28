#include <iostream>
#include <cstdio>
#include "ix.h"
#include "ix_test_util.h"

IndexManager *indexManager;

int testCase_11(const string &indexFileName, const Attribute &attribute){
    // Create Index file
    // Open Index file
    // Insert large number of records
    // Scan large number of records to validate insert correctly
    // Delete some tuples
    // Insert large number of records again
    // Scan large number of records to validate insert correctly
    // Delete all
    // Close Index
    // Destroy Index

    cerr << endl << "***** In IX Test Case 11 *****" << endl;

    RID rid;
    IXFileHandle ixfileHandle;
    IX_ScanIterator ix_ScanIterator;
    unsigned key;
    unsigned inRecordNum = 0;
    unsigned outRecordNum = 0;
    unsigned numOfTuples = 1000 * 1000;

    // create index file
    RC rc = indexManager->createFile(indexFileName);
    assert(rc == success && "indexManager::createFile() should not fail.");

    // open index file
    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");

    // insert entries
    for(unsigned i = 0; i <= numOfTuples; i++)
    {
        key = i; 
        rid.pageNum = key + 1;
        rid.slotNum = key + 2;

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");
        inRecordNum += 1;
        if (inRecordNum % 200000 == 0) {
            cerr << inRecordNum << " inserted - rid: " << rid.pageNum << " " << rid.slotNum << endl;
        }
    }

    // scan
    rc = indexManager->scan(ixfileHandle, attribute, NULL, NULL, true, true, ix_ScanIterator);
    assert(rc == success && "indexManager::scan() should not fail.");

    // Iterate
    cerr << endl;
    while(ix_ScanIterator.getNextEntry(rid, &key) == success)
    {
        if (rid.pageNum != key + 1 || rid.slotNum != key + 2) {
            cerr << "Wrong entries output... The test failed." << endl;
            rc = ix_ScanIterator.close();
            rc = indexManager->closeFile(ixfileHandle);
            return fail;
        }
        outRecordNum += 1;
        if (outRecordNum % 200000 == 0) {
            cerr << outRecordNum << " scanned. " << endl;
        }
    }

    // Inconsistency?
    if (inRecordNum != outRecordNum || inRecordNum == 0 || outRecordNum == 0)
    {
        cerr << "Wrong entries output... The test failed." << endl;
        rc = ix_ScanIterator.close();
        rc = indexManager->closeFile(ixfileHandle);
        return fail;
    }

    // Delete some tuples
    cerr << endl;
    unsigned deletedRecordNum = 0;
    for(unsigned i = 5; i <= numOfTuples; i += 10)
    {
        key = i; 
        rid.pageNum = key + 1;
        rid.slotNum = key + 2;

        rc = indexManager->deleteEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::deleteEntry() should not fail.");

        deletedRecordNum += 1;
        if (deletedRecordNum % 20000 == 0) {
            cerr << deletedRecordNum << " deleted. " << endl;
        }
    }

    // Close Scan and reinitialize the scan
    rc = ix_ScanIterator.close();
    assert(rc == success && "IX_ScanIterator::close() should not fail.");

    rc = indexManager->scan(ixfileHandle, attribute, NULL, NULL, true, true, ix_ScanIterator);
    assert(rc == success && "IX_ScanIterator::scan() should not fail.");

    cerr << endl;
    // Iterate
    outRecordNum = 0;
    while(ix_ScanIterator.getNextEntry(rid, &key) == success)
    {
        if (rid.pageNum != key + 1 || rid.slotNum != key + 2) {
            cerr << "Wrong entries output... The test failed." << endl;
            rc = ix_ScanIterator.close();
            rc = indexManager->closeFile(ixfileHandle);
            return fail;
        }
        outRecordNum += 1;
        if (outRecordNum % 200000 == 0) {
            cerr << outRecordNum << " scanned. " << endl;
        }

    }
    cerr << outRecordNum << " scanned. " << endl;

    // Inconsistency?
    if ((inRecordNum - deletedRecordNum) != outRecordNum || inRecordNum == 0 || deletedRecordNum == 0 || outRecordNum == 0)
    {
        cerr << "Wrong entries output... The test failed." << endl;
        rc = ix_ScanIterator.close();
        rc = indexManager->closeFile(ixfileHandle);
        return fail;
    }

    // Insert the deleted entries again
    int reInsertedRecordNum = 0;
    cerr << endl;
    for(unsigned i = 5; i <= numOfTuples; i += 10)
    {
        key = i; 
        rid.pageNum = key + 1;
        rid.slotNum = key + 2;

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");

        reInsertedRecordNum += 1;
        if (reInsertedRecordNum % 20000 == 0) {
            cerr << reInsertedRecordNum << " inserted - rid: " << rid.pageNum << " " << rid.slotNum << endl;
        }
    }

    // Close Scan and reinitialize the scan
    rc = ix_ScanIterator.close();
    assert(rc == success && "IX_ScanIterator::close() should not fail.");

    rc = indexManager->scan(ixfileHandle, attribute, NULL, NULL, true, true, ix_ScanIterator);
    assert(rc == success && "IX_ScanIterator::scan() should not fail.");

    // Iterate
    cerr << endl;
    outRecordNum = 0;
    while(ix_ScanIterator.getNextEntry(rid, &key) == success)
    {
        if (rid.pageNum != key + 1 || rid.slotNum != key + 2) {
            cerr << "Wrong entries output... The test failed." << endl;
            rc = ix_ScanIterator.close();
            rc = indexManager->closeFile(ixfileHandle);
            return fail;
        }
        outRecordNum += 1;

        if (outRecordNum % 200000 == 0) {
            cerr << outRecordNum << " scanned. " << endl;
        }

    }

    // Inconsistency?
    if ((inRecordNum - deletedRecordNum + reInsertedRecordNum) != outRecordNum || inRecordNum == 0 
         || reInsertedRecordNum == 0 || outRecordNum == 0)
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
    // Global Initialization
    indexManager = IndexManager::instance();

    const string indexFileName = "age_idx";
    Attribute attrAge;
    attrAge.length = 4;
    attrAge.name = "age";
    attrAge.type = TypeInt;

    remove("age_idx");

    RC result = testCase_11(indexFileName, attrAge);
    if (result == success) {
        cerr << "***** IX Test Case 11 finished. The result will be examined. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Case 11 failed. *****" << endl;
        return fail;
    }
}

