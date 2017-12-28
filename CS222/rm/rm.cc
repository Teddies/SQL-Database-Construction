
#include "rm.h"

RelationManager* RelationManager::instance()
{
    static RelationManager _rm;
    return &_rm;
}

RelationManager::RelationManager()
{
	id = 0; oo = 0;
}

RelationManager::~RelationManager()
{
}

RC RelationManager::tablesRecordGenerator(const int &id, const string &tableName, const string &fileName, void *record)
{
	int length1 = tableName.length();
	int length2 = fileName.length();
	memset(record, 0, 1);
	memcpy((char *)record + 1, &id, 4);
	memcpy((char *)record + 5, &length1, 4);
	memcpy((char *)record + 9, tableName.c_str(), length1);
	memcpy((char *)record + 9 + length1, &length2, 4);
	memcpy((char *)record + 13 + length1, fileName.c_str(), length2);
	return 0;
}

RC RelationManager::columnsRecordGenerator(const int &id, const string &attrName, const int &type, const int &len, const int &pos, void *record)
{
	int length = attrName.length();
	memset(record, 0, 1);
	memcpy((char *)record + 1, &id, 4);
	memcpy((char *)record + 5, &length, 4);
	memcpy((char *)record + 9, attrName.c_str(), length);
	memcpy((char *)record + 9 + length, &type, 4);
	memcpy((char *)record + 13 + length, &len, 4);
	memcpy((char *)record + 17 + length, &pos, 4);
	return 0;
}

RC RelationManager::getCatalogAttribute(vector<Attribute> &attrs1, vector<Attribute> &attrs2)
{
	Attribute attr;
	attr.name = "table-id";
	attr.type = TypeInt;
	attr.length = (AttrLength)4;
	attrs1.push_back(attr);
	attrs2.push_back(attr);
	attr.name = "table-name";
	attr.type = TypeVarChar;
	attr.length = (AttrLength)50;
	attrs1.push_back(attr);
	attr.name = "file-name";
	attrs1.push_back(attr);
	attr.name = "column-name";
	attrs2.push_back(attr);
	attr.name = "column-type";
	attr.type = TypeInt;
	attr.length = (AttrLength)4;
	attrs2.push_back(attr);
	attr.name = "column-length";
	attrs2.push_back(attr);
	attr.name = "column-position";
	attrs2.push_back(attr);
	return 0;
}

RC RelationManager::getCliCatalogAttribute(vector<Attribute> &attrs1, vector<Attribute> &attrs2) {
	Attribute attr;
	attr.name = "table_name";
	attr.type = TypeVarChar;
	attr.length = 50;
	attrs1.push_back(attr);

	attr.name = "file_location";
	attr.type = TypeVarChar;
	attr.length = 100;
	attrs1.push_back(attr);

	attr.name = "type";
	attr.type = TypeVarChar;
	attr.length = 20;
	attrs1.push_back(attr);

  	attr.name = "column_name";
	attr.type = TypeVarChar;
	attr.length = 30;
	attrs2.push_back(attr);

	attr.name = "table_name";
	attr.type = TypeVarChar;
	attr.length = 50;
	attrs2.push_back(attr);

	attr.name = "position";
	attr.type = TypeInt;
	attr.length = 4;
	attrs2.push_back(attr);

	attr.name = "type";
	attr.type = TypeInt;
	attr.length = 4;
	attrs2.push_back(attr);

	attr.name = "length";
	attr.type = TypeInt;
	attr.length = 4;
	attrs2.push_back(attr);
	return 0;
}

RC RelationManager::createCatalog()
{
	RC rc1 = _rbf_manager->createFile("Tables");
	RC rc2 = _rbf_manager->createFile("Columns");
	if (rc1 != 0 || rc2 != 0) {
		return -1;
	}

	// Construct and insert the first two records of Tables
	string tableName1, tableName2;
	tableName1 = "Tables";
	tableName2 = "Columns";
	vector<Attribute> attrs1;
	vector<Attribute> attrs2;

	getCatalogAttribute(attrs1, attrs2);
	createTable(tableName1, attrs1);
	createTable(tableName2, attrs2);

	return 0;
}

