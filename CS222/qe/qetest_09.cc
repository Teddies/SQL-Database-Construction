#include <fstream>
#include <iostream>

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"

RC testCase_9() {
	// Mandatory for all
	// 1. INLJoin -- on TypeReal Attribute
	// SELECT * from left, right WHERE left.C = right.C
	cerr << endl << "***** In QE Test Case 9 *****" << endl;

	RC rc = success;

	// Prepare the iterator and condition
	TableScan *leftIn = new TableScan(*rm, "left");
	IndexScan *rightIn = new IndexScan(*rm, "right", "C");

	Condition cond;
	cond.lhsAttr = "left.C";
	cond.op = EQ_OP;
	cond.bRhsIsAttr = true;
	cond.rhsAttr = "right.C";

	int expectedResultCnt = 75; // 50.0~124.0  left.C: [50.0,149.0], right.C: [25.0,124.0]
	int actualResultCnt = 0;
	float valueC = 0;

	// Create INLJoin
	INLJoin *inlJoin = new INLJoin(leftIn, rightIn, cond);

	// Go over the data through iterator
	void *data = malloc(bufSize);
	bool nullBit = false;
	
	while (inlJoin->getNextTuple(data) != QE_EOF) {
		int offset = 0;

		// Is an attribute left.A NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 7);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		// Print left.A
		cerr << "left.A " << *(int *) ((char *) data + offset + 1);
		offset += sizeof(int);

		// Is an attribute left.B NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 6);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		// Print left.B
		cerr << "  left.B " << *(int *) ((char *) data + offset + 1);
		offset += sizeof(int);

		// Is an attribute left.C NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 5);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		// Print left.C
		cerr << "  left.C " << *(float *) ((char *) data + offset + 1);
		offset += sizeof(float);

		// Is an attribute right.B NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 4);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		// Print right.B
		cerr << "  right.B " << *(int *) ((char *) data + offset + 1);
		offset += sizeof(int);

		// Is an attribute right.C NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 3);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		// Print right.C
		valueC = *(float *) ((char *) data + offset + 1);
		cerr << "  right.C " << valueC;
		offset += sizeof(float);
		if (valueC < 50.0 || valueC > 124.0) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			rc = fail;
			goto clean_up;
		}

		// Is an attribute right.C NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 2);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		// Print right.D
		cerr << "  right.D " << *(int *) ((char *) data + offset + 1) << endl;
		offset += sizeof(int);

		memset(data, 0, bufSize);
		actualResultCnt++;
	}

	if (expectedResultCnt != actualResultCnt) {
		cerr << "***** The number of returned tuple is not correct. *****" << endl;
		rc = fail;
	}

clean_up:
	delete inlJoin;
	delete leftIn;
	delete rightIn;
	free(data);
	return rc;
}

int main() {

	if (testCase_9() != success) {
		cerr << "***** [FAIL] QE Test Case 9 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Test Case 9 finished. The result will be examined. *****" << endl;
		return success;
	}
}
