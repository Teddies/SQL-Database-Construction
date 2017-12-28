/*#include "rm_test_util.h"


int TEST_RM_PRIVATE_0(const string &tableName)
{
    // Functions tested
    // 1. Insert 10000 Tuples 
    // 2. Scan Table (with condition on varchar attr)
    // 3. Delete Table 
    cerr << endl << "***** In RM Test Case Private 0 *****" << endl;

    RID rid;    
    int tupleSize = 0;
    int numTuples = 10000;
    void *tuple;
    void *returnedData = malloc(100);
    RC rc = 0;

    // GetAttributes
    vector<Attribute> attrs;
    rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");

    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
	memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);

    RID rids[numTuples];
    vector<char *> tuples;
    set<int> ages; 
    for(int i = 0; i < numTuples; i++)
    {
        tuple = malloc(100);

        // Insert Tuple
        float height = (float)i;
        int age = i;

        string name;
	if (i % 2 == 0) {
           name = "aaa";
        } else {
           name = "bbb";
        }
        prepareTuple(attrs.size(), nullsIndicator, name.size(), name, age, height, 2000 + i, tuple, &tupleSize);
        ages.insert(age);
        rc = rm->insertTuple(tableName, tuple, rid);
	    assert(rc == success && "RelationManager::insertTuple() should not fail.");

        tuples.push_back((char *)tuple);
        rids[i] = rid;
    }
    cerr << "All records have been processed." << endl;

    // Set up the iterator
    RM_ScanIterator rmsi;
    string attr = "EmpName";
    vector<string> attributes;
    attributes.push_back("Age");

    void *value = malloc(7);
    string name = "aaa";
    int nameLength = 3;
    
    memcpy((char *)value, &nameLength, 4);
    memcpy((char *)value + 4, name.c_str(), nameLength);

    rc = rm->scan(tableName, attr, EQ_OP, value, attributes, rmsi);
    if(rc != success) {
        free(returnedData);
        for(int i = 0; i < numTuples; i++)
        {
            free(tuples[i]);
        }
	    cerr << "***** RM Test Case Private 0 failed. *****" << endl << endl;
        return -1;
    }
    int counter = 0;
    int ageReturned = 0;
    
    while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
    {
       ageReturned = *(int *)((char *)returnedData + 1);
       if (ageReturned % 2 != 0){
             cerr << "***** A returned value is not correct. RM Test Case Private 0 failed. *****" << endl << endl;
             rmsi.close();
             free(returnedData);
             for(int i = 0; i < numTuples; i++)
             {
                 free(tuples[i]);
             }
             return -1;
       }
       counter++;
    }
    rmsi.close();

    if (counter != numTuples / 2){
         cerr << "***** The number of returned tuples:" << counter << ". RM Test Case Private 0 failed. *****" << endl << endl;
         free(returnedData);
         for(int i = 0; i < numTuples; i++)
         {
             free(tuples[i]);
         }
         return -1;
    }

    // Delete a Table
    rc = rm->deleteTable(tableName);
    if(rc != success) {
        cerr << "***** RelationManager::deleteTale() should not fail. RM Test Case Private 0 failed. *****" << endl << endl;
		free(returnedData);
		for(int i = 0; i < numTuples; i++)
		{
			free(tuples[i]);
		}
        return -1;
    }
    
    free(returnedData);
    for(int i = 0; i < numTuples; i++)
    {
        free(tuples[i]);
    }
    cerr << "***** RM Test Case Private 0 finished. The result will be examined. *****" << endl << endl;
    return 0;
}

int main()
{
    createTable("tbl_private_0");
    RC rcmain = TEST_RM_PRIVATE_0("tbl_private_0");
    return rcmain;
}
*/
