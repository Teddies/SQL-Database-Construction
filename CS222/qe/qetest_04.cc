#include <fstream>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"

int testCase_4() {
	// Mandatory for all
	// 1. Filter -- on TypeVarChar Attribute
	// SELECT * FROM leftvarchar where B = "llllllllllll"
	cerr << endl << "***** In QE Test Case 4 *****" << endl;

	RC rc = success;
	TableScan *ts = new TableScan(*rm, "leftvarchar");

	// Set up condition
	Condition cond;
	cond.lhsAttr = "leftvarchar.B";
	cond.op = EQ_OP;
	cond.bRhsIsAttr = false;
	Value value;
	value.type = TypeVarChar;
	value.data = malloc(bufSize);
	int length = 12;
	*(int *) ((char *) value.data) = length;
	for (unsigned i = 0; i < 12; ++i) {
		*(char *) ((char*)value.data + 4 + i) = 12 + 96;
	}
	cond.rhsValue = value; // "llllllllllll"

	// Create Filter
	Filter *filter = new Filter(ts, cond);

	int expectedResultCnt = 39;
	int actualResultCnt = 0;

	// Go over the data through iterator
	void *data = malloc(bufSize);
	bool nullBit = false;

	while (filter->getNextTuple(data) != QE_EOF) {
		int offset = 0;

		// Null indicators should be placed in the beginning.

		// Is an attribute A NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 7);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			rc = fail;
			goto clean_up;
		}

		// Print leftvarchar.A
		cerr << "leftvarchar.A " << *(int *) ((char *) data + offset + 1);
		offset += sizeof(int);

		// Is an attribute B NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 6);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			rc = fail;
			goto clean_up;
		}

		// Print leftvarchar.B
		int length = *(int *) ((char *) data + offset + 1);
		offset += 4;
		cerr << "  leftvarchar.B.length " << length;

		char *b = (char *) malloc(100);
		memcpy(b, (char *) data + offset + 1, length);
		b[length] = '\0';
		offset += length;
		cerr << "  leftvarchar.B " << b << endl;

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
	free(data);
	free(value.data);
	return rc;
}

int main() {
	// Tables created: leftvarchar, rightvarchar
	// Indexes created: none

	// Create left/right large table, and populate the two tables
	if (createLeftVarCharTable() != success) {
		cerr << "***** [FAIL] QE Test Case 4 failed. *****" << endl;
		return fail;
	}

	if (populateLeftVarCharTable() != success) {
		cerr << "***** [FAIL] QE Test Case 4 failed. *****" << endl;
		return fail;
	}

	if (createRightVarCharTable() != success) {
		cerr << "***** [FAIL] QE Test Case 4 failed. *****" << endl;
		return fail;
	}

	if (populateRightVarCharTable() != success) {
		cerr << "***** [FAIL] QE Test Case 4 failed. *****" << endl;
		return fail;
	}

	if (testCase_4() != success) {
		cerr << "***** [FAIL] QE Test Case 4 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Test Case 4 finished. The result will be examined. *****" << endl;
		return success;
	}
}
