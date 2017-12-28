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

int RBFTest_p5(PagedFileManager *pfm)
{
    // Functions Tested:
    // 1. Open File
    // 2. Append Page
    // 3. Get Number Of Pages
    // 4. Get Counter Values
    // 5. Close File
    cout << endl << "***** In RBF Test Case Private 5 *****" << endl;

    RC rc;
    string fileName = "testp5";

    unsigned readPageCount = 0;
    unsigned writePageCount = 0;
    unsigned appendPageCount = 0;
    unsigned readPageCount1 = 0;
    unsigned writePageCount1 = 0;
    unsigned appendPageCount1 = 0;
    
    rc = pfm->createFile(fileName);
    assert(rc == success && "Creating the file should not fail.");

    // Open the file "testp5"
    FileHandle fileHandle;
    rc = pfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");

    // Collect before counters
    rc = fileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case 13 failed." << endl;
        rc = pfm->closeFile(fileHandle);
        return -1;
    }
    
    // Append the first page read the first page write the first page append the second page
    void *data = malloc(PAGE_SIZE);
    void *read_buffer = malloc(PAGE_SIZE);
    for(unsigned i = 0; i < PAGE_SIZE; i++)
    {
        *((char *)data+i) = i % 96 + 30;
    }
    rc = fileHandle.appendPage(data);
    assert(rc == success && "Appending a page should not fail.");
    rc = fileHandle.readPage(0, read_buffer);
    assert(rc == success && "Reading a page should not fail.");
    for(unsigned i = 0; i < PAGE_SIZE; i++)
    {
        *((char *)data+i) = i % 96 + 30;
    }
    rc = fileHandle.writePage(0, data);
    assert(rc == success && "Writing a page should not fail.");
    for(unsigned i = 0; i < PAGE_SIZE; i++)
    {
        *((char *)data+i) = i % 96 + 30;
    }
    rc = fileHandle.appendPage(data);
    assert(rc == success && "Appending a page should not fail.");
   
    // collect after counters
    rc = fileHandle.collectCounterValues(readPageCount1, writePageCount1, appendPageCount1);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case 13 failed." << endl;
        rc = pfm->closeFile(fileHandle);
        return -1;
    }
    assert(readPageCount1 - readPageCount == 1 && "Read counter should be correct.");
    assert(writePageCount1 - writePageCount == 1 && "Write counter should be correct.");
    assert(appendPageCount1 - appendPageCount == 2 && "Append counter should be correct.");
    assert(appendPageCount1 > appendPageCount && "The appendPageCount should have been increased.");
       
    // Get the number of pages
    unsigned count = fileHandle.getNumberOfPages();
    assert(count == (unsigned)2 && "The count should be one at this moment.");

    // Close the file "test3"
    rc = pfm->closeFile(fileHandle);

    assert(rc == success && "Closing the file should not fail.");

      // Open the file "test3"
    FileHandle fileHandle2;
    rc = pfm->openFile(fileName, fileHandle2);

    assert(rc == success && "Open the file should not fail.");

// collect after counters
    rc = fileHandle2.collectCounterValues(readPageCount1, writePageCount1, appendPageCount1);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case 13 failed." << endl;
        rc = pfm->closeFile(fileHandle);
        return -1;
    }
    assert(readPageCount1 - readPageCount == 1 && "Persistent read counter should be correct.");
    assert(writePageCount1 - writePageCount == 1 && "Persistent write counter should be correct.");
    assert(appendPageCount1 - appendPageCount == 2 && "Persistent append counter should be correct.");

    rc = pfm->closeFile(fileHandle2);

    assert(rc == success && "Closing the file should not fail.");

    rc = pfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    free(data);
    free(read_buffer);

    cout << "RBF Test Case Private 5 Finished! The result will be examined." << endl << endl;

    return 0;
}

int main()
{
    // To test the functionality of the paged file manager
    PagedFileManager *pfm = PagedFileManager::instance();
    
    RC rcmain = RBFTest_p5(pfm);
    return rcmain;
}
