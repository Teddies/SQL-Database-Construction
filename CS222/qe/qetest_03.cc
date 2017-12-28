#include <fstream>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"


RC testCase_3() {
	// Mandatory for all
	// Filter -- TableScan as input, on an Integer Attribute
	// SELECT * FROM LEFT WHERE B <= 30
	cerr << endl << "***** In QE Test Case 3 *****" << endl;
	RC rc = success;

	TableScan *ts = new TableScan(*rm, "left");
	int compVal = 30;
	int valueB = 0;

	// Set up condition
	Condition cond;
	cond.lhsAttr = "left.B";
	cond.op = LE_OP;
	cond.bRhsIsAttr = false;
	Value value;
	value.type = TypeInt;
	value.data = malloc(bufSize);
	*(int *) value.data = compVal;
	cond.rhsValue = value;

	int expectedResultCnt = 21;  // 10~30;
	int actualResultCnt = 0;

	// Create Filter
	Filter *filter = new Filter(ts, cond);

	// Go over the data through iterator
	void *data = malloc(bufSize);
	bool nullBit = false;
	
	int valueA = 0;
	float valueC = 0.0;
	
	while (filter->getNextTuple(data) != QE_EOF) {
		int offset = 0;
		// Print left.A
		// Null indicators should be placed in the beginning.

		// Is an attribute A NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 7);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			rc = fail;
			goto clean_up;
		}
		valueA = *(int *)((char *)data+1+offset);

		cerr << "left.A " << valueA;
		offset += sizeof(int);

		// Is an attribute B NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 6);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			rc = fail;
			goto clean_up;
		}

		// Print left.B
		valueB = *(int *)((char *)data+1+offset);
		cerr << "  left.B " << valueB;
		offset += sizeof(int);
		if (valueB > compVal) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			rc = fail;
			goto clean_up;
		}


		// Is an attribute C NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 5);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			rc = fail;
			goto clean_up;
		}
		valueC = *(float *)((char *)data+1+offset);
		
		// Print left.C
		cerr << "  left.C " << valueC << endl;
		offset += sizeof(float);

		memset(data, 0, bufSize);
		actualResultCnt++;
	}

	if (expectedResultCnt != actualResultCnt) {
		cerr << "***** The number of returned tuple is not correct. *****" << endl;
		rc = fail;
	}

clean_up:
	delete filter;
	delete ts;
	free(value.data);
	free(data);
	return rc;
}


int main() {
	// Tables created: none
	// Indexes created: none

	if (testCase_3() != success) {
		cerr << "***** [FAIL] QE Test Case 3 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Test Case 3 finished. The result will be examined. *****" << endl;
		return success;
	}
}
