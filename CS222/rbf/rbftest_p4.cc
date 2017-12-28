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

int RBFTest_private_4(RecordBasedFileManager *rbfm) {
    // Functions Tested:
    // 1. Open the File created by test_private_3 - test_private_3a
    // 2. Read entire records - test_private_3a
    // 3. Check correctness
    // 4. Close the File - test_private_3a
    // 5. Destroy the File - test_private_3a
    // 6. Open the File created by test 14 - test_private_3b
    // 7. Read entire records - test_private_3b
    // 8. Check correctness
    // 9. Close the File - test_private_3b
    // 10. Destroy the File - test_private_3b

    cout << endl << "***** In RBF Test Case Private 4 *****" << endl;

    RC rc;
    string fileName = "test_private_3a";
    string fileName2 = "test_private_3b";

    // Open the file "test_private_3a"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening a file should not fail.");

    void *record = malloc(1000);
    void *returnedData = malloc(1000);
    int numRecords = 5000;

    vector<Attribute> recordDescriptorForTwitterUser, recordDescriptorForTweetMessage;
    createRecordDescriptorForTwitterUser(recordDescriptorForTwitterUser);

    vector<RID> rids, rids2;
    RID tempRID, tempRID2;

    // Read rids from the disk - do not use this code. This is not a page-based operation. For test purpose only.
    ifstream ridsFileRead("test_private_3a_rids", ios::in | ios::binary);

    unsigned pageNum;
    unsigned slotNum;

    if (ridsFileRead.is_open()) {
        ridsFileRead.seekg(0,ios::beg);
        for (int i = 0; i < numRecords; i++) {
            ridsFileRead.read(reinterpret_cast<char*>(&pageNum), sizeof(unsigned));
            ridsFileRead.read(reinterpret_cast<char*>(&slotNum), sizeof(unsigned));
            tempRID.pageNum = pageNum;
            tempRID.slotNum = slotNum;
            rids.push_back(tempRID);
        }
        ridsFileRead.close();
    }

    if (rids.size() != numRecords) {
        return -1;
    }

    // NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptorForTwitterUser.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);

    // Compare records from the disk read with the record created from the method
    for (int i = 0; i < numRecords; i++) {
        memset(record, 0, 1000);
        memset(returnedData, 0, 1000);
        rc = rbfm->readRecord(fileHandle, recordDescriptorForTwitterUser, rids[i],
                returnedData);
        if (rc != success) {
            return -1;
        }
        assert(rc == success);

        int size = 0;
        prepareLargeRecordForTwitterUser(recordDescriptorForTwitterUser.size(), nullsIndicator, i, record, &size);
        if (memcmp(returnedData, record, size) != 0) {
            cout << "***** [FAIL] Comparison failed - RBF Test Case Private 4 Failed! *****" << i << endl << endl;
            free(record);
            free(returnedData);
            return -1;
        }
    }

    // Close the file
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing a file should not fail.");

    int fsize = getFileSize(fileName);
    if (fsize == 0) {
        cout << "File Size should not be zero at this moment." << endl;
        cout << "***** [FAIL] RBF Test Case Private 4 Failed! *****" << endl << endl;
        return -1;
    }
    
    // Destroy File
    rc = rbfm->destroyFile(fileName);
    assert(rc == success && "Destroying a file should not fail.");


    // Open the file "test_private_3b"
    FileHandle fileHandle2;
    rc = rbfm->openFile(fileName2, fileHandle2);
    assert(rc == success && "Opening a file should not fail.");

    createRecordDescriptorForTweetMessage(recordDescriptorForTweetMessage);

    cout << endl;

    // NULL field indicator
    int nullFieldsIndicatorActualSize2 = getActualByteForNullsIndicator(recordDescriptorForTweetMessage.size());
    unsigned char *nullsIndicator2 = (unsigned char *) malloc(nullFieldsIndicatorActualSize2);
    memset(nullsIndicator2, 0, nullFieldsIndicatorActualSize2);

    // Read rids from the disk - do not use this code. This is not a page-based operation. For test purpose only.
    ifstream ridsFileRead2("test_private_3b_rids", ios::in | ios::binary);

    if (ridsFileRead2.is_open()) {
        ridsFileRead2.seekg(0,ios::beg);
        for (int i = 0; i < numRecords; i++) {
            ridsFileRead2.read(reinterpret_cast<char*>(&pageNum), sizeof(unsigned));
            ridsFileRead2.read(reinterpret_cast<char*>(&slotNum), sizeof(unsigned));
            tempRID2.pageNum = pageNum;
            tempRID2.slotNum = slotNum;
            rids2.push_back(tempRID2);
        }
        ridsFileRead2.close();
    }

    if (rids2.size() != numRecords) {
        return -1;
    }



    // Compare records from the disk read with the record created from the method
    for (int i = 0; i < numRecords; i++) {
        memset(record, 0, 1000);
        memset(returnedData, 0, 1000);
        rc = rbfm->readRecord(fileHandle2, recordDescriptorForTweetMessage, rids2[i],
                returnedData);
        if (rc != success) {
            return -1;
        }
        assert(rc == success);

        int size = 0;
        prepareLargeRecordForTweetMessage(recordDescriptorForTweetMessage.size(), nullsIndicator2, i, record, &size);
        if (memcmp(returnedData, record, size) != 0) {
            cout << "***** [FAIL] Comparison failed - RBF Test Case Private 4 Failed! *****" << i << endl << endl;
            free(record);
            free(returnedData);
            return -1;
        }
    }

    // Close the file
    rc = rbfm->closeFile(fileHandle2);
    assert(rc == success && "Closing a file should not fail.");

    fsize = getFileSize(fileName2);
    if (fsize == 0) {
        cout << "File Size should not be zero at this moment." << endl;
        cout << "***** [FAIL] RBF Test Case Private 4 Failed! *****" << endl << endl;
        return -1;
    }

    // Destroy File
    rc = rbfm->destroyFile(fileName2);
    assert(rc == success && "Destroying a file should not fail.");

    free(record);
    free(returnedData);

    remove("test_private_3a_rids");
    remove("test_private_3b_rids");

    cout << "***** RBF Test Case Private 4 Finished. The result will be examined. *****" << endl << endl;

    return 0;
}

int main() {
    RecordBasedFileManager *rbfm = RecordBasedFileManager::instance(); // To test the functionality of the record-based file manager

    RC rcmain = RBFTest_private_4(rbfm);

    return rcmain;
}
