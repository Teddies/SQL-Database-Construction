#include <fstream>
#include <iostream>

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"

RC testCase_6() {
	// Mandatory for all
	// Project -- TableScan as input
	// SELECT C,D FROM RIGHT
	cout << endl << "***** In QE Test Case 6 *****" << endl;

	RC rc = success;
	TableScan *ts = new TableScan(*rm, "right");

	vector<string> attrNames;
	attrNames.push_back("right.C");
	attrNames.push_back("right.D");

	int expectedResultCnt = 100;
	int actualResultCnt = 0;
	float valueC = 0.0;
	int valueD = 0;

	// Create Projector
	Project *project = new Project(ts, attrNames);

	// Go over the data through iterator
	void *data = malloc(bufSize);
	bool nullBit = false;
	
	while (project->getNextTuple(data) != QE_EOF) {
		int offset = 0;

		// Is an attribute C NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 7);
		if (nullBit) {
			cout << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		valueC = *(float *)((char *)data+1+offset);
		
		// Print right.C
		cout << "right.C " << valueC;
		offset += sizeof(float);


		// Is an attribute D NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 6);
		if (nullBit) {
			cout << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}

		// Print right.D
		valueD = *(int *)((char *)data+1+offset);
		cout << "  right.D " << valueD << endl;
		offset += sizeof(int);
		if (valueD < 0 || valueD > 99) {
			cout << endl << "***** A returned value is not correct. *****" << endl;
			rc = fail;
			goto clean_up;
		}

		memset(data, 0, bufSize);
		actualResultCnt++;
	}

	if (expectedResultCnt != actualResultCnt) {
		cout << "***** The number of returned tuple is not correct. *****" << endl;
		rc = fail;
	}

clean_up:
	delete project;
	delete ts;
	free(data);
	return rc;
}

int main() {
	// Tables created: none
	// Indexes created: none

	if (testCase_6() != success) {
		cout << "***** [FAIL] QE Test Case 6 failed. *****" << endl;
		return fail;
	} else {
		cout << "***** QE Test Case 6 finished. The result will be examined. *****" << endl;
		return success;
	}
}
