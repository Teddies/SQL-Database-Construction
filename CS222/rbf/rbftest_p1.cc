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

int RBFTest_private_1(RecordBasedFileManager *rbfm) {
    // Functions Tested:
    // 1. Create File - RBFM
    // 2. Open File
    // 3. insertRecord() - with an empty string field (not NULL)
    // 4. Close File
    // 5. Destroy File
    cout << "***** In RBF Test Case Private 1 *****" << endl;

    RC rc;
    string fileName = "test_private_1";

    // Create a file named "test_private_1"
    rc = rbfm->createFile(fileName);
    assert(rc == success && "Creating a file should not fail.");

    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating a file should not fail.");

    // Open the file "test_private_1"
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

    // Insert a record into a file - referred_topics is an empty string - "", not null value.
    prepareRecordForTweetMessage(recordDescriptorForTweetMessage.size(), nullsIndicator, 101, 1, 0, "", 31, "Finding shortcut_menu was easy.", 123.4, 1013.45, record, &recordSize);
    
    // An empty string should be printed for the referred_topics field.
        
    rc = rbfm->insertRecord(fileHandle, recordDescriptorForTweetMessage, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
    // Given the rid, read the record from file
    rc = rbfm->readRecord(fileHandle, recordDescriptorForTweetMessage, rid, returnedData);
    assert(rc == success && "Reading a record should not fail.");

    // An empty string should be printed for the referred_topics field.

    // Compare whether the two memory blocks are the same
    if(memcmp(record, returnedData, recordSize) != 0)
    {
        cout << "***** [FAIL] Test Case Private 1 Failed! *****" << endl << endl;
        free(record);
        free(returnedData);
        return -1;
    }
    
    // Close the file "test_private_1"
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");

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
    
    cout << "***** RBF Test Case Private 1 Finished. The result will be examined! *****" << endl << endl;
    
    return 0;
}

int main() {
    RecordBasedFileManager *rbfm = RecordBasedFileManager::instance(); // To test the functionality of the record-based file manager

    remove("test_private_1");

    RC rcmain = RBFTest_private_1(rbfm);
    return rcmain;
}
