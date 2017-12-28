#include <iostream>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "ix.h"
#include "ix_test_util.h"

IndexManager *indexManager;

int testCase_5(const string &indexFileName, const Attribute &attribute)
{
    // Functions tested
    // 1. Destroy Index File **
    // 2. Open Index File -- should fail
    // 3. Scan  -- should fail
    cerr << endl << "***** In IX Test Case 5 *****" << endl;

    IXFileHandle ixfileHandle;
    IX_ScanIterator ix_ScanIterator;

    // destroy index file
    RC rc = indexManager->destroyFile(indexFileName);
    assert(rc == success && "indexManager::destroyFile() should not fail.");

    // Try to open the destroyed index
    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc != success && "indexManager::openFile() on a non-existing file should fail.");

    // Try to conduct a scan on the destroyed index
    rc = indexManager->scan(ixfileHandle, attribute, NULL, NULL, true, true, ix_ScanIterator);
    assert(rc != success && "indexManager::scan() on a non-existing file should fail.");

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

    RC result = testCase_5(indexFileName, attrAge);;
    if (result == success) {
        cerr << "***** IX Test Case 5 finished. The result will be examined. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Case 5 failed. *****" << endl;
        return fail;
    }

}
