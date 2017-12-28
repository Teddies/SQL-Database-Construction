#include <fstream>
#include <iostream>

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"

RC privateTestCase_2() {
	// Function Tested
	// Insert records into table with NULL values
	// Table scan with filter
	// Index scan with filter
	// compare the results from both scans
	cerr << endl <<  "***** In QE Private Test Case 2 *****" << endl;
	RC rc = success;

	TableScan *ts = new TableScan(*rm, "left2");
	int compVal = 50;

	// Set up condition
	Condition cond1;
	cond1.lhsAttr = "left2.B";
	cond1.op = LT_OP;
	cond1.bRhsIsAttr = false;
	Value value1;
	value1.type = TypeInt;
	value1.data = malloc(bufSize);
	*(int *) value1.data = compVal;
	cond1.rhsValue = value1;

	// Create Filter
	Filter *filter1 = new Filter(ts, cond1);

	IndexScan *is = new IndexScan(*rm, "left2", "B");

	// Set up condition
	Condition cond2;
	cond2.lhsAttr = "left2.B";
	cond2.op = LT_OP;
	cond2.bRhsIsAttr = false;
	Value value2;
	value2.type = TypeInt;
	value2.data = malloc(bufSize);
	*(int *) value2.data = compVal;
	cond2.rhsValue = value2;

	// Create Filter
	Filter *filter2 = new Filter(is, cond2);

	// Go over the data through iterator
	char data1[bufSize];
	char data2[bufSize];
	int offset;
	int tsVal;
	int isVal;
	memset(data1, 0, bufSize);
	memset(data2, 0, bufSize);
	int count = 0;
	bool nullBit1;
	bool nullBit2;
	int nullCount1 = 0;
	int nullCount2 = 0;
	int tscount = 0;
	int iscount = 0;
	
	while (filter1->getNextTuple(data1) != QE_EOF) {
		tscount++;
		
		if (filter2->getNextTuple(data2) == QE_EOF) {
			cerr << " ***** [FAIL] The numbers of results: " << count << " " << tscount << " " << iscount << " from both scan do not match. ***** " << endl;
			rc = fail;
			goto clean_up;
		} else {
			iscount++;
		}

		offset = 0; // including null indicators
		
		// Compare field A value
		nullBit1 = *(unsigned char *)((char *)data1 + offset) & (1 << 7);
		nullBit2 = *(unsigned char *)((char *)data2 + offset) & (1 << 7);
		
		if (!nullBit1) {
			tsVal = *(int *) ((char *) data1 + offset);
		} else {
			tsVal = -9999; // Assign an arbitrary number
			nullCount1++;
		}
		
		if (!nullBit2) {
			isVal = *(int *) ((char *) data2 + offset);
		} else {
			isVal = -9999; // Assign an arbitrary number
			nullCount2++;
		}

		offset++;

		memset(data1, 0, bufSize);
		memset(data2, 0, bufSize);
		
		count++;
	}

	// left.B [10-109] : 40
	if (count != 40 || nullCount1 != 20 || nullCount2 != 20) {
			cerr << "***** [FAIL] The number of result: " << count << " " << nullCount1 << " " << nullCount2 << " is not correct. ***** " << endl;
			rc = fail;
	}
	
clean_up:
	delete filter1;
	delete filter2;
	delete ts;
	delete is;
	free(value1.data);
	free(value2.data);
	return rc;
}



int main() {
	// Tables created: left2
	// Indexes created: left2.B
	
	if (createLeftTable2() != success) {
		cerr << "***** [FAIL] QE Private Test Case 2 failed. *****" << endl;
		return fail;
	}

	if (createIndexforLeftB2() != success) {
		cerr << "***** [FAIL] QE Private Test Case 2 failed. *****" << endl;
		return fail;
	}

	if (populateLeftTable2() != success) {
		cerr << "***** [FAIL] QE Private Test Case 2 failed. *****" << endl;
		return fail;
	}
	
	if (privateTestCase_2() != success) {
		cerr << "***** [FAIL] QE Private Test Case 2 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Private Test Case 2 finished. The result will be examined. *****" << endl;
		return success;
	}
}