RC RelationManager::deleteCatalog()
{
	RM_ScanIterator rmsi;
	vector<string> attributes;
	attributes.push_back("file-name");
	scan("Tables", "", NO_OP, NULL, attributes, rmsi);
	RID rid;
	void *returnedData = malloc(4000);
	vector<string> tableNameAttr;
	while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
	{
		int size = *(int *)((char *)returnedData + 1);
		char *buffer = (char *)malloc(size + 1);
		memcpy(buffer, (char *)returnedData + 4 + 1, size);
		buffer[size] = 0;
		tableNameAttr.push_back(buffer);
	}
	rmsi.close();
	free(returnedData);

	RC rc;
	for (unsigned int i = 0; i < tableNameAttr.size(); i++) {
		rc = _rbf_manager->destroyFile(tableNameAttr[i]);
		if (rc != 0) {
			return -1;
		}
	}
    return 0;
}

RC RelationManager::createTable(const string &tableName, const vector<Attribute> &attrs)
{
	void *record = malloc(PAGE_SIZE);
	RID rid;
	string fileName = tableName;
	string tableName1 = "Tables";
	string tableName2 = "Columns";

    // Potential problems: when creating new file in the beginning, some catalog info will be written into this new file (don't know why)
	if (fileName != tableName1 && fileName != tableName2) {
		RC rc = _rbf_manager->createFile(fileName);
		if (rc != 0) {
			return -1;
		}
	}

	vector<Attribute> attrs1;
	vector<Attribute> attrs2;
	getCatalogAttribute(attrs1, attrs2);

	FileHandle fileHandle1, fileHandle2;
	_rbf_manager->openFile(tableName1, fileHandle1);
	_rbf_manager->openFile(tableName2, fileHandle2);

	id = fileHandle1.id;
	id ++;
    tablesRecordGenerator(id, tableName, fileName, record);
    _rbf_manager->insertRecord(fileHandle1, attrs1, record, rid);
    memset(record, 0, PAGE_SIZE);
    for (unsigned int i = 0; i < attrs.size(); i++) {
    	string attrName = attrs[i].name;
    	int attrType = (int)attrs[i].type;
    	int attrLength = (int)attrs[i].length;
    	columnsRecordGenerator(id, attrName, attrType, attrLength, i, record);
    	_rbf_manager->insertRecord(fileHandle2, attrs2, record, rid);
    	memset(record, 0, PAGE_SIZE);
    }
    fileHandle1.id = id;
    fileHandle2.id = id;
	_rbf_manager->closeFile(fileHandle1);
	_rbf_manager->closeFile(fileHandle2);
    free(record);

	return 0;
}

