#include <fstream>
#include <iostream>

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"


RC testCase_15()
{
	// Optional for all: +5 extra credit points will be given based on the results of the group-based hash aggregation related tests.
    // Aggregate -- MIN (with GroupBy)
	// SELECT group.B, MIN(group.A) FROM group GROUP BY group.B

    cerr << "***** In QE Test Case 15 *****" << endl;

	RC rc = 0;
    // Create TableScan
    TableScan *input = new TableScan(*rm, "group");

    // Create Aggregate
    Attribute aggAttr;
    aggAttr.name = "group.A";
    aggAttr.type = TypeInt;
    aggAttr.length = 4;

    Attribute gAttr;
    gAttr.name = "group.B";
    gAttr.type = TypeInt;
    gAttr.length = 4;
    Aggregate *agg = new Aggregate(input, aggAttr, gAttr, MIN);

    int idVal = 0;
    int minVal = 0;
    int expectedResultCnt = 5;
    int actualResultCnt = 0;

    void *data = malloc(bufSize);

    while(agg->getNextTuple(data) != QE_EOF)
    {
        int offset = 0;

        // Print group.B
        idVal = *(int *)((char *)data + offset + 1);
        cerr << "group.B " << idVal;
        offset += sizeof(int);

        // Print MIN(group.A)
        minVal = *(float *)((char *)data + offset + 1);
        cerr << "  MIN(group.A) " <<  minVal << endl;
        offset += sizeof(float);

        memset(data, 0, bufSize);
        if (idVal != minVal) {
            cerr << "***** The group-based aggregation is not working properly. *****" << endl;
        	rc = fail;
        	goto clean_up;
        }
        actualResultCnt++;
    }

    if (expectedResultCnt != actualResultCnt) {
        cerr << "***** The number of returned tuple is not correct. *****" << endl;
    	rc = fail;
    }

clean_up:
	delete agg;
	delete input;
    free(data);
    return rc;
}


int main() {
	
	if (createGroupTable() != success) {
		cerr << "***** [FAIL] QE Test Case 15 failed. *****" << endl;
		return fail;
	}

	if (populateGroupTable() != success) {
		cerr << "***** [FAIL] QE Test Case 15 failed. *****" << endl;
		return fail;
	}
	
	if (testCase_15() != success) {
		cerr << "***** [FAIL] QE Test Case 15 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Test Case 15 finished. The result will be examined. *****" << endl;
		return success;
	}
}
