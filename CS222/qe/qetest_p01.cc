#include <fstream>
#include <iostream>

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"

RC privateTestCase_1() {
	// Function Tested
	// Insert records into table
	// Table scan with filter
	// Index scan with filter
	//compare the results from both scans
	cerr << endl << "***** In QE Private Test Case 1 *****" << endl;
	RC rc = success;

	TableScan *ts = new TableScan(*rm, "largeleft2");
	int compVal = 9999;

	// Set up condition
	Condition cond1;
	cond1.lhsAttr = "largeleft2.B";
	cond1.op = LT_OP;
	cond1.bRhsIsAttr = false;
	Value value1;
	value1.type = TypeInt;
	value1.data = malloc(bufSize);
	*(int *) value1.data = compVal;
	cond1.rhsValue = value1;

	// Create Filter
	Filter *filter1 = new Filter(ts, cond1);

	IndexScan *is = new IndexScan(*rm, "largeleft2", "B");

	// Set up condition
	Condition cond2;
	cond2.lhsAttr = "largeleft2.B";
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
	int tscount = 0;
	int iscount = 0;
	
	while (filter1->getNextTuple(data1) != QE_EOF) {
		tscount++;
		
		if (filter2->getNextTuple(data2) == QE_EOF) {
			cerr << "***** [FAIL] The numbers of results from both scan: " << count << " " << tscount << " " << iscount << " do not match. ***** " << endl;
			rc = fail;
			goto clean_up;
		} else {
			iscount++;
		}

		memset(data1, 0, bufSize);
		memset(data2, 0, bufSize);
		
		count++;
	}

	// largeleft.B < 9999 from [10-9999] = 9989
	if (count != 9989 || tscount != count || iscount != count) {
			cerr << " ***** [FAIL] The number of result: " << count << " is not correct. ***** " << endl;
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
	// Tables created: largeleft2
	// Indexes created: largeleft2.B
	
	if (createLargeLeftTable2() != success) {
		cerr << "***** [FAIL] QE Private Test Case 1 failed. *****" << endl;
		return fail;
	}

	if (createIndexforLargeLeftB2() != success) {
		cerr << "***** [FAIL] QE Private Test Case 1 failed. *****" << endl;
		return fail;
	}

	if (populateLargeLeftTable2() != success) {
		cerr << "***** [FAIL] QE Private Test Case 1 failed. *****" << endl;
		return fail;
	}

	
	if (privateTestCase_1() != success) {
		cerr << "***** [FAIL] QE Private Test Case 1 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Private Test Case 1 finished. The result will be examined. *****" << endl;
		return success;
	}
}
