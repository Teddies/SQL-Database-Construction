#include "rm_test_util.h"

int main()
{

  // By executing this script, the following tables including the system tables will be removed.
  cout << endl << "***** RM TEST - Deleting the Catalog and User tables *****" << endl;

  RC rc = rm->deleteTable("tbl_employee");
  if (rc != 0) {
	  cout << "Deleting tbl_employee failed." << endl;
  }

  rc = rm->deleteTable("tbl_employee2");
  if (rc != 0) {
	  cout << "Deleting tbl_employee2 failed." << endl;
  }

  rc = rm->deleteTable("tbl_employee3");
  if (rc != 0) {
	  cout << "Deleting tbl_employee3 failed." << endl;
  }

  rc = rm->deleteTable("tbl_employee4");
  if (rc != 0) {
	  cout << "Deleting tbl_employee4 failed." << endl;
  }

  rc = rm->deleteCatalog();
  if (rc != 0) {
	  cout << "Deleting the catalog failed." << endl;
	  return rc;
  }

  return success;
}
