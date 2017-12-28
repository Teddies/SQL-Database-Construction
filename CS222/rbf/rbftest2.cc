#include <iostream>
#include <string>
#include <cassert>
#include <sys/stat.h>
#include <stdlib.h> 
#include <string.h>
#include <stdexcept>
#include <stdio.h> 

#include "pfm.h"
#include "rbfm.h"
#include "test_util.h"

using namespace std;

int RBFTest_2(PagedFileManager *pfm)
{
    // Functions Tested:
    // 1. Destroy File
    cout << endl << "***** In RBF Test Case 2 *****" << endl;

    RC rc;
    string fileName = "test1";

    rc = pfm->destroyFile(fileName);
    assert(rc == success  && "Destroying the file should not fail.");

    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success  && "Destroying the file should not fail.");
    
    // Destroy "test1" again, should fail
    rc = pfm->destroyFile(fileName);
    assert(rc != success && "Destroy the same file should fail.");

    cout << "RBF Test Case 2 Finished! The result will be examined." << endl << endl;
    return 0;
}

int main()
{
    // To test the functionality of the paged file manager
    PagedFileManager *pfm = PagedFileManager::instance();
    
    RC rcmain = RBFTest_2(pfm);
    return rcmain;
}