RC RelationManager::deleteTable(const string &tableName)
{
	FileHandle fileHandle;
	if (_rbf_manager->openFile(tableName, fileHandle) != 0) {
		return -1;
	}
	string fileName = tableName;
	string tableName1 = "Tables";
	string tableName2 = "Columns";
	// cannot delete catalog by deleteTable function
	if (fileName == tableName1 || fileName == tableName2) {
		return -1;
	}

	vector<Attribute> attrs1;
	vector<Attribute> attrs2;
	getCatalogAttribute(attrs1, attrs2);

	// get handlers of two files
	FileHandle fileHandle1, fileHandle2;
	if (_rbf_manager->openFile(tableName1, fileHandle1) != 0
			|| _rbf_manager->openFile(tableName2, fileHandle2) != 0){
		return -1;
	}

	// delete related index file
	vector<Attribute> attrs;
	getAttributes(tableName, attrs);
	for (unsigned i = 0; i < attrs.size(); i++) {
		string indexTableName = tableName + "_" + attrs[i].name + ".idx";
		IXFileHandle ixfileHandle;
		if (_index_manager->openFile(indexTableName, ixfileHandle) == 0) {
			_index_manager->closeFile(ixfileHandle);
			_index_manager->destroyFile(indexTableName);

			// delete information in Tables
			void *value = malloc(PAGE_SIZE);
			memset(value, 0 ,PAGE_SIZE);
			int len = indexTableName.length();
			memcpy(value, &len, sizeof(int));
			memcpy((char *)value + 4, indexTableName.c_str(), len);
			CompOp compOp = EQ_OP;
			RBFM_ScanIterator rbfm_ScanIterator;
			vector<string> attributeNames;
			attributeNames.push_back("table-id");
			_rbf_manager->scan(fileHandle1, attrs1, "table-name", compOp, value, attributeNames, rbfm_ScanIterator);
			RID rid;
			void *data = malloc(PAGE_SIZE);
			rbfm_ScanIterator.getNextRecord(rid, data);
			// delete register info in "Tables"
			deleteTuple("Tables", rid);
			free(value);
			free(data);
		}
	}

	void *value = malloc(PAGE_SIZE);
	memset(value, 0 ,PAGE_SIZE);
	int len = tableName.length();
	memcpy(value, &len, sizeof(int));
	memcpy((char *)value + 4, tableName.c_str(), len);
	CompOp compOp = EQ_OP;
	RBFM_ScanIterator rbfm_ScanIterator;
	vector<string> attributeNames;
	attributeNames.push_back("table-id");
	_rbf_manager->scan(fileHandle1, attrs1, "table-name", compOp, value, attributeNames, rbfm_ScanIterator);
	RID rid;
	void *data = malloc(PAGE_SIZE);
	rbfm_ScanIterator.getNextRecord(rid, data);
	// get table_id value
	int idValue = *(int *)((char *)data + 1);
	memset(data, 0, PAGE_SIZE);
	memset(value, 0, PAGE_SIZE);
	memcpy(value, &idValue, 4);
	// delete register info in "Tables"
	deleteTuple("Tables", rid);

	attributeNames.clear();
	attributeNames.push_back("column-name");
	attributeNames.push_back("column-type");
	attributeNames.push_back("column-length");
	RBFM_ScanIterator rbfm_ScanIterator2;
	_rbf_manager->scan(fileHandle2, attrs2, "table-id", compOp, value, attributeNames, rbfm_ScanIterator2);

	while(rbfm_ScanIterator2.getNextRecord(rid, data)  != -1){
		deleteTuple("Columns", rid);
	}

	_rbf_manager->closeFile(fileHandle);
	_rbf_manager->closeFile(fileHandle1);
	_rbf_manager->closeFile(fileHandle2);
	// destroy the table file
	_rbf_manager->destroyFile(tableName);
	free(value);
	free(data);
    return 0;
}

