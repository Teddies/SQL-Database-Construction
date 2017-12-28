#include <fstream>
#include <iostream>

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"

RC privateTestCase_3() {
	// Functions Tested
	// Inserting more records into table
	// Table scan with filter: largeleft2.B >= largeTupleCount * 2 - 990
	// Index scan with filter: largeleft2.B >= largeTupleCount * 2 - 990
	// Compare the results from both scans
	cout << endl << "***** In QE Private Test Case 3 *****" << endl;
	RC rc = success;

	TableScan *ts = new TableScan(*rm, "largeleft2");
	int compVal = largeTupleCount * 2 - 990;

	// Set up condition
	Condition cond1;
	cond1.lhsAttr = "largeleft2.B";
	cond1.op = GE_OP;
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
	cond2.op = GE_OP;
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
			cout << "***** [FAIL] The numbers of results from both scan: " << count << " " << tscount << " " << iscount << " do not match. ***** " << endl;
			rc = fail;
			goto clean_up;
		} else {
			iscount++;
		}

		offset = 1; // including nulls-indicator
 		offset += sizeof(int); // skip the field A

		//compare field B value
 		tsVal = *(int *) ((char *) data1 + offset);
 		isVal = *(int *) ((char *) data2 + offset);
 		if (tsVal < compVal || isVal < compVal) {
			cerr << "***** [FAIL] Incorrect scan value failure: table_scan.B " << tsVal << " : index_scan.B " << isVal << " *****" << endl;
			rc = fail;
			goto clean_up;
 		}

		memset(data1, 0, bufSize);
		memset(data2, 0, bufSize);

		count++;
	}

	if (count != 1000) {
			cout << " ***** [FAIL] The number of result: " << count << " is not correct. ***** " << endl;
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
	
	if (addRecordsToLargeLeftTable2() != success) {
		cout << "***** [FAIL] QE Private Test Case 3 failed. *****" << endl;
		return fail;
	}

	if (privateTestCase_3() != success) {
		cout << "***** [FAIL] QE Private Test Case 3 failed. *****" << endl;
		return fail;
	} else {
		cout << "***** QE Private Test Case 3 finished. The result will be examined. *****" << endl;
		return success;
	}
}
