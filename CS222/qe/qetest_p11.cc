#include <fstream>
#include <iostream>

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"


RC privateTestCase_11()
{
	// Optional for all
	// (+5 extra credit points will be given based on the results of the group-based hash aggregation related tests)
    // Aggregate -- MAX (with GroupBy)
	// SELECT group.B, MAX(group.C) FROM group where group.B > 3 GROUP BY group.B

    cout << endl << "***** In QE Private Test Case 11 *****" << endl;

	RC rc = 0;

    // Create IndexScan
    IndexScan *input = new IndexScan(*rm, "group", "B");

	int compVal = 3; // group.B should be 4,5
	
	// Set up condition
	Condition cond2;
	cond2.lhsAttr = "group.B";
	cond2.op = GT_OP;
	cond2.bRhsIsAttr = false;
	Value value2;
	value2.type = TypeInt;
	value2.data = malloc(bufSize);
	*(int *) value2.data = compVal;
	cond2.rhsValue = value2;

	// Create Filter
	Filter *filter = new Filter(input, cond2);

    // Create Aggregate
    Attribute aggAttr;
    aggAttr.name = "group.C";
    aggAttr.type = TypeReal;
    aggAttr.length = 4;

    Attribute gAttr;
    gAttr.name = "group.B";
    gAttr.type = TypeInt;
    gAttr.length = 4;
    Aggregate *agg = new Aggregate(filter, aggAttr, gAttr, MAX);

    int idVal = 0;
    int maxVal = 0;
    int expectedResultCnt = 2;
    int actualResultCnt = 0;

    void *data = malloc(bufSize);

    while(agg->getNextTuple(data) != QE_EOF)
    {
        int offset = 0;

        // Print group.B
        idVal = *(int *)((char *)data + offset + 1);
        cout << "group.B " << idVal;
        offset += sizeof(int);

        // Print MAX(group.A)
        maxVal = *(float *)((char *)data + offset + 1);
        cout << "  MAX(group.C) " <<  maxVal << endl;
        offset += sizeof(float);

        memset(data, 0, bufSize);
        if ((idVal == 4 && maxVal != 148) || (idVal == 5 && maxVal != 149)) {
            cout << "***** The group-based aggregation is not working properly. *****" << endl;
        	rc = fail;
        	goto clean_up;
        } 
        actualResultCnt++;
    }

    if (expectedResultCnt != actualResultCnt) {
        cout << "***** The number of returned tuple: " << actualResultCnt << " is not correct. *****" << endl;
    	rc = fail;
    }

clean_up:
	delete agg;
	delete filter;
	delete input;
    free(data);
    return rc;
}


int main() {
	// Indexes created: group.B
	
	if (createIndexforGroupB() != success) {
		cout << "***** [FAIL] QE Private Test Case 11 failed. *****" << endl;
		return fail;
	}

	if (privateTestCase_11() != success) {
		cout << "***** [FAIL] QE Private Test Case 11 failed. *****" << endl;
		return fail;
	} else {
		cout << "***** QE Private Test Case 11 finished. The result will be examined. *****" << endl;
		return success;
	}
}