RC RelationManager::getAttributes(const string &tableName, vector<Attribute> &attrs)
{
	string tableName1 = "Tables";
	string tableName2 = "Columns";

	vector<Attribute> attrs1;
	vector<Attribute> attrs2;
	getCatalogAttribute(attrs1, attrs2);

	FileHandle fileHandle1, fileHandle2;
	_rbf_manager->openFile(tableName1, fileHandle1);
	_rbf_manager->openFile(tableName2, fileHandle2);

	CompOp compOp = EQ_OP;
	void *value = malloc(PAGE_SIZE);
	void *pageReader = malloc(PAGE_SIZE);
	memset(value, 0, PAGE_SIZE);
	int len = tableName.length();
	memcpy(value, &len, sizeof(int));
	memcpy((char *)value + 4, tableName.c_str(), len);
	vector<string> attributeNames;
	attributeNames.push_back("table-id");
	RBFM_ScanIterator rbfm_ScanIterator;
	_rbf_manager->scan(fileHandle1, attrs1, "table-name", compOp, value, attributeNames, rbfm_ScanIterator);
	RID rid;
	void *data = malloc(PAGE_SIZE);
	rbfm_ScanIterator.getNextRecord(rid, data);
	int idValue = *(int *)((char *)data + 1);
	memset(data, 0, PAGE_SIZE);
	memset(value, 0, PAGE_SIZE);
	memcpy(value, &idValue, 4);

	attributeNames.clear();
	attributeNames.push_back("column-name");
	attributeNames.push_back("column-type");
	attributeNames.push_back("column-length");
	RBFM_ScanIterator rbfm_ScanIterator2;
	_rbf_manager->scan(fileHandle2, attrs2, "table-id", compOp, value, attributeNames, rbfm_ScanIterator2);
	while (rbfm_ScanIterator2.getNextRecord(rid, data) != -1) {
		Attribute attr;
		string s;
		char p = *(char *)data;
		int offset = 1;
		if ((p & 0x80) != 0x80) {
			int length = *(int *)((char *)data + 1);
			offset += sizeof(int);
			for(int i = 0; i < length; i++) {
				char ch = *((char *)data + offset + i);
				s += ch;
			}
			offset += length;
		}
		attr.name = s;
		if ((p & 0x40) != 0x40) {
			attr.type = (AttrType)(*(int *)((char *)data + offset));
			offset += sizeof(int);
		}
		if ((p & 0x20) != 0x20) {
			attr.length = (AttrLength)(*(int *)((char *)data + offset));
			offset += sizeof(int);
		}
		attrs.push_back(attr);
	}
	_rbf_manager->closeFile(fileHandle1);
	_rbf_manager->closeFile(fileHandle2);
	free(value);
	free(data);
	free(pageReader);
	return 0;
}

RC RelationManager::insertTuple(const string &tableName, const void *data, RID &rid)
{
	FileHandle fileHandle;
	vector<Attribute> descriptor;
	if(instance()->getAttributes(tableName, descriptor) == 0){
		// should search tables table (i.e. catalog) to search the file name related to this table (named tableName)
		if(_rbf_manager->openFile(tableName, fileHandle) == 0){
			RC rc1 = _rbf_manager->insertRecord(fileHandle, descriptor, data, rid);
			RC rc2 = _rbf_manager->closeFile(fileHandle);
			// insertIndex
			for (unsigned i = 0; i < descriptor.size(); i++) {
				string indexTableName = tableName + "_" + descriptor[i].name + ".idx";
				IXFileHandle ixfileHandle;
				// if exists such index file
				if (_index_manager->openFile(indexTableName, ixfileHandle) == 0) {
				    Attribute indexAttr;
				    indexAttr.length = descriptor[i].length;
				    indexAttr.name = descriptor[i].name;
				    indexAttr.type = descriptor[i].type;
				    void *key = malloc(PAGE_SIZE);
				    memset(key, 0, PAGE_SIZE);
				    readKeyFromTuple(descriptor, i, data, key);
				    void *tempKey = malloc(PAGE_SIZE);
					if (indexAttr.type != TypeVarChar) {
						memcpy(tempKey, (char *)key + 1, 4);
						_index_manager->insertEntry(ixfileHandle, indexAttr, tempKey, rid);
					}
					else {
						memcpy(tempKey, (char *)key + 5, *(int *)((char *)key + 1));
						_index_manager->insertEntry(ixfileHandle, indexAttr, tempKey, rid);
					}
					free(tempKey);
					free(key);
					_index_manager->closeFile(ixfileHandle);
				}
			}

			if (rc1 != 0 || rc2 != 0) {
				return -1;
			}
			return 0;
		}
	}
    return -1;
}

