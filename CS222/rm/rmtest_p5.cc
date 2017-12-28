/*#include "rm_test_util.h"


int TEST_RM_PRIVATE_5(const string &tableName)
{
    // Functions tested
    // 1. Scan Table (VarChar with Nulls)
    cerr << endl << "***** In RM Test Case Private 5 *****" << endl;

    RID rid; 
    vector<string> attributes;   
    void *returnedData = malloc(300);

    void *value = malloc(16);
    string msg = "Msg00099";
    int msgLength = 8;
    
    memcpy((char *)value, &msgLength, 4);
    memcpy((char *)value + 4, msg.c_str(), msgLength);
    
    string attr = "message_text";   
    attributes.push_back("sender_location");
    attributes.push_back("send_time");

    RM_ScanIterator rmsi2;
    RC rc = rm->scan(tableName, attr, LT_OP, value, attributes, rmsi2);
    if(rc != success) {
        free(returnedData);
	    cerr << "***** RM Test Case Private 5 failed. *****" << endl << endl;
        return -1;
    }
    
    float sender_location = 0.0;
    float send_time = 0.0;
    bool nullBit = false;
    int counter = 0;
    
    while(rmsi2.getNextTuple(rid, returnedData) != RM_EOF)
    {

		// There are 2 tuples whose message_text value is NULL
    	// For these tuples, send_time is also NULL. These NULLs should not be returned.
		nullBit = *(unsigned char *)((char *)returnedData) & (1 << 6);
		
		if (nullBit) {
            cerr << "***** A wrong entry was returned. RM Test Case Private 4 failed *****" << endl << endl;
             rmsi2.close();
             free(returnedData);
             free(value);
             return -1;
		}
        counter++;
    
    	sender_location = *(float *)((char *)returnedData + 1);
    	send_time = *(float *)((char *)returnedData + 5);
    	    
        if (!(sender_location >= 0.0 || sender_location <= 98.0 || send_time >= 2000.0 || send_time <= 2098.0))
        {
            cerr << "***** A wrong entry was returned. RM Test Case Private 5 failed *****" << endl << endl;
             rmsi2.close();
             free(returnedData);
             free(value);
             return -1;
        }
    }

    rmsi2.close();
    free(returnedData);
    free(value);
    
    if (counter != 97){
        cerr << "***** The number of returned tuple: " << counter << " is not correct. RM Test Case Private 5 failed *****" << endl << endl;
     } else {
         cerr << "***** RM Test Case Private 5 finished. The result will be examined. *****" << endl << endl;
    }
    return 0;
}

int main()
{
	// Using a table with Null values
    RC rcmain = TEST_RM_PRIVATE_5("tbl_private_2");
    return rcmain;
}
*/
