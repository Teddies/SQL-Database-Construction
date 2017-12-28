/*#include "rm_test_util.h"


int TEST_RM_PRIVATE_7(const string &tableName)
{
    // Functions tested
    // 1. Scan Table
    cerr << endl << "***** In RM Test Case Private 7 *****" << endl;

    RID rid; 
    vector<string> attributes;   
    void *returnedData = malloc(300);

    void *value = malloc(20);
    string msg = "UpdatedMsg00100";
    int msgLength = 15;
    
    memcpy((char *)value, &msgLength, 4);
    memcpy((char *)value + 4, msg.c_str(), msgLength);
    
    string attr = "message_text";   
    attributes.push_back("sender_location");
    attributes.push_back("send_time");

    RM_ScanIterator rmsi2;
    RC rc = rm->scan(tableName, attr, EQ_OP, value, attributes, rmsi2);
    if(rc != success) {
        free(returnedData);
	    cerr << "***** RM Test Case Private 7 failed. *****" << endl << endl;
        return -1;
    }
    
    float sender_location = 0.0;
    float send_time = 0.0;
    bool nullBit = false;
    int counter = 0;
    
    while(rmsi2.getNextTuple(rid, returnedData) != RM_EOF)
    {
        counter++;
        if (counter > 1) {
            cerr << "***** A wrong entry was returned. RM Test Case Private 7 failed *****" << endl << endl;
            rmsi2.close();
            free(returnedData);
            free(value);
            return -1;
        }
    
    	sender_location = *(float *)((char *)returnedData + 1);
    	send_time = *(float *)((char *)returnedData + 5);
    	    
        if (!(sender_location == 5100.0 || send_time == 7100.0))
        {
             cerr << "***** A wrong entry was returned. RM Test Case Private 7 failed *****" << endl << endl;
             rmsi2.close();
             free(returnedData);
             free(value);
             return -1;
        }
    }

    rmsi2.close();
    free(returnedData);
    free(value);

    cerr << "***** RM Test Case Private 7 finished. The result will be examined. *****" << endl << endl;

    return 0;
}

int main()
{
    RC rcmain = TEST_RM_PRIVATE_7("tbl_private_1");
    return rcmain;
}
*/
