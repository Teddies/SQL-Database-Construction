#include <fstream>
#include <iostream>

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"

RC testCase_11() {
	// Optional for all: +10 extra credit points will be given based on the results of the GHJ related tests.
	// 1. GHJoin -- on TypeReal Attribute
	// SELECT * from largeleft, largeright WHERE largeleft.C = largeright.C
	cerr << endl << "***** In QE Test Case 11 *****" << endl;

	RC rc = success;

	// Prepare the iterator and condition
	TableScan *leftIn = new TableScan(*rm, "largeleft");
	TableScan *rightIn = new TableScan(*rm, "largeright");

	Condition cond;
	cond.lhsAttr = "largeleft.C";
	cond.op = EQ_OP;
	cond.bRhsIsAttr = true;
	cond.rhsAttr = "largeright.C";

	int expectedResultCnt = 49975; // 50.0~50024.0  left.C: [50.0,50049.0], right.C: [25.0,50024.0]
	int actualResultCnt = 0;
	float valueC = 0;
	int numPartitons = 10;
	
	// Create GHJoin
	GHJoin *ghJoin = new GHJoin(leftIn, rightIn, cond, numPartitons);

	// Go over the data through iterator
	void *data = malloc(bufSize);
	bool nullBit = false;
	
	while (ghJoin->getNextTuple(data) != QE_EOF) {

		// At this point, partitions should be on disk.

		if (actualResultCnt % 5000 == 0) {
			cerr << "Processing " << actualResultCnt << " of " << largeTupleCount << " tuples." << endl;
			int offset = 0;
			// Is an attribute left.A NULL?
			nullBit = *(unsigned char *)((char *)data) & (1 << 7);
			if (nullBit) {
				cerr << endl << "***** A returned value is not correct. *****" << endl;
				goto clean_up;
			}
			// Print left.A
			cerr << "largeleft.A " << *(int *) ((char *) data + offset + 1);
			offset += sizeof(int);

			// Is an attribute left.B NULL?
			nullBit = *(unsigned char *)((char *)data) & (1 << 6);
			if (nullBit) {
				cerr << endl << "***** A returned value is not correct. *****" << endl;
				goto clean_up;
			}
			// Print left.B
			cerr << "  largeleft.B " << *(int *) ((char *) data + offset + 1);
			offset += sizeof(int);

			// Is an attribute left.C NULL?
			nullBit = *(unsigned char *)((char *)data) & (1 << 5);
			if (nullBit) {
				cerr << endl << "***** A returned value is not correct. *****" << endl;
				goto clean_up;
			}
			// Print left.C
			cerr << "  largeleft.C " << *(float *) ((char *) data + offset + 1);
			offset += sizeof(float);

			// Is an attribute right.B NULL?
			nullBit = *(unsigned char *)((char *)data) & (1 << 5);
			if (nullBit) {
				cerr << endl << "***** A returned value is not correct. *****" << endl;
				goto clean_up;
			}
			// Print right.B
			cerr << "  largeright.B " << *(int *) ((char *) data + offset + 1);
			offset += sizeof(int);

			// Is an attribute right.C NULL?
			nullBit = *(unsigned char *)((char *)data) & (1 << 4);
			if (nullBit) {
				cerr << endl << "***** A returned value is not correct. *****" << endl;
				goto clean_up;
			}
			// Print right.C
			valueC = *(float *) ((char *) data + offset + 1);
			cerr << "  largeright.C " << valueC;
			offset += sizeof(float);
			if (valueC < 50.0 || valueC > 50024.0) {
				cerr << endl << "***** A returned value is not correct. *****" << endl;
				rc = fail;
				goto clean_up;
			}

			// Is an attribute right.D NULL?
			nullBit = *(unsigned char *)((char *)data) & (1 << 3);
			if (nullBit) {
				cerr << endl << "***** A returned value is not correct. *****" << endl;
				goto clean_up;
			}
			// Print right.D
			cerr << "  largeright.D " << *(int *) ((char *) data + offset + 1) << endl;
			offset += sizeof(int);
		}

		memset(data, 0, bufSize);
		actualResultCnt++;

	}

	if (expectedResultCnt != actualResultCnt) {
		cerr << "***** The number of returned tuple is not correct. *****" << endl;
		rc = fail;
	}

clean_up:
	delete ghJoin;
	delete leftIn;
	delete rightIn;
	free(data);
	return rc;
}

int main() {
	// Tables created: largeleft, largeright
	// Indexes created: none

	// Create left/right large table, and populate the table
	if (createLargeLeftTable() != success) {
		cerr << "***** [FAIL] QE Test Case 11 failed. *****" << endl;
		return fail;
	}

	if (populateLargeLeftTable() != success) {
		cerr << "***** [FAIL] QE Test Case 11 failed. *****" << endl;
		return fail;
	}

	if (createLargeRightTable() != success) {
		cerr << "***** [FAIL] QE Test Case 11 failed. *****" << endl;
		return fail;
	}

	if (populateLargeRightTable() != success) {
		cerr << "***** [FAIL] QE Test Case 11 failed. *****" << endl;
		return fail;
	}
	
	if (testCase_11() != success) {
		cerr << "***** [FAIL] QE Test Case 11 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Test Case 11 finished. The result will be examined. *****" << endl;
		return success;
	}
}