RC RelationManager::deleteTuple(const string &tableName, const RID &rid)
{
	FileHandle fileHandle;
	vector<Attribute> descriptor;
	if(instance()->getAttributes(tableName, descriptor) == 0){
		if(_rbf_manager->openFile(tableName, fileHandle) == 0){
			void *data = malloc(PAGE_SIZE);
			_rbf_manager->readRecord(fileHandle, descriptor, rid, data);
			RC rc1 = _rbf_manager->deleteRecord(fileHandle, descriptor, rid);
			RC rc2 = _rbf_manager->closeFile(fileHandle);
			// deleteIndex
			for (unsigned i = 0; i < descriptor.size(); i++) {
				string indexTableName = tableName + "_" + descriptor[i].name + ".idx";
				IXFileHandle ixfileHandle;
				// if exists such index file
				if (_index_manager->openFile(indexTableName, ixfileHandle) == 0) {
				    Attribute indexAttr;
				    indexAttr.length = descriptor[i].length;
				    indexAttr.name = descriptor[i].name;
				    indexAttr.type = descriptor[i].type;
				    void *key = malloc(PAGE_SIZE);
				    memset(key, 0, PAGE_SIZE);
				    readKeyFromTuple(descriptor, i, data, key);
				    void *tempKey = malloc(PAGE_SIZE);
					if (indexAttr.type != TypeVarChar) {
						memcpy(tempKey, (char *)key + 1, 4);
						_index_manager->deleteEntry(ixfileHandle, indexAttr, tempKey, rid);
					}
					else {
						memcpy(tempKey, (char *)key + 5, *(int *)((char *)key + 1));
						_index_manager->deleteEntry(ixfileHandle, indexAttr, tempKey, rid);
					}
					free(tempKey);
					free(key);
					_index_manager->closeFile(ixfileHandle);
				}
			}
			free(data);
			if (rc1 != 0 || rc2 != 0) {
				return -1;
			}
			return 0;
		}
	}
    return -1;
}

RC RelationManager::updateTuple(const string &tableName, const void *data, const RID &rid)
{
	FileHandle fileHandle;
	vector<Attribute> descriptor;
	if(instance()->getAttributes(tableName, descriptor) == 0){
		if(_rbf_manager->openFile(tableName, fileHandle) == 0){
			void *dataForDelete = malloc(PAGE_SIZE);
			_rbf_manager->readRecord(fileHandle, descriptor, rid, dataForDelete);
			RC rc1 = _rbf_manager->updateRecord(fileHandle, descriptor, data, rid);
			RC rc2 = _rbf_manager->closeFile(fileHandle);
			// updateIndex
			for (unsigned i = 0; i < descriptor.size(); i++) {
				string indexTableName = tableName + "_" + descriptor[i].name + ".idx";
				IXFileHandle ixfileHandle;
				// if exists such index file
				if (_index_manager->openFile(indexTableName, ixfileHandle) == 0) {
				    Attribute indexAttr;
				    indexAttr.length = descriptor[i].length;
				    indexAttr.name = descriptor[i].name;
				    indexAttr.type = descriptor[i].type;

				    void *key = malloc(PAGE_SIZE);
				    memset(key, 0, PAGE_SIZE);
				    readKeyFromTuple(descriptor, i, dataForDelete, key);

				    void *tempKey = malloc(PAGE_SIZE);
					if (indexAttr.type != TypeVarChar) {
						memcpy(tempKey, (char *)key + 1, 4);
						_index_manager->deleteEntry(ixfileHandle, indexAttr, tempKey, rid);
					}
					else {
						memcpy(tempKey, (char *)key + 5, *(int *)((char *)key + 1));
						_index_manager->deleteEntry(ixfileHandle, indexAttr, tempKey, rid);
					}
				    memset(key, 0, PAGE_SIZE);
				    memset(tempKey, 0, PAGE_SIZE);
					readKeyFromTuple(descriptor, i, data, key);
					if (indexAttr.type != TypeVarChar) {
						memcpy(tempKey, (char *)key + 1, 4);
						_index_manager->insertEntry(ixfileHandle, indexAttr, tempKey, rid);
					}
					else {
						memcpy(tempKey, (char *)key + 5, *(int *)((char *)key + 1));
						_index_manager->insertEntry(ixfileHandle, indexAttr, tempKey, rid);
					}
					free(tempKey);
					free(key);
					_index_manager->closeFile(ixfileHandle);
				}
			}
			free(dataForDelete);
			if (rc1 != 0 || rc2 != 0) {
				return -1;
			}
			return 0;
		}
	}
    return -1;
}

