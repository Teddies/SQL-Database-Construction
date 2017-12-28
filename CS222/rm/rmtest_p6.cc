/*#include "rm_test_util.h"

RC TEST_RM_PRIVATE_6(const string &tableName)
{
    // Functions tested
    // 1. Update tuples
    // 2. Read Attribute
    cerr << endl << "***** In RM Test Case Private 6 *****" << endl;

    RID rid;
    int tupleSize = 0;
    int numTuples = 100000;
    void *tuple;
    void *returnedData = malloc(300);

    vector<RID> rids;
    vector<char *> tuples;
    set<int> user_ids;
    RC rc = 0;
    
    readRIDsFromDisk(rids, numTuples);

    // GetAttributes
    vector<Attribute> attrs;
    rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");

    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
	memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);

	int updateCount = 0;

    for(int i = 0; i < numTuples; i=i+100)
    {
        tuple = malloc(300);

        // Update Tuple
        float sender_location = (float)i + 5000;
	    float send_time = (float)i + 7000;
        int tweetid = i;
        int userid = i + i % 500;
        stringstream ss;
        ss << setw(5) << setfill('0') << i;
        string msg = "UpdatedMsg" + ss.str();
        string referred_topics = "UpdatedRto" + ss.str();
        
        prepareTweetTuple(attrs.size(), nullsIndicator, tweetid, userid, sender_location, send_time, referred_topics.size(), referred_topics, msg.size(), msg, tuple, &tupleSize);

        // Update tuples
        rc = rm->updateTuple(tableName, tuple, rids[i]);
    	assert(rc == success && "RelationManager::updateTuple() should not fail.");

        if (i % 10000 == 0){
           cerr << (i+1) << "/" << numTuples << " records have been processed so far." << endl;
        }
        updateCount++;

        tuples.push_back((char *)tuple);
    }
	cerr << "All records have been processed - update count: " << updateCount << endl;
	
	bool testFail = false;
    string attributeName;

    int readCount = 0;
    // Integrity check
	for(int i = 0; i < numTuples; i=i+100)
    {
        int attrID = rand() % 6;
        if (attrID == 0) {
	    attributeName = "tweetid";
        } else if (attrID == 1) {
	    attributeName = "userid";
        } else if (attrID == 2) {
            attributeName = "sender_location";
        } else if (attrID == 3) {
            attributeName = "send_time";
        } else if (attrID == 4){
            attributeName = "referred_topics";
        } else if (attrID == 5){
            attributeName = "message_text";
        }
        rc = rm->readAttribute(tableName, rids[i], attributeName, returnedData);
    	assert(rc == success && "RelationManager::readAttribute() should not fail.");

		int value = 0;
		float fvalue = 0;
        stringstream ss;
        ss << setw(5) << setfill('0') << i;
        string msgToCheck = "UpdatedMsg" + ss.str();
        string referred_topicsToCheck = "UpdatedRto" + ss.str();

		// tweetid
        if (attrID == 0) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(readCount) + 1), 4) != 0) {
                testFail = true;
            } else {
            	value = *(int *)((char *)returnedData + 1);
            	if (value != i) {
                    testFail = true;
            	}
            }
		// userid
        } else if (attrID == 1) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(readCount) + 5), 4) != 0) {
                testFail = true;
            } else {
            	value = *(int *)((char *)returnedData + 1);
            	if (value != (i + i % 500)) {
                    testFail = true;
            	}
            }
		// sender_location
        } else if (attrID == 2) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(readCount) + 9), 4) != 0) {
                testFail = true;
            } else {
            	fvalue = *(float *)((char *)returnedData + 1);
            	if (fvalue != ((float) i + 5000)) {
                    testFail = true;
            	}
            }
		// send_time
        } else if (attrID == 3) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(readCount) + 13), 4) != 0) {
                testFail = true;
            }
		// referred_topics
        } else if (attrID == 4) {
            if (memcmp(((char *)returnedData + 5), ((char *)tuples.at(readCount) + 21), 15) != 0) {
                testFail = true;
            } else {
            	std::string strToCheck (((char *)returnedData + 5), 15);
            	if (strToCheck.compare(referred_topicsToCheck) != 0) {
            		testFail = true;
            	}
            }
		// message_text
        } else if (attrID == 5) {
            if (memcmp(((char *)returnedData + 5), ((char *)tuples.at(readCount) + 40), 15) != 0) {
                testFail = true;
            } else {
            	std::string strToCheck (((char *)returnedData + 5), 15);
            	if (strToCheck.compare(msgToCheck) != 0) {
            		testFail = true;
            	}
            }
        }

        if (testFail) {
		    cerr << "***** RM Test Case Private 6 failed on " << i << "th tuple - attr: " << attrID << "*****" << endl << endl;
		    free(returnedData);
			for(int j = 0; j < numTuples; j++)
			{
				free(tuples[j]);
			}
		    rc = rm->deleteTable(tableName);
		    remove("rids_file");
		    remove("user_ids_file");
		    
		    return -1;
        }
        readCount++;
    }

    free(returnedData);
    for(int i = 0; i < updateCount; i++)
    {
        free(tuples[i]);
    }

    cerr << "***** RM Test Case Private 6 finished. The result will be examined. *****" << endl << endl;

    return 0;
}

int main()
{
    RC rcmain = TEST_RM_PRIVATE_6("tbl_private_1");
    return rcmain;
}
*/
