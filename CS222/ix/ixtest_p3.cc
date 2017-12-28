#include <cstdlib>
#include <cstdio>
#include <iostream>
#include "ix.h"
#include "ix_test_util.h"

IndexManager *indexManager;

int testCase_p3(const string &indexFileName, const Attribute &attribute)
{
    cerr << endl << "***** In IX Test Private Case 3 *****" << endl;

    // Varchar index handling check
    RID rid;
    IXFileHandle ixfileHandle;
    IX_ScanIterator ix_ScanIterator;
    char key[100];
    int numOfTuples = 100000;
    int i = 0;
    *(int*)key = 6;
    int count = 0;
    char lowKey[100];
    char highKey[100];

    // create index files
    RC rc = indexManager->createFile(indexFileName);
    assert(rc == success && "indexManager::createFile() should not fail.");

    // open the index files
    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");

    // insert entry
    for(i = 1; i <= numOfTuples; i++)
    {
        sprintf(key + 4, "%06d", i);
        rid.pageNum = i;
        rid.slotNum = i % PAGE_SIZE;

        rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
        assert(rc == success && "indexManager::insertEntry() should not fail.");
   }

    *(int*)lowKey = 6;
    sprintf(lowKey+4, "%06d", 90000);
    *(int*)highKey = 6;
    sprintf(highKey+4, "%06d", 100000);

    // Conduct a scan
    rc = indexManager->scan(ixfileHandle, attribute, lowKey, highKey, true, true, ix_ScanIterator);
    assert(rc == success && "indexManager::scan() should not fail.");

    //iterate
    count = 0;
    while(ix_ScanIterator.getNextEntry(rid, &key) != IX_EOF)
    {
        key[10] = '\0';
        if (count % 2000 == 0){
            fprintf(stderr,"output: %s\n", key+4);
        }
        count++;
    }

    if (count != 10001) {
        cerr << "Wrong output count! expected: 10001" << ", actual: " << count << " Failure" << endl;
        goto error_close_scan;
    }

    // close scan
    rc = ix_ScanIterator.close();
    assert(rc == success && "IX_ScanIterator::close() should not fail.");

    // Close index file
    rc = indexManager->closeFile(ixfileHandle);
    assert(rc == success && "indexManager::closeFile() should not fail.");

    // Destroy Index
    rc = indexManager->destroyFile(indexFileName);
    assert(rc == success && "indexManager::destroyFile() should not fail.");

    return success;

error_close_scan: //close scan
    ix_ScanIterator.close();
    indexManager->closeFile(ixfileHandle);
    indexManager->destroyFile(indexFileName);
    return fail;
}

int main(){
    indexManager = IndexManager::instance();
    const string indexEmpNameFileName1 = "private_empname_shortidx";
    Attribute attrShortEmpName;
    attrShortEmpName.length = 20;
    attrShortEmpName.name = "ShortEmpName";
    attrShortEmpName.type = TypeVarChar;
    
    remove("private_empname_shortidx");

    int rcmain = testCase_p3(indexEmpNameFileName1, attrShortEmpName);

    if (rcmain == success) {
        cerr << "***** IX Test Private Case 3 finished. The result will be examined. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Private Case 3 failed. *****" << endl;
        return fail;
    }


}