RC RelationManager::readTuple(const string &tableName, const RID &rid, void *data)
{
	FileHandle fileHandle;
	vector<Attribute> descriptor;
	if(instance()->getAttributes(tableName, descriptor) == 0){
		if(_rbf_manager->openFile(tableName, fileHandle) == 0){
			RC rc1 = _rbf_manager->readRecord(fileHandle, descriptor, rid, data);
			RC rc2 = _rbf_manager->closeFile(fileHandle);
			if (rc1 != 0 || rc2 != 0) {
				return -1;
			}
			return 0;
		}
	}
    return -1;
}

RC RelationManager::printTuple(const vector<Attribute> &attrs, const void *data)
{
	return _rbf_manager->printRecord(attrs, data);
}

RC RelationManager::readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data)
{
	FileHandle fileHandle;
	vector<Attribute> descriptor;
	if(instance()->getAttributes(tableName, descriptor) == 0){
		if(_rbf_manager->openFile(tableName, fileHandle) == 0){
			RC rc1 = _rbf_manager->readAttribute(fileHandle, descriptor, rid, attributeName, data);
			RC rc2 = _rbf_manager->closeFile(fileHandle);
			if (rc1 != 0 || rc2 != 0) {
				return -1;
			}
			return 0;
		}
	}
	return -1;
}

RC RelationManager::scan(const string &tableName,
      const string &conditionAttribute,
      const CompOp compOp,
      const void *value,
      const vector<string> &attributeNames,
      RM_ScanIterator &rm_ScanIterator)
{
	FileHandle fileHandle;
	vector<Attribute> descriptor;
	if (tableName == "Tables" || tableName == "Columns") {
		vector<Attribute> alternate;
		if (tableName == "Tables") {
			getCatalogAttribute(descriptor, alternate);
		} else if (tableName == "Columns") {
			getCatalogAttribute(alternate, descriptor);
		}
	}
	else if (tableName == "cli_tables" || tableName == "cli_columns") {
		vector<Attribute> alternate;
		if (tableName == "cli_tables") {
			getCliCatalogAttribute(descriptor, alternate);
		} else if (tableName == "cli_columns") {
			getCliCatalogAttribute(alternate, descriptor);
		}
	} 
	else {
		getAttributes(tableName, descriptor);
	}

	if (_rbf_manager->openFile(tableName, fileHandle) == 0) {
		if(_rbf_manager->scan(fileHandle, descriptor, conditionAttribute, compOp, value,
									   attributeNames, rm_ScanIterator.rbfm_ScanIterator) == 0) {
			return 0;
		}
	}
	return -1;
}

