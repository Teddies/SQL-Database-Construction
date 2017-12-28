#include "rm_test_util.h"

RC RM_TEST_EXTRA_2(const string &tableName, const int nameLength, const string &name, const int age, const int height, const int salary, const int ssn)
{
    // Functions Tested
    // 1. Add Attribute **
    // 2. Insert Tuple
    cout << endl << "***** In RM Extra Credit Test Case 2 *****" << endl;

    RID rid;
    int tupleSize=0;
    void *tuple = malloc(200);
    void *returnedData = malloc(200);

    // Test Add Attribute
    Attribute attr;
    attr.name = "SSN";
    attr.type = TypeInt;
    attr.length = 9;
    RC rc = rm->addAttribute(tableName, attr);
    assert(rc == success && "RelationManager::addAttribute() should not fail.");
    
    // GetAttributes
    vector<Attribute> attrs;
    rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");

    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
	memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);

    // Test Insert Tuple
    prepareTupleAfterAdd(attrs.size(), nullsIndicator, nameLength, name, age, height, salary, ssn, tuple, &tupleSize);
    rc = rm->insertTuple(tableName, tuple, rid);
    assert(rc == success && "RelationManager::insertTuple() should not fail.");

    // Test Read Tuple
    rc = rm->readTuple(tableName, rid, returnedData);
    assert(rc == success && "RelationManager::readTuple() should not fail.");

    cout << "Inserted Data:" << endl;
    rc = rm->printTuple(attrs, tuple);

    cout << endl << "Returned Data:" << endl;
    rc = rm->printTuple(attrs, returnedData);

    if (memcmp(returnedData, tuple, tupleSize) != 0)
    {
        cout << "***** [FAIL] Extra Credit Test Case 2 Failed *****" << endl << endl;
        free(tuple);
        free(returnedData);
        return -1;
    }
    else
    {
        cout << "***** Extra Credit Test Case 2 Finished. The result will be examined. *****" << endl << endl;
        free(tuple);
        free(returnedData);
        return success;
    }

}

int main()
{
    string name2 = "Victors";

    // Drop table for the case where we execute this test multiple times.
    // We ignore the error code for this operation.
    RC rcmain = rm->deleteTable("tbl_employee200");

    rcmain = createTable("tbl_employee200");

    // Add Attributes
    rcmain = RM_TEST_EXTRA_2("tbl_employee200", 7, name2, 22, 180, 6000, 123479765);

    return rcmain;
}
