#include <fstream>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"

int testCase_1() {
	// Mandatory for all
	// Create an Index
	// Load Data
	// Create an Index

	RC rc = success;
	cerr << endl << "***** In QE Test Case 1 *****" << endl;

	// Create an index before inserting tuples.
	rc = createIndexforLeftB();
	if (rc != success) {
		cerr << "***** createIndexforLeftB() failed.  *****" << endl;
		return rc;
	}

	// Insert tuples.
	rc = populateLeftTable();
	if (rc != success) {
		cerr << "***** populateLeftTable() failed.  *****" << endl;
		return rc;
	}

	// Create an index after inserting tuples - should reflect the currently existing tuples.
	rc = createIndexforLeftC();
	if (rc != success) {
		cerr << "***** createIndexforLeftC() failed.  *****" << endl;
		return rc;
	}
	return rc;
}


int main() {
	// Tables created: left
	// Indexes created: left.B, left.C

	// Initialize the system catalog
	if (deleteAndCreateCatalog() != success) {
		cerr << "***** deleteAndCreateCatalog() failed." << endl;
		cerr << "***** [FAIL] QE Test Case 1 failed. *****" << endl;
		return fail;
	}
	
	// Create the left table
	if (createLeftTable() != success) {
		cerr << "***** createLeftTable() failed." << endl;
		cerr << "***** [FAIL] QE Test Case 1 failed. *****" << endl;
		return fail;
	}

	if (testCase_1() != success) {
		cerr << "***** [FAIL] QE Test Case 1 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Test Case 1 finished. The result will be examined. *****" << endl;
		return success;
	}
}