// !!!!!!!!!!index part!!!!!!!!!!!!!
RC RelationManager::createIndex(const string &tableName, const string &attributeName) {
	string indexTableName = tableName + "_" + attributeName + ".idx";
	vector<Attribute> descriptor;
	// there's a check (if 'tableName' table exists) in the beginning of 'getAttributes'
	if (getAttributes(tableName, descriptor) == 0) {
		int returnValue = -1;
		int attrPos = -1;
    // check if attributeName exists in tableName
		for (unsigned i = 0; i < descriptor.size(); i++) {
			if (descriptor[i].name == attributeName) {
				attrPos = i;
				returnValue = 0;
			}
		}
		if (returnValue == -1) {
			return -1;
		}

    // (table-id, table-name, file-name) in 'Tables' table
		string indexFileName = indexTableName;
		RID rid;
		// register the index info in the Tables table
		string tableName1 = "Tables";
		vector<Attribute> attrs1;
		vector<Attribute> attrs2;
		getCatalogAttribute(attrs1, attrs2);
		FileHandle fileHandle1;
		_rbf_manager->openFile(tableName1, fileHandle1);
		id = fileHandle1.id;
		id ++;
		void *record = malloc(PAGE_SIZE);
		tablesRecordGenerator(id, indexTableName, indexFileName, record);
		_rbf_manager->insertRecord(fileHandle1, attrs1, record, rid);
		memset(record, 0, PAGE_SIZE);
		free(record);
		fileHandle1.id = id;
		_rbf_manager->closeFile(fileHandle1);

		// create index file
		returnValue = _index_manager->createFile(indexTableName);
		if (returnValue == -1) {
			return returnValue;
		}
		IXFileHandle ixfileHandle;
		returnValue = _index_manager->openFile(indexTableName, ixfileHandle);
		if (returnValue == -1) {
			return returnValue;
		}

		RM_ScanIterator rmsi;
		// indexing attribute
		Attribute keyAttribute = descriptor[attrPos];
		vector<string> attributeNames;
		attributeNames.push_back(keyAttribute.name);
		returnValue = scan(tableName, attributeName, NO_OP, NULL, attributeNames, rmsi);

		char *data = (char *) malloc(PAGE_SIZE);
		memset(data, 0, PAGE_SIZE);
		while (rmsi.getNextTuple(rid, data) != RM_EOF) {
			if (keyAttribute.type != TypeVarChar) {
				void *key = malloc(4);
				memcpy(key, (char *)data + 1, 4);
				_index_manager->insertEntry(ixfileHandle, keyAttribute, key, rid);
				free(key);
			}
			else {
				void *key = malloc(PAGE_SIZE);
				memcpy(key, (char *)data + 5, *(int *)((char *)data + 1));
				_index_manager->insertEntry(ixfileHandle, keyAttribute, key, rid);
				free(key);
			}
		}
		free(data);
		rmsi.close();
		returnValue = _index_manager->closeFile(ixfileHandle);
		if (returnValue == -1) {
			return returnValue;
		}
	}
	else {
		return -1;
	}
	return 0;
}

RC RelationManager::destroyIndex(const string &tableName, const string &attributeName) {
	string indexTableName = tableName + "_" + attributeName + ".idx";

	// delete registration in 'Tables' table
	string tableName1 = "Tables";
	vector<Attribute> attrs1;
	vector<Attribute> attrs2;
	getCatalogAttribute(attrs1, attrs2);
	FileHandle fileHandle1;
	vector<string> attributeNames;
	attributeNames.push_back("table-id");
	RBFM_ScanIterator rbfsi;
	_rbf_manager->openFile(tableName1, fileHandle1);
	void *data = malloc(PAGE_SIZE);
	int length = sizeof(indexTableName);
	memcpy(data, &length, 4);
	memcpy((char *)data + 4, indexTableName.c_str(), length);
	_rbf_manager->scan(fileHandle1, attrs1, "table-name", EQ_OP, data, attributeNames, rbfsi);
	RID rid;
	void *value = malloc(PAGE_SIZE);
	rbfsi.getNextRecord(rid, value);
  // delete register info in "Tables"
	deleteTuple(tableName1, rid);
	memset(value, 0, PAGE_SIZE);
	memset(data, 0, PAGE_SIZE);
	_rbf_manager->closeFile(fileHandle1);
	_index_manager->destroyFile(indexTableName);
	free(data);
	free(value);
	return 0;
}

RC RelationManager::indexScan(const string &tableName,
			const string &attributeName,
			const void *lowKey,
			const void *highKey,
			bool lowKeyInclusive,
			bool highKeyInclusive,
			RM_IndexScanIterator &rm_IndexScanIterator
) {
	IXFileHandle ixfileHandle;
	vector<Attribute> descriptor;
	string indexTableName = tableName + "_" + attributeName + ".idx";

	if(getAttributes(tableName, descriptor) == 0) {
		// check if attributeName exists in tableName
		int attrPos = -1;
		for (unsigned i = 0; i < descriptor.size(); i++) {
			if (descriptor[i].name == attributeName) {
				attrPos = i;
			}
		}
		//    RC scan(IXFileHandle &ixfileHandle,
		//            const Attribute &attribute,
		//            const void *lowKey,
		//            const void *highKey,
		//            bool lowKeyInclusive,
		//            bool highKeyInclusive,
		//            IX_ScanIterator &ix_ScanIterator);
		if (_index_manager->openFile(indexTableName, ixfileHandle) == 0) {
			if (_index_manager->scan(ixfileHandle, descriptor[attrPos], lowKey, highKey,
					lowKeyInclusive, highKeyInclusive, rm_IndexScanIterator.ix_ScanIterator) == 0) {
				return 0;
			}
		}
	}
	return -1;
}

