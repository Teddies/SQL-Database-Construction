#include <fstream>
#include <iostream>

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"

RC testCase_14() {
	// Mandatory for the grad teams/solos
	// Optional for the undergrad solos: +5 extra credit points will be given based on the results of the basic aggregation related tests.
	// 1. Basic aggregation - AVG
	// SELECT AVG(right.B) from left

    cerr << "***** In QE Test Case 14 *****" << endl;

    // Create TableScan
    TableScan *input = new TableScan(*rm, "right");

    RC rc = success;

    // Create Aggregate
    Attribute aggAttr;
    aggAttr.name = "right.B";
    aggAttr.type = TypeInt;
    aggAttr.length = 4;
    Aggregate *agg = new Aggregate(input, aggAttr, AVG);

    void *data = malloc(bufSize);
    float average = 0.0;
	int count = 0;
	    
    while(agg->getNextTuple(data) != QE_EOF)
    {
    	average = *(float *) ((char *) data + 1);
        cerr << "AVG(right.B) " << average << endl;
        memset(data, 0, sizeof(float)+1);
        count++;
        if (count > 1) {
        	cerr << "***** The number of returned tuple is not correct. *****" << endl;
        	rc = fail;
        	break;
        }
    }

    if (average != 69.5) {
    	rc = fail;
    }

    delete agg;
    delete input;
    free(data);
    return rc;
}


int main() {
	
	if (testCase_14() != success) {
		cerr << "***** [FAIL] QE Test Case 14 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Test Case 14 finished. The result will be examined. *****" << endl;
		return success;
	}
}
