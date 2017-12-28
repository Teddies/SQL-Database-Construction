/*#include "rm_test_util.h"


RC TEST_RM_PRIVATE_8(const string &tableName)
{
    // Functions tested
    // 1. Delete Tuples
    // 2. Scan Empty table
    // 3. Delete Table 
    cerr << endl << "***** In RM Test Case Private 8 *****" << endl;

    RC rc;
    RID rid;
    int numTuples = 100000;
    void *returnedData = malloc(300);
    vector<RID> rids;
    vector<string> attributes;

    attributes.push_back("tweetid");
    attributes.push_back("userid");
    attributes.push_back("sender_location");
    attributes.push_back("send_time");
    attributes.push_back("referred_topics");
    attributes.push_back("message_text");

    readRIDsFromDisk(rids, numTuples);

    for(int i = 0; i < numTuples; i++)
    {
        rc = rm->deleteTuple(tableName, rids[i]);
        if(rc != success) {
            free(returnedData);
            cout << "***** RelationManager::deleteTuple() failed. RM Test Case Private 8 failed *****" << endl << endl;
            return -1;
        }

        rc = rm->readTuple(tableName, rids[i], returnedData);
        if(rc == success) {
            free(returnedData);
            cout << "***** RelationManager::readTuple() should fail at this point. RM Test Case Private 8 failed *****" << endl << endl;
            return -1;
        }

        if (i % 10000 == 0){
            cout << (i+1) << " / " << numTuples << " have been processed." << endl;
        }
    }
	cout << "All records have been processed." << endl;
	
    // Set up the iterator
    RM_ScanIterator rmsi3;
    rc = rm->scan(tableName, "", NO_OP, NULL, attributes, rmsi3);
    if(rc != success) {
        free(returnedData);
        cout << "***** RelationManager::scan() failed. RM Test Case Private 8 failed. *****" << endl << endl;
        return -1;
    }

    if(rmsi3.getNextTuple(rid, returnedData) != RM_EOF)
    {
        cout << "***** RM_ScanIterator::getNextTuple() should fail at this point. RM Test Case Private 8 failed. *****" << endl << endl;
        rmsi3.close();
        free(returnedData);
        return -1;
    }
    rmsi3.close();
    free(returnedData);

    // Delete a Table
    rc = rm->deleteTable(tableName);
    if(rc != success) {
        cout << "***** RelationManager::deleteTable() failed. RM Test Case Private 8 failed. *****" << endl << endl;
        return -1;
    }

    cerr << "***** RM Test Case Private 8 finished. The result will be examined. *****" << endl << endl;
    return 0;
}

int main()
{
    RC rcmain = TEST_RM_PRIVATE_8("tbl_private_1");
    return rcmain;
}
*/
