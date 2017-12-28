#include <fstream>
#include <iostream>

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"

RC privateTestCase_8() {
	// Optional for all
	// (+10 extra credit points will be given based on the results of the GHJ related tests)
	// 1. GHJoin -- on Varchar Attribute
	// 2. GHJoin -- on INT Attribute
	cerr << endl << "***** In QE Test Private Test Case 8 *****" << endl;

	RC rc = success;

	// Prepare the iterator and condition
	TableScan *leftIn = new TableScan(*rm, "leftvarchar");
	TableScan *rightIn = new TableScan(*rm, "rightvarchar");
	TableScan *anotherRightIn = new TableScan(*rm, "left");

	// Set up condition
	Condition filterCond;
	filterCond.lhsAttr = "leftvarchar.A";
	filterCond.op = LE_OP;
	filterCond.bRhsIsAttr = false;
	Value value1;
	value1.type = TypeInt;
	value1.data = malloc(bufSize);
	*(int *) value1.data = 45; // A[20-45], then B: a ... zzzzzzzzzzzzzzzzzzzzzzzzzz
	filterCond.rhsValue = value1;

	// Create Filter
	Filter *filter = new Filter(leftIn, filterCond);

	// Set up condition
	Condition filterCond2;
	filterCond2.lhsAttr = "rightvarchar.C";
	filterCond2.op = LE_OP;
	filterCond2.bRhsIsAttr = false;
	Value value2;
	value2.type = TypeReal;
	value2.data = malloc(bufSize);
	*(float *) value2.data = 35.0; // C[10.0-35.0], then B: a ... zzzzzzzzzzzzzzzzzzzzzzzzzz
	filterCond2.rhsValue = value2;

	// Create Filter
	Filter *filter2 = new Filter(rightIn, filterCond2);

	Condition cond;
	cond.lhsAttr = "leftvarchar.B";
	cond.op = EQ_OP;
	cond.bRhsIsAttr = true;
	cond.rhsAttr = "rightvarchar.B";

	Condition cond2;
	cond2.lhsAttr = "leftvarchar.A"; // [20 - 45]
	cond2.op = EQ_OP;
	cond2.bRhsIsAttr = true;
	cond2.rhsAttr = "left.A"; // [0 - 99]

	int expectedResultCnt = 26; 
	int actualResultCnt = 0;
	float valueC = 0;
	int numPartitons = 5;
	
	// Create GHJoin
	GHJoin *ghJoin = new GHJoin(filter, filter2, cond, numPartitons);

	GHJoin *ghJoin2 = new GHJoin(ghJoin, anotherRightIn, cond2, numPartitons);

	// Go over the data through iterator
	void *data = malloc(bufSize);
	bool nullBit = false;
	
	while (ghJoin2->getNextTuple(data) != QE_EOF) {

		// At this point, partitions should be on disk.

		cerr << (actualResultCnt+1) << " / " << expectedResultCnt << " tuples: ";
		int offset = 0;
		// is an attribute leftvarchar.A NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 7);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		// Print left.A
		int leftVarcharA = *(int *) ((char *) data + offset + 1);
		cerr << "leftvarchar.A " << leftVarcharA;
		offset += sizeof(int);

		// is an attribute left.B NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 6);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		// Print left.B
		int length = *(int *) ((char *) data + offset + 1);
		offset += 4;

		char *b = (char *) malloc(100);
		memcpy(b, (char *) data + offset + 1, length);
		b[length] = '\0';
		offset += length;
		cerr << " B " << b;

		// is an attribute right.C NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 4);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		
		// skip rightvarchar.B
		offset = offset + 4 + length;
		
		// Print right.C
		cerr << "  rightvarchar.C " << *(float *) ((char *) data + offset + 1) << endl;
		offset += sizeof(float);

		// left.A
		int la = *(int *) ((char *) data + offset + 1);
		offset += sizeof(int);

		// left.B
		int lb = *(int *) ((char *) data + offset + 1);
		if (la != leftVarcharA || lb != (leftVarcharA + 10)) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		offset += sizeof(int);

		// left.C
		float lc = *(float *) ((char *) data + offset + 1);
		offset += sizeof(float);

		memset(data, 0, bufSize);
		actualResultCnt++;
	}

	if (expectedResultCnt != actualResultCnt) {
		cerr << "***** The number of returned tuple: " << actualResultCnt << " is not correct. *****" << endl;
		rc = fail;
	}

clean_up:
	delete ghJoin;
	delete ghJoin2;
	delete filter;
	delete filter2;
	delete leftIn;
	delete rightIn;
	delete anotherRightIn;
	free(data);
	return rc;
}

int main() {
	
	if (privateTestCase_8() != success) {
		cerr << "***** [FAIL] QE Private Test Case 8 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Private Test Case 8 finished. The result will be examined. *****" << endl;
		return success;
	}
}
