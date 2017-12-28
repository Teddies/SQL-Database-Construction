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

int RBFTest_1(PagedFileManager *pfm)
{
    // Functions Tested:
    // 1. Create File
    cout << endl << "***** In RBF Test Case 1 *****" << endl;

    RC rc;
    string fileName = "test1";

    // Create a file named "test"
    rc = pfm->createFile(fileName);
    assert(rc == success && "Creating the file failed.");

    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating the file failed.");

    // Create "test" again, should fail
    rc = pfm->createFile(fileName);
    assert(rc != success && "Creating the same file should fail.");

    cout << "RBF Test Case 1 Finished! The result will be examined." << endl << endl;
    return 0;
}

int main()
{
    // To test the functionality of the paged file manager
    PagedFileManager *pfm = PagedFileManager::instance();
    
    // Remove files that might be created by previous test run
    remove("test1");
    
    RC rcmain = RBFTest_1(pfm);
    return rcmain;
}
