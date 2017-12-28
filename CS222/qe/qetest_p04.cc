#include <fstream>
#include <iostream>

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"

RC privateTestCase_4() {
	RC rc = success;
	// Functions Tested
	// Filter
	// BNLJoin - optional for undergrad solo
	cerr << endl << "***** In QE Private Test Case 4 *****" << endl;

	// Prepare the iterator and condition
	TableScan *leftIn = new TableScan(*rm, "largeleft2");
	TableScan *rightIn = new TableScan(*rm, "largeright2");

	// Set up condition
	Condition filterCond;
	filterCond.lhsAttr = "largeleft2.B";
	filterCond.op = LE_OP;
	filterCond.bRhsIsAttr = false;
	Value value1;
	value1.type = TypeInt;
	value1.data = malloc(bufSize);
	*(int *) value1.data = 1109;
	filterCond.rhsValue = value1;

	// Create Filter
	Filter *filter = new Filter(leftIn, filterCond);

	Condition joinCond;
	joinCond.lhsAttr = "largeleft2.B";
	joinCond.op = EQ_OP;
	joinCond.bRhsIsAttr = true;
	joinCond.rhsAttr = "largeright2.B";

	int expectedResultCnt = 1090; //20~1109 --> largeleft2.B: [10,1109], largeright2.B: [20,50019]
	int actualResultCnt = 0;
	int valueB = 0;

	// Create BNLJoin
	BNLJoin *bnlJoin = new BNLJoin(filter, rightIn, joinCond, 50);

	// Go over the data through iterator
	void *data = malloc(bufSize);
	while (bnlJoin->getNextTuple(data) != QE_EOF) {

		actualResultCnt++;
		int offset = 1; // including nulls-indicator

		int la = 0;
		int lb = 0;
		float lc = 0.0;
		int rb = 0;
		float rc = 0.0;
		int rd = 0;

		// left.A
		la = *(int *) ((char *) data + offset);
		offset += sizeof(int);

		// left.B
		lb = *(int *) ((char *) data + offset);
		offset += sizeof(int);

		// left.C
		lc = *(float *) ((char *) data + offset);
		offset += sizeof(float);

		// right.B
		valueB =  *(int *) ((char *) data + offset);
		offset += sizeof(int);

		// right.C
		rc = *(float *) ((char *) data + offset);
		offset += sizeof(float);

		// right.D
		rd = *(int *) ((char *) data + offset);
		offset += sizeof(int);

		if (valueB < 20 || valueB > 1109) {
			cerr << "***** [FAIL] Incorrect value: " << valueB << " returned. *****" << endl;
			cerr << "count:" << actualResultCnt << " lA:" << la << " lB:" << lb << " lC:" << lc << " rB:" << valueB << " rC:" << rc << " rD:" << rd << endl;
			rc = fail;
			goto clean_up;
		}

		memset(data, 0, bufSize);
	}

	if (expectedResultCnt != actualResultCnt) {
        cerr << " ***** Expected Result Count: " << expectedResultCnt << endl;
		cerr << " ***** [FAIL] The number of result: " << actualResultCnt << " is not correct. ***** " << endl;
		rc = fail;
	}

clean_up:
	delete bnlJoin;
	delete filter;
	delete leftIn;
	delete rightIn;
	free(data);
	return rc;
}

int main() {
	// Tables created: largeright2
	// Indexes created: largeright2.B
	
	if (createLargeRightTable2() != success) {
		cerr << "***** [FAIL] QE Private Test Case 4 failed. *****" << endl;
		return fail;
	}

	if (populateLargeRightTable2() != success) {
		cerr << "***** [FAIL] QE Private Test Case 4 failed. *****" << endl;
		return fail;
	}

	if (createIndexforLargeRightB2() != success) {
		cerr << "***** [FAIL] QE Private Test Case 4 failed. *****" << endl;
		return fail;
	}
	
	if (privateTestCase_4() != success) {
		cerr << "***** [FAIL] QE Private Test Case 4 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Private Test Case 4 finished. The result will be examined. *****" << endl;
		return success;
	}
}