RC RelationManager::readKeyFromTuple(const vector<Attribute> &attrs, int attrCount, const void *data, void *key) {
	// int
	if (attrs[attrCount].type == TypeInt) {
		bool nullFlag = isIthAttributeNull(attrCount, (void *)data);
		// don't consider null key
		if (!nullFlag) {
			int offset = ceil(attrs.size() / 8);
			for (int i = 0; i < (int)attrs.size(); i++) {
				if (i == attrCount) {
					memcpy(key, (char *)data + offset, 4);
					return 0;
				}
				if (attrs[i].type == TypeInt) {
					offset += 4;
				}
				else if (attrs[i].type == TypeReal) {
					offset += 4;
				}
				else if (attrs[i].type == TypeVarChar) {
					int length = *(int *)((char *)data + offset);
					offset += 4;
					offset += length;
				}
			}
		}
	}
	// real
	else if (attrs[attrCount].type == TypeReal) {
		bool nullFlag = isIthAttributeNull(attrCount, (void *)data);
		// don't consider null key
		if (!nullFlag) {
			int offset = ceil(attrs.size() / 8);
			for (int i = 0; i < (int)attrs.size(); i++) {
				if (i == attrCount) {
					memcpy(key, (char *)data + offset, 4);
					return 0;
				}
				if (attrs[i].type == TypeInt) {
					offset += 4;
				}
				else if (attrs[i].type == TypeReal) {
					offset += 4;
				}
				else if (attrs[i].type == TypeVarChar) {
					int length = *(int *)((char *)data + offset);
					offset += 4;
					offset += length;
				}
			}
		}
	}
	// varchar
	else {
		bool nullFlag = isIthAttributeNull(attrCount, (void *)data);
		// don't consider null key
		if (!nullFlag) {
			int offset = ceil(attrs.size() / 8);
			for (int i = 0; i < (int)attrs.size(); i++) {
				if (i == attrCount) {
					int l = *(int *)((char *)data + offset);
					memcpy(key, &l, 4);
					memcpy((char *)key + 4, (char *)data + offset, l);
					return 0;
				}
				if (attrs[i].type == TypeInt) {
					offset += 4;
				}
				else if (attrs[i].type == TypeReal) {
					offset += 4;
				}
				else if (attrs[i].type == TypeVarChar) {
					int length = *(int *)((char *)data + offset);
					offset += 4;
					offset += length;
				}
			}
		}
	}
	return -1;
}

bool RelationManager::isIthAttributeNull(int i, void *data) {
	int offset = 0;
	int add = i / 8;
	int mod = i % 8;
	offset += add;// which 8 bit it shifts
	char *cont = (char *)data + offset;
	char p = *cont;
	if ((mod == 0) && ((p & 0x80) == 0x80)) {
		return true;
	}
	if ((mod == 1) && ((p & 0x40) == 0x40)) {
		return true;
	}
	if ((mod == 2) && ((p & 0x20) == 0x20)) {
		return true;
	}
	if ((mod == 3) && ((p & 0x10) == 0x10)) {
		return true;
	}
	if ((mod == 4) && ((p & 0x08) == 0x08)) {
		return true;
	}
	if ((mod == 5) && ((p & 0x04) == 0x04)) {
		return true;
	}
	if ((mod == 6) && ((p & 0x02) == 0x02)) {
		return true;
	}
	if ((mod == 7) && ((p & 0x01) == 0x01)) {
		return true;
	}
	return false;
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// Extra credit work
RC RelationManager::dropAttribute(const string &tableName, const string &attributeName)
{
    return -1;
}

// Extra credit work
RC RelationManager::addAttribute(const string &tableName, const Attribute &attr)
{
    return -1;
}
