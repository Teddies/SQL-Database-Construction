/*#include "rm_test_util.h"

RC TEST_RM_PRIVATE_1(const string &tableName)
{
    // Functions tested
    // 1. Insert 100,000 tuples
    // 2. Read Attribute
    cerr << endl << "***** In RM Test Case Private 1 *****" << endl;

    RID rid;
    int tupleSize = 0;
    int numTuples = 100000;
    void *tuple;
    void *returnedData = malloc(300);

    vector<RID> rids;
    vector<char *> tuples;
    set<int> user_ids;
    RC rc = 0;
    
    // GetAttributes
    vector<Attribute> attrs;
    rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");

    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
	memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);

    for(int i = 0; i < numTuples; i++)
    {
        tuple = malloc(300);

        // Insert Tuple
        float sender_location = (float)i;
	    float send_time = (float)i + 2000;
        int tweetid = i;
        int userid = i + i % 100;
        stringstream ss;
        ss << setw(5) << setfill('0') << i;
        string msg = "Msg" + ss.str();
        string referred_topics = "Rto" + ss.str();
        
        prepareTweetTuple(attrs.size(), nullsIndicator, tweetid, userid, sender_location, send_time, referred_topics.size(), referred_topics, msg.size(), msg, tuple, &tupleSize);

        user_ids.insert(userid);
        rc = rm->insertTuple(tableName, tuple, rid);
    	assert(rc == success && "RelationManager::insertTuple() should not fail.");

        tuples.push_back((char *)tuple);
        rids.push_back(rid);

        if (i % 10000 == 0){
           cerr << (i+1) << "/" << numTuples << " records have been inserted so far." << endl;
        }
    }
	cerr << "All records have been inserted." << endl;
	
	// Required for the other tests
    writeRIDsToDisk(rids);
    writeUserIdsToDisk(user_ids);

	bool testFail = false;
    string attributeName;

	for(int i = 0; i < numTuples; i=i+10)
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
        string msgToCheck = "Msg" + ss.str();
        string referred_topicsToCheck = "Rto" + ss.str();
		
		// tweetid
        if (attrID == 0) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(i) + 1), 4) != 0) {
                testFail = true;
            } else {
            	value = *(int *)((char *)returnedData + 1);
            	if (value != i) {
                    testFail = true;
            	}
            }
		// userid
        } else if (attrID == 1) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(i) + 5), 4) != 0) {
                testFail = true;
            } else {
            	value = *(int *)((char *)returnedData + 1);
            	if (value != (i + i % 100)) {
                    testFail = true;
            	}
            }
		// sender_location
        } else if (attrID == 2) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(i) + 9), 4) != 0) {
                testFail = true;
            } else {
            	fvalue = *(float *)((char *)returnedData + 1);
            	if (fvalue != (float) i) {
                    testFail = true;
            	}
            }
		// send_time
        } else if (attrID == 3) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(i) + 13), 4) != 0) {
                testFail = true;
            }
		// referred_topics
        } else if (attrID == 4) {
            if (memcmp(((char *)returnedData + 5), ((char *)tuples.at(i) + 21), 8) != 0) {
                testFail = true;
            } else {
            	std::string strToCheck (((char *)returnedData + 5), 8);
            	if (strToCheck.compare(referred_topicsToCheck) != 0) {
            		testFail = true;
            	}
            }
		// message_text
        } else if (attrID == 5) {
            if (memcmp(((char *)returnedData + 5), ((char *)tuples.at(i) + 33), 8) != 0) {
                testFail = true;
            } else {
            	std::string strToCheck (((char *)returnedData + 5), 8);
            	if (strToCheck.compare(msgToCheck) != 0) {
            		testFail = true;
            	}
            }
        }

        if (testFail) {
		    cerr << "***** RM Test Case Private 1 failed on " << i << "th tuple - attr: " << attrID << "*****" << endl << endl;
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

    }

    free(returnedData);
    for(int i = 0; i < numTuples; i++)
    {
        free(tuples[i]);
    }

    cerr << "***** RM Test Case Private 1 finished. The result will be examined. *****" << endl << endl;

    return 0;
}

int main()
{
    createTweetTable("tbl_private_1");
    RC rcmain = TEST_RM_PRIVATE_1("tbl_private_1");
    return rcmain;
}
*/
