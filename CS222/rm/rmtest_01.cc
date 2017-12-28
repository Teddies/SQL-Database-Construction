#include "rm_test_util.h"

RC TEST_RM_1(const string &tableName, const int nameLength, const string &name, const int age, const float height, const int salary)
{
    // Functions tested
    // 1. Insert Tuple **
    // 2. Read Tuple **
    // NOTE: "**" signifies the new functions being tested in this test case. 
    cout << endl << "***** In RM Test Case 1 *****" << endl;
   
    RID rid; 
    int tupleSize = 0;
    void *tuple = malloc(200);
    void *returnedData = malloc(200);

    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");

    // Initialize a NULL field indicator
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
	memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);

    // Insert a tuple into a table
    prepareTuple(attrs.size(), nullsIndicator, nameLength, name, age, height, salary, tuple, &tupleSize);
    cout << "The tuple to be inserted:" << endl;
    rc = rm->printTuple(attrs, tuple);
    cout << endl;
    
    rc = rm->insertTuple(tableName, tuple, rid);
    assert(rc == success && "RelationManager::insertTuple() should not fail.");
    
    // Given the rid, read the tuple from table
    rc = rm->readTuple(tableName, rid, returnedData);
    assert(rc == success && "RelationManager::readTuple() should not fail.");

    cout << "The returned tuple:" << endl;
    rc = rm->printTuple(attrs, returnedData);
    cout << endl;

    // Compare whether the two memory blocks are the same
    if(memcmp(tuple, returnedData, tupleSize) == 0)
    {
        cout << "**** RM Test Case 1 finished. The result will be examined. *****" << endl << endl;
        free(tuple);
        free(returnedData);
        return success;
    }
    else
    {
        cout << "**** [FAIL] RM Test Case 1 failed *****" << endl << endl;
        free(tuple);
        free(returnedData);
        return -1;
    }

}

int main()
{
    // Insert/Read Tuple
    RC rcmain = TEST_RM_1("tbl_employee", 14, "Peter Anteater", 27, 6.2, 10000);

    return rcmain;
}
