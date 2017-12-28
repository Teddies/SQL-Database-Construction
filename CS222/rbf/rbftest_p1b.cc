#include <iostream>
#include <fstream>
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

int RBFtest_private_1b(RecordBasedFileManager *rbfm) {
    // Functions Tested:
    // 1. Create File - RBFM
    // 2. Open File
    // 3. insertRecord() - with two consecutive NULLs
    // 4. Close File
    // 5. Destroy File
    cout << "***** In RBF Test Case Private 1b *****" << endl;

    RC rc;
    string fileName = "test_private_1b";

    // Create a file named "test_private_1b"
    rc = rbfm->createFile(fileName);
    assert(rc == success && "Creating a file should not fail.");

    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating a file should not fail.");

    // Open the file "test_private_1b"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening a file should not fail.");
   
    RID rid;
    int recordSize = 0;
    void *record = malloc(2000);
    void *returnedData = malloc(2000);

    vector<Attribute> recordDescriptorForTweetMessage;
    createRecordDescriptorForTweetMessage(recordDescriptorForTweetMessage);
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptorForTweetMessage.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);

    // Setting the message_text(4th) and sender_location(5th) field values as null
    nullsIndicator[0] = 20; // 00011000
    
    // Insert a record into a file
    prepareRecordForTweetMessage(recordDescriptorForTweetMessage.size(), nullsIndicator, 101, 1, 8, "database", 31, "Finding shortcut_menu was easy.", 123.4, 1013.45, record, &recordSize);
    
    rc = rbfm->insertRecord(fileHandle, recordDescriptorForTweetMessage, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
    // Given the rid, read the record from file
    rc = rbfm->readRecord(fileHandle, recordDescriptorForTweetMessage, rid, returnedData);
    assert(rc == success && "Reading a record should not fail.");

    // Compare whether the two memory blocks are the same
    if(memcmp(record, returnedData, recordSize) != 0)
    {
        cout << "***** [FAIL] Test Case Private 1b Failed! *****" << endl << endl;
        free(record);
        free(returnedData);
        return -1;
    }
    
    // Close the file "test_private_1b"
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");

    cout << endl;
    int fsize = getFileSize(fileName);
    if (fsize == 0) {
        cout << "File Size should not be zero at this moment." << endl;
        rc = rbfm->destroyFile(fileName);
        free(record);
        free(returnedData);
        return -1;
    }
    
    // Destroy File
    rc = rbfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success  && "Destroying the file should not fail.");

    free(record);
    free(returnedData);
    
    cout << "***** RBF Test Case Private 1b Finished. The result will be examined! *****" << endl << endl;
    
    return 0;
}

int main() {
    RecordBasedFileManager *rbfm = RecordBasedFileManager::instance(); // To test the functionality of the record-based file manager

    remove("test_private_1b");

    RC rcmain = RBFtest_private_1b(rbfm);
    return rcmain;
}
