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

int RBFTest_private_0(RecordBasedFileManager *rbfm) {
    // Checks whether VarChar is implemented correctly or not.
    //
    // Functions tested
    // 1. Create Two Record-Based File
    // 2. Open Two Record-Based File
    // 3. Insert Multiple Records Into Two files
    // 4. Close Two Record-Based File
    // 5. Compare The File Sizes
    // 6. Destroy Two Record-Based File
    cout << endl << "***** In RBF Test Case Private 0 *****" << endl;

    RC rc;
    string fileName1 = "test_private_0a";
    string fileName2 = "test_private_0b";

    // Create a file named "test_private_0a"
    rc = rbfm->createFile(fileName1);
    assert(rc == success && "Creating a file should not fail.");

    rc = createFileShouldSucceed(fileName1);
    assert(rc == success && "Creating a file failed.");

    // Create a file named "test_private_0b"
    rc = rbfm->createFile(fileName2);
    assert(rc == success && "Creating a file should not fail.");

    rc = createFileShouldSucceed(fileName2);
    assert(rc == success && "Creating a file failed.");

    // Open the file "test_private_0a"
    FileHandle fileHandle1;
    rc = rbfm->openFile(fileName1, fileHandle1);
    assert(rc == success && "Opening a file should not fail.");

    // Open the file "test_private_0b"
    FileHandle fileHandle2;
    rc = rbfm->openFile(fileName2, fileHandle2);
    assert(rc == success && "Opening a file should not fail.");

    RID rid;
    void *record = malloc(3000);
    int numRecords = 5000;

    // Each varchar field length - 200
    vector<Attribute> recordDescriptor1;
    createRecordDescriptorForTwitterUser(recordDescriptor1);

    // NULL field indicator
    int nullFieldsIndicatorActualSize1 = getActualByteForNullsIndicator(recordDescriptor1.size());
    unsigned char *nullsIndicator1 = (unsigned char *) malloc(nullFieldsIndicatorActualSize1);
    memset(nullsIndicator1, 0, nullFieldsIndicatorActualSize1);

    // Each varchar field length - 800
    vector<Attribute> recordDescriptor2;
    createRecordDescriptorForTwitterUser2(recordDescriptor2);

    bool equalSizes = false;

    // Insert 5000 records into file
    for (int i = 0; i < numRecords; i++) {
        // Test insert Record
        int size = 0;
        memset(record, 0, 3000);
        prepareLargeRecordForTwitterUser(recordDescriptor1.size(), nullsIndicator1, i, record, &size);

        rc = rbfm->insertRecord(fileHandle1, recordDescriptor1, record, rid);
        assert(rc == success && "Inserting a record should not fail.");

        rc = rbfm->insertRecord(fileHandle2, recordDescriptor2, record, rid);
        assert(rc == success && "Inserting a record should not fail.");

        if (i%1000 == 0 && i != 0) {
            cout << i << " / " << numRecords << "records are inserted." << endl;
            compareFileSizes(fileName1, fileName2);
        }
    }
    // Close the file "test_private_0a"
    rc = rbfm->closeFile(fileHandle1);
    assert(rc == success && "Closing a file should not fail.");

    // Close the file "test_private_0b"
    rc = rbfm->closeFile(fileHandle2);
    assert(rc == success && "Closing a file should not fail.");

    free(record);

    cout << endl;
    equalSizes = compareFileSizes(fileName1, fileName2);

    rc = rbfm->destroyFile(fileName1);
    assert(rc == success && "Destroying a file should not fail.");

    rc = destroyFileShouldSucceed(fileName1);
    assert(rc == success  && "Destroying the file should not fail.");

    rc = rbfm->destroyFile(fileName2);
    assert(rc == success && "Destroying a file should not fail.");

    rc = destroyFileShouldSucceed(fileName1);
    assert(rc == success  && "Destroying the file should not fail.");

    if (!equalSizes) {
        cout << "Variable length Record is not properly implemented." << endl;
        cout << "**** [FAIL] RBF Test Private 0 failed. Two files are of different sizes. *****" << endl;
        return -1;
    }
    
    cout << "***** RBF Test Case Private 0 Finished. The result will be examined! *****" << endl << endl;

    return 0;
}

int main() {
    RecordBasedFileManager *rbfm = RecordBasedFileManager::instance(); // To test the functionality of the record-based file manager

    remove("test_private_0a");
    remove("test_private_0b");

    RC rcmain = RBFTest_private_0(rbfm);
    return rcmain;
}
