#include "rm_test_util.h"

RC TEST_RM_4(const string &tableName, const int nameLength, const string &name, const int age, const float height, const int salary)
{
    // Functions Tested
    // 1. Insert tuple
    // 2. Read Attributes **
    cout << endl << "***** In RM Test Case 4 *****" << endl;
    
    RID rid;    
    int tupleSize = 0;
    void *tuple = malloc(200);
    void *returnedData = malloc(200);

    // Test Insert the Tuple
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");

    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
	memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    prepareTuple(attrs.size(), nullsIndicator, nameLength, name, age, height, salary, tuple, &tupleSize);
    rc = rm->insertTuple(tableName, tuple, rid);
    assert(rc == success && "RelationManager::insertTuple() should not fail.");

    // Test Read Attribute
    rc = rm->readAttribute(tableName, rid, "Salary", returnedData);
    assert(rc == success && "RelationManager::readAttribute() should not fail.");
 
    int salaryBack = *(int *)((char *)returnedData+nullAttributesIndicatorActualSize);

    cout << "Salary: " << salary << " Returned Salary: " << salaryBack << endl;
    if (memcmp((char *)returnedData+nullAttributesIndicatorActualSize, (char *)tuple+19+nullAttributesIndicatorActualSize, 4) == 0)
    {
        cout << "***** RM Test case 4 Finished. The result will be examined. *****" << endl << endl;
        free(tuple);
        free(returnedData);
        return success;
    }
    else
    {
        cout << "***** [FAIL] RM Test Case 4 Failed. *****" << endl << endl;
        free(tuple);
        free(returnedData);
        return -1;
    }
    
}

int main()
{
    // Read Attributes
    RC rcmain = TEST_RM_4("tbl_employee", 7, "Hoffman", 31, 5.8, 9999);

    return rcmain;
}
