#include <fstream>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"

RC testCase_2() {
	// Mandatory for all
	// Create an Index
	// Load Data
	// Create an Index
	
	RC rc = success;
	cerr << endl << "***** In QE Test Case 2 *****" << endl;

	rc = createIndexforRightB();
	if (rc != success) {
		cerr << "***** createIndexforRightB() failed.  *****" << endl;
		return rc;
	}

	rc = populateRightTable();
	if (rc != success) {
		cerr << "***** populateRightTable() failed.  *****" << endl;
		return rc;
	}

	rc = createIndexforRightC();
	if (rc != success) {
		cerr << "***** createIndexforRightC() failed.  *****" << endl;
		return rc;
	}

	return rc;
}

int main() {
	// Tables created: right
	// Indexes created: right.B, right.C

	// Create the right table
	if (createRightTable() != success) {
		cerr << "***** createRightTable() failed. *****" << endl;
		cerr << "***** [FAIL] QE Test Case 2 failed. *****" << endl;
		return fail;
	}

	if (testCase_2() != success) {
		cerr << "***** [FAIL] QE Test Case 2 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Test Case 2 finished. The result will be examined. *****" << endl;
		return success;
	}
}
