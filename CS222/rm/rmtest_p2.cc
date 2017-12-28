/*#include "rm_test_util.h"

RC TEST_RM_PRIVATE_2(const string &tableName)
{
    // Functions tested
    // 1. Insert 100000 Tuples - with null values
    // 2. Read Attribute
    cerr << endl << "***** In RM Test Case Private 2 *****" << endl;

    RID rid;
    int tupleSize = 0;
    int numTuples = 100000;
    void *tuple;
    void *returnedData = malloc(300);

    vector<RID> rids;
    vector<char *> tuples;
    RC rc = 0;
    
    // GetAttributes
    vector<Attribute> attrs;
    rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");

    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    unsigned char *nullsIndicatorWithNulls = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
	memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
	memset(nullsIndicatorWithNulls, 0, nullAttributesIndicatorActualSize);
	nullsIndicatorWithNulls[0] = 84; // 01010100 - the 2nd, 4th, and 6th column is null. - userid, send_time, and message_text
	
    for(int i = 0; i < numTuples; i++)
    {
        tuple = malloc(300);

        // Insert Tuple
        float sender_location = (float)i;
	    float send_time = (float)i + 2000;
        int tweetid = i;
        int userid = i;
        stringstream ss;
        ss << setw(5) << setfill('0') << i;
        string msg = "Msg" + ss.str();
        string referred_topics = "Rto" + ss.str();

        // There will be some tuples with nulls.
		if (i % 50 == 0) {
        	prepareTweetTuple(attrs.size(), nullsIndicatorWithNulls, tweetid, userid, sender_location, send_time, referred_topics.size(), referred_topics, msg.size(), msg, tuple, &tupleSize);
		} else {
        	prepareTweetTuple(attrs.size(), nullsIndicator, tweetid, userid, sender_location, send_time, referred_topics.size(), referred_topics, msg.size(), msg, tuple, &tupleSize);
		}        

        rc = rm->insertTuple(tableName, tuple, rid);
    	assert(rc == success && "RelationManager::insertTuple() should not fail.");

        tuples.push_back((char *)tuple);
        rids.push_back(rid);
        if (i % 10000 == 0){
           cerr << (i+1) << "/" << numTuples << " records have been inserted so far." << endl;
        }
    }
	cerr << "All records have been inserted." << endl;
	
	bool testFail = false;
	bool nullBit = false;
    for(int i = 0; i < numTuples; i++)
    {
        int attrID = rand() % 6;
        string attributeName;
        
        // Force attrID to be the ID that contains NULL when a i%50 is 0.
        if (i%50 == 0) {
        	if (attrID % 2 == 0) {
        		attrID = 1;
        	} else {
            	attrID = 5;
        	}
        }

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

		// NULL indicator should say that a NULL value is returned.
		if (i%50 == 0) {
				nullBit = *(unsigned char *)((char *)returnedData) & (1 << 7);
				if (!nullBit) {
					cerr << "A returned value from a readAttribute() is not correct: attrID - " << attrID << endl;
					testFail = true;					
				}		
		} else {
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
	            	if (value != i) {
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
		}
        
        if (testFail) {
		    cerr << "***** RM Test Case Private 2 failed on the tuple #" << i << " - attrID: " << attrID << "*****" << endl << endl;
		    free(returnedData);
			for(int j = 0; j < numTuples; j++)
			{
				free(tuples[j]);
			}
		    rc = rm->deleteTable(tableName);
		    
		    return -1;
        }

    }

    free(returnedData);
    for(int i = 0; i < numTuples; i++)
    {
        free(tuples[i]);
    }
    
    cerr << "***** RM Test Case Private 2 finished. The result will be examined. *****" << endl << endl;

    return 0;
}

int main()
{
    createTweetTable("tbl_private_2");
    RC rcmain = TEST_RM_PRIVATE_2("tbl_private_2");
    return rcmain;
}
*/
