#include <fstream>
#include <iostream>

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"

RC privateTestCase_6() {
	RC rc = success;
	// Functions Tested
	// Left deep join using Index Nested Loop Join
	cerr << endl << "***** In QE Private Test Case 6 *****" << endl;

	// Prepare the iterator and condition
	TableScan *leftIn = new TableScan(*rm, "largeleft2");
	IndexScan *rightIn1 = new IndexScan(*rm, "largeright2", "B");
	IndexScan *rightIn2 = new IndexScan(*rm, "left", "B");
	
	// Set up condition
	Condition filterCond;
	filterCond.lhsAttr = "largeleft2.B";
	filterCond.op = LE_OP;
	filterCond.bRhsIsAttr = false;
	Value value1;
	value1.type = TypeInt;
	value1.data = malloc(bufSize);
	*(int *) value1.data = 109;
	filterCond.rhsValue = value1;

	// Create Filter
	Filter *filter = new Filter(leftIn, filterCond);

	Condition joinCond1;
	joinCond1.lhsAttr = "largeleft2.B";
	joinCond1.op = EQ_OP;
	joinCond1.bRhsIsAttr = true;
	joinCond1.rhsAttr = "largeright2.B";

	// Create child NLJoin
	INLJoin *childINLJoin = new INLJoin(filter, rightIn1, joinCond1);

	Condition joinCond2;
	joinCond2.lhsAttr = "largeleft2.B";
	joinCond2.op = EQ_OP;
	joinCond2.bRhsIsAttr = true;
	joinCond2.rhsAttr = "left.B";

	// Create top NLJoin
	INLJoin *inlJoin = new INLJoin(childINLJoin, rightIn2, joinCond2);

	int expectedResultCnt = 90; //20~109 --> largeleft2.B: [10,109], largeright2.B: [20,50019], left.B: [10, 109]
	int actualResultCnt = 0;
	int valueB = 0;

	// Go over the data through iterator
	void *data = malloc(bufSize);
	while (inlJoin->getNextTuple(data) != QE_EOF) {
		if (actualResultCnt % 10 == 0) { 
		
			int offset = 2; //including nulls-indicator

			// Print largeLeft.A
			cerr << "largeleft2.A " << *(int *) ((char *) data + offset);
			offset += sizeof(int);

			// Print largeLeft.B
			cerr << " B " << *(int *) ((char *) data + offset);
			offset += sizeof(int);

			// Print largeLeft.C
			cerr << " C " << *(float *) ((char *) data + offset);
			offset += sizeof(float);

			// Print largeLeft.B
			valueB =  *(int *) ((char *) data + offset);
			cerr << " largeright2.B " << valueB;
			offset += sizeof(int);

			if (valueB < 20 || valueB > 109) {
				cerr << endl << "***** [FAIL] Incorrect value: " << valueB << " returned. *****" << endl;
				rc = fail;
				goto clean_up;
			}

			// Print largeLeft.C
			cerr << " C " << *(float *) ((char *) data + offset);
			offset += sizeof(float);

			// Print largeLeft.D
			cerr << " D " << *(int *) ((char *) data + offset);
			offset += sizeof(int);

			// Print left.A
			cerr << " left.A " << *(int *) ((char *) data + offset);
			offset += sizeof(int);

			// Print largeLeft.B
			cerr << " B " << *(int *) ((char *) data + offset);
			offset += sizeof(int);

			// Print largeLeft.C
			cerr << " C " << *(float *) ((char *) data + offset) << endl;
			offset += sizeof(float);
 		}
 		
		memset(data, 0, bufSize);
		++actualResultCnt;
	}

	if (expectedResultCnt != actualResultCnt) {
        cerr << " ***** Expected Result Count: " << expectedResultCnt << endl;
		cerr << " ***** [FAIL] The number of result: " << actualResultCnt << " is not correct. ***** " << endl;
		rc = fail;
	}

clean_up:
	delete inlJoin;
	delete childINLJoin;
	delete leftIn;
	delete rightIn1;
	delete rightIn2;
	free(data);
	return rc;
}

int main() {
	
	if (privateTestCase_6() != success) {
		cerr << "***** [FAIL] QE Private Test Case 6 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Private Test Case 6 finished. The result will be examined. *****" << endl;
		return success;
	}
}
