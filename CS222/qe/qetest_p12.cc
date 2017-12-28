#include <fstream>
#include <iostream>

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"


RC privateTestCase_12()
{
	// Optional for all teams - grad team, grad solo, undergrad team, and undergrad solo
	// (+5 extra credit points will be given based on the results of the group-based hash aggregation related tests)
    // Aggregate -- COUNT (with GroupBy)
	// SELECT group.B, COUNT(group.A) FROM group where group.B < 5 GROUP BY group.B

    cerr << endl << "***** In QE Private Test Case 12 *****" << endl;

	RC rc = 0;

    // Create IndexScan
    IndexScan *input = new IndexScan(*rm, "group", "B");

	int compVal = 5; // group.B should be 1,2,3,4
	
	// Set up condition
	Condition cond2;
	cond2.lhsAttr = "group.B";
	cond2.op = LT_OP;
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
    aggAttr.name = "group.A";
    aggAttr.type = TypeInt;
    aggAttr.length = 4;

    Attribute gAttr;
    gAttr.name = "group.B";
    gAttr.type = TypeInt;
    gAttr.length = 4;
    Aggregate *agg = new Aggregate(filter, aggAttr, gAttr, COUNT);

    int idVal = 0;
    int countVal = 0;
    int expectedResultCnt = 4;
    int actualResultCnt = 0;

    void *data = malloc(bufSize);

    while(agg->getNextTuple(data) != QE_EOF)
    {
        int offset = 0;

        // Print group.B
        idVal = *(int *)((char *)data + offset + 1);
        cerr << "group.B " << idVal;
        offset += sizeof(int);

        // Print COUNT(group.A)
        countVal = *(float *)((char *)data + offset + 1);
        cerr << "  COUNT(group.A) " <<  countVal << endl;
        offset += sizeof(float);

        memset(data, 0, bufSize);
//         if (countVal != 4) {
//             cerr << "***** The group-based aggregation is not working properly. *****" << endl;
//         	rc = fail;
//         	goto clean_up;
//         } 
        actualResultCnt++;
    }

    if (expectedResultCnt != actualResultCnt) {
        cerr << "***** The number of returned tuple: " << actualResultCnt << " is not correct. *****" << endl;
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
	
	if (privateTestCase_12() != success) {
		cerr << "***** [FAIL] QE Private Test Case 12 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Private Test Case 12 finished. The result will be examined. *****" << endl;
		return success;
	}
}
