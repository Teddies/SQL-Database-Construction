#include "rbfm.h"

RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;

RecordBasedFileManager* RecordBasedFileManager::instance()
{
    if(!_rbf_manager)
        _rbf_manager = new RecordBasedFileManager();

    return _rbf_manager;
}

void RecordBasedFileManager::destroyInstance()
{
	if(_rbf_manager != NULL) {
        delete _rbf_manager;
		_rbf_manager = NULL;
	}
}

RecordBasedFileManager::RecordBasedFileManager()
{
}

RecordBasedFileManager::~RecordBasedFileManager()
{
}

RC RecordBasedFileManager::createFile(const string &fileName) {
    return PagedFileManager::createFile(fileName);
}

RC RecordBasedFileManager::destroyFile(const string &fileName) {
    return PagedFileManager::destroyFile(fileName);
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
    return PagedFileManager::openFile(fileName, fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
    return PagedFileManager::closeFile(fileHandle);
}

RC RecordBasedFileManager::recordConverter(const vector<Attribute> &recordDescriptor, const void *data, void *record, short int &recordSize) {
	vector<unsigned int> nullsIndicator; 									// Record the indexes which bits are null
	unsigned int actualByte = ceil((double) recordDescriptor.size() / 8);
	int ofs = 2 * recordDescriptor.size(); 									// Offset of void *record
	short int attrPointer;													// The pointer number at the beginning of each record,																	// which is the first bit position of each attribute
	unsigned int i;
	char *cont;
	unsigned int offset = 0, index = 0;
	int varCharLength;
	// Record the indexes of the nulls indicator
	for (i = 0; i < actualByte; i++) {
		cont = (char *)data + offset;
		char p = *cont;
		if ((p & 0x80) == 0x80)
			nullsIndicator.push_back(offset * 8);
		if ((p & 0x40) == 0x40)
			nullsIndicator.push_back(offset * 8 + 1);
		if ((p & 0x20) == 0x20)
			nullsIndicator.push_back(offset * 8 + 2);
		if ((p & 0x10) == 0x10)
			nullsIndicator.push_back(offset * 8 + 3);
		if ((p & 0x08) == 0x08)
			nullsIndicator.push_back(offset * 8 + 4);
		if ((p & 0x04) == 0x04)
			nullsIndicator.push_back(offset * 8 + 5);
		if ((p & 0x02) == 0x02)
			nullsIndicator.push_back(offset * 8 + 6);
		if ((p & 0x01) == 0x01)
			nullsIndicator.push_back(offset * 8 + 7);
		offset += 1;
	}
	// Travel all the attributes to set the corresponding attribute to appropriate places with a new record representation
	for (i = 0; i < recordDescriptor.size(); i++) {
		if (!nullsIndicator.empty() && index < nullsIndicator.size() && i == nullsIndicator[index]) {
			index ++;
			attrPointer = -1;
			memcpy((char *)record + 2 * i, &attrPointer, sizeof(short int)); // For the NULL attribute, the pointer value is -1
		}
		else {
			if (recordDescriptor[i].type == 2) {
				string s = "";
				cont = (char *)data + offset;
				varCharLength = *(int *) cont;
				offset += sizeof(int);
				for (int j = 0; j < varCharLength; j++) {
					cont = (char *)data + offset;
				    offset += sizeof(char);
					char ch = *cont;
					s += ch;
				}
				memcpy((char *)record + ofs, s.c_str(), varCharLength);
				attrPointer = ofs;
				ofs += varCharLength;
			}
			else if (recordDescriptor[i].type == 1) {
				cont = (char *)data + offset;
				offset += sizeof(float);
				float f = *(float *) cont;
				memcpy((char *)record + ofs, &f, sizeof(float));
				attrPointer = ofs;
				ofs += sizeof(float);
			}
			else {
				cont = (char *)data + offset;
				offset += sizeof(int);
				int n = *(int *) cont;
				memcpy((char *)record + ofs, &n, sizeof(int));
				attrPointer = ofs;
				ofs += sizeof(int);
			}
			memcpy((char *)record + 2 * i, &attrPointer, sizeof(short int));
		}
	}
	recordSize = (short int) ofs;
	return 0;
}

RC RecordBasedFileManager::recordWrapper(const vector<Attribute> &recordDescriptor, const void *record, void *data, const short int recordSize, int &newSize) {
	int nullFieldsIndicatorActualSize = ceil((double)recordDescriptor.size() / 8);
	int byteSize = 0; 														// Iterator for every time putting 8 bits into nullsIndicator
	int offset = nullFieldsIndicatorActualSize;
	unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
	short int pointer1, pointer2;
	unsigned int i, j;
	bitset<8> bit(0);
	// Prepare for the construction of nulls indicator
	if (recordDescriptor.size() == 1) {
		short int pointer = *(short int*)record;
		if (pointer == -1) {
			bit[7] = 1;
			int n = bit.to_ulong();
			memset(nullsIndicator, n, 1);
			memcpy(data, nullsIndicator, nullFieldsIndicatorActualSize);
			newSize = 2;
			return 0;
		}
		int varLength = recordSize - pointer;
		if (recordDescriptor[0].type == 2) {
			memcpy((char *)data + offset, &varLength, sizeof(int));
			offset += sizeof(int);
		}
		memcpy((char *)data + offset, (char *)record + pointer, varLength);
		offset += varLength;
		newSize = offset;
		memset(nullsIndicator, 0, 1);
		memcpy(data, nullsIndicator, nullFieldsIndicatorActualSize);
		return 0;
	}
	for (i = 0; i < recordDescriptor.size() - 1; i++) {
		j = i; 																// Record the non-null attribute index
		if (i != 0 && i % 8 == 0) {
			int n = bit.to_ulong();
			memset(nullsIndicator + byteSize, n, 1);
			bit.reset();
			byteSize++;
		}
		pointer1 = *(short int*)((char *)record + 2 * i);
		pointer2 = *(short int *)((char *)record + 2 * i + 2);				// The value differences between two adjacent pointers
																			// indicates the length of the record
		if (pointer1 == -1) {
			bit[7 - i % 8] = 1;												// Endianness of an integer`s binary format is from right to left
		}
		else {
			while (pointer2 == -1 && i < recordDescriptor.size() - 1) {
				i++;
				if (i % 8 == 0) {
					int n = bit.to_ulong();
					memset(nullsIndicator + byteSize, n, 1);
					bit.reset();
					byteSize++;
				}
				bit[7 - i % 8] = 1;
				pointer2 = *(short int *)((char *)record + 2 * i + 2);
			}
			if (i == recordDescriptor.size() -1) {
				pointer2 = *(short int *)((char *)record + 2 * i);
				break;
			}
			if (recordDescriptor[j].type == 2) {
				int varCharLength = pointer2 - pointer1;
				memcpy((char *)data + offset, &varCharLength, sizeof(int));
				offset += sizeof(int);
				memcpy((char *)data + offset, (char *)record + pointer1, varCharLength);
				offset += varCharLength;
			}
			else {
				int varLength = pointer2 - pointer1;
				memcpy((char *)data + offset, (char *)record + pointer1, varLength);
				offset += varLength;
			}
		}
	}
	// The pointer travels to the end of the record
	int varLength;
	if (pointer2 != -1) {
		varLength = recordSize - pointer2;
		if (recordDescriptor[i].type == 2) {
			memcpy((char *)data + offset, &varLength, sizeof(int));
			offset += sizeof(int);
		}
		memcpy((char *)data + offset, (char *)record + pointer2, varLength);
		offset += varLength;
	}
	else if (pointer1 != -1){
		varLength = recordSize - pointer1;
		if (recordDescriptor[j].type == 2) {
			memcpy((char *)data + offset, &varLength, sizeof(int));
			offset += sizeof(int);
		}
		memcpy((char *)data + offset, (char *)record + pointer1, varLength);
		offset += varLength;
	}
	else {
		bit[7 - i % 8] = 1;
	}
	if (byteSize < nullFieldsIndicatorActualSize) {
		int n = bit.to_ulong();
		memset(nullsIndicator + byteSize, n, 1);
	}
	newSize = offset;
	memcpy(data, nullsIndicator, nullFieldsIndicatorActualSize);
	free(nullsIndicator);
	return 0;
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
	// First set the new bit representation of the record in the page
	void *record = malloc(PAGE_SIZE);
	short int recordSize;
	short int actualSize;
	int i, j;
	recordConverter(recordDescriptor, data, record, recordSize);
	// Next, find the appropriate page place to insert this newly represented record
	void *memory = malloc(PAGE_SIZE);										// Update the record-inserted page
	void *pageReader = malloc(4); 											// Read the slot information
	short int flag, numberOfSlot, head, length;
	for (i = fileHandle.getNumberOfPages(); i > 0; i--) {
		memset(memory, 0, PAGE_SIZE);
		fileHandle.readPage(i - 1, memory);
		memcpy(pageReader, (char *)memory + 4092, 4);
		numberOfSlot = *(short int *) pageReader;
		flag = *(short int *)((char *)pageReader + 2);

		int deletedFlag = -1;
		short int maxOffset = -3;
		// Find if there is deleted slot can be reused
		if (flag >= ((recordSize > 8) ? recordSize : 8)) {
			for (j = 0; j < numberOfSlot; j ++) {
				memcpy(pageReader, (char *)memory + PAGE_SIZE - 4 * (j + 2), 4);
				head = *(short int *) pageReader;
				length = *(short int *)((char *)pageReader + 2);
				// Find max offset record
				//((head > 8) ? head : 8)
				if (((length > 8) ? length : 8) + head > maxOffset) {
					maxOffset = ((length > 8) ? length : 8) + head;
				}
				// Find matched deleted slot
				if (head == -1 && length == -1 && deletedFlag == -1) {
					deletedFlag = j;
				}
			}
			// Reuse deleted slot
			if (deletedFlag != -1) {
				flag = flag - ((recordSize > 8) ? recordSize : 8);
				// Update the flag value and the number of slots
				memcpy((char *)pageReader, &flag, sizeof(short int));
				memcpy((char *)memory + 4094, pageReader, 2);

				head = maxOffset;
				memcpy((char *)pageReader, &head, sizeof(short int));
				actualSize = ((recordSize > 8) ? recordSize : 8);
				memcpy((char *)pageReader + 2, &actualSize, sizeof(short int));
				memcpy((char *)memory + PAGE_SIZE - 4 * (deletedFlag + 2), pageReader, 4);

				// Insert new record in the old page
				memcpy((char *)memory + head, record, ((recordSize > 8) ? recordSize : 8));
				fileHandle.writePage(i - 1, memory);

				// Update the RID of this record
				rid.pageNum = (unsigned) i - 1;
				rid.slotNum = (unsigned) deletedFlag;
				break;
			}
		}

		// Use new slot to insert record
		if (flag >= ((recordSize > 8) ? recordSize : 8) + 4) {
			flag = flag - ((recordSize > 8) ? recordSize : 8) - 4;
			numberOfSlot++;

			// Update the flag value and the number of slots
			memcpy((char *)pageReader, &numberOfSlot, sizeof(short int));
			memcpy((char *)pageReader + 2, &flag, sizeof(short int));
			memcpy((char *)memory + 4092, pageReader, 4);

			head = maxOffset;
			memcpy((char *)pageReader, &head, sizeof(short int));
			actualSize = ((recordSize > 8) ? recordSize : 8);
			memcpy((char *)pageReader + 2, &actualSize, sizeof(short int));
			memcpy((char *)memory + PAGE_SIZE - 4 * (numberOfSlot + 1), pageReader, 4);

			// Insert the record in the old page
			memcpy((char *)memory + head, record, ((recordSize > 8) ? recordSize : 8));
			fileHandle.writePage(i - 1, memory);

			// Update the RID of this record
			rid.pageNum = (unsigned) i - 1;
			rid.slotNum = (unsigned) numberOfSlot - 1;
			break;
		}
	}
	// If there is no enough spaces in the existed pages, insert this record into a new page
	if (i == 0) {
		i = fileHandle.getNumberOfPages() + 1;
		memset(memory, 0, PAGE_SIZE);
		flag = 4088 - ((recordSize > 8) ? recordSize : 8);
		numberOfSlot = 1;
		memcpy((char *)memory, record, ((recordSize > 8) ? recordSize : 8));
		head = 0;
		memcpy((char *)memory + 4088, &head, sizeof(short int));
		actualSize = ((recordSize > 8) ? recordSize : 8);
		memcpy((char *)memory + 4090, &actualSize, sizeof(short int));
		memcpy((char *)memory + 4092, &numberOfSlot, sizeof(short int));
		memcpy((char *)memory + 4094, &flag, sizeof(short int));
		fileHandle.appendPage(memory);
		// Update the RID of this record
		rid.pageNum = (unsigned) i - 1;
		rid.slotNum = (unsigned) numberOfSlot - 1;
	}
	free(memory);
	free(record);
	free(pageReader);
    return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
	// First, locate the record according to the provided RID
	void *memory = malloc(PAGE_SIZE);
	void *pageReader = malloc(PAGE_SIZE);
	RC rc = fileHandle.readPage(rid.pageNum, memory);

	// Check the invalid input RID
	if (rc != 0) {
		return -1;
	}
	memcpy(pageReader, (char *)memory + PAGE_SIZE - 4, 2);
	if (rid.slotNum >= (unsigned)*(short int *)pageReader) {
		return -1;
	}

	memcpy(pageReader, (char *)memory + PAGE_SIZE - (rid.slotNum + 2) * 4, 4);
	short int head = *(short int *)pageReader;
	short int length = *(short int *)((char *)pageReader + 2);
	// Check whether this record has been deleted
	if (head == -1) {
		return -1;
	}

	int pageNum = (int)rid.pageNum;
	short int slotNum = (short int)rid.slotNum;

	// Judge whether the corresponding record is actually a pointer pointing to another page
	short int fl = *(short int *)((char *)memory + head);
	while (fl == -2) {
		pageNum = *(int *)((char *)memory + head + 2);
		slotNum = *(short int *)((char *)memory + head + 6);
		fileHandle.readPage(pageNum, memory);
		memcpy(pageReader, (char *)memory + PAGE_SIZE - (slotNum + 2) * 4, 4);
		head = *(short int *)pageReader;
		if (head == -1) {
			return -1;
		}
		length = *(short int *)((char *)pageReader + 2);
		fl = *(short int *)((char *)memory + head);
	}
	// Read the corresponding record into the pageReader
	memset(pageReader, 0, PAGE_SIZE);
	memcpy(pageReader, (char *)memory + head, length);

	// Next, wrap the record into the required format
	int newSize = 0;
	recordWrapper(recordDescriptor, pageReader, data, length, newSize);
	free(memory);
	free(pageReader);
    return 0;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {
	char *cont;
	vector<unsigned int> nullsIndicator; 									// Record the indexes which bits are null
	int actualByte = ceil((double) recordDescriptor.size() / 8);
	unsigned int offset = 0, index = 0;
	int varCharLength;
	// Deal with the nulls indicator
	for (int i = 0; i < actualByte; i++) {
		cont = (char *)data + offset;
		char p = *cont;
		if ((p & 0x80) == 0x80)
			nullsIndicator.push_back(offset * 8);
		if ((p & 0x40) == 0x40)
			nullsIndicator.push_back(offset * 8 + 1);
		if ((p & 0x20) == 0x20)
			nullsIndicator.push_back(offset * 8 + 2);
		if ((p & 0x10) == 0x10)
			nullsIndicator.push_back(offset * 8 + 3);
		if ((p & 0x08) == 0x08)
			nullsIndicator.push_back(offset * 8 + 4);
		if ((p & 0x04) == 0x04)
			nullsIndicator.push_back(offset * 8 + 5);
		if ((p & 0x02) == 0x02)
			nullsIndicator.push_back(offset * 8 + 6);
		if ((p & 0x01) == 0x01)
			nullsIndicator.push_back(offset * 8 + 7);
		offset += 1;
	}

	for (unsigned int i = 0; i < recordDescriptor.size(); i++) {
		if (!nullsIndicator.empty() && index < nullsIndicator.size() && i == nullsIndicator[index]) {
			index ++;
			cout << recordDescriptor[i].name << ": NULL";
		}
		else {
			if (recordDescriptor[i].type == 2) {
				cont = (char *)data + offset;
				varCharLength = *(int *) cont;
				offset += sizeof(int);
				cout << recordDescriptor[i].name << ": ";
				for (int j= 0; j < varCharLength; j++) {
					cont = (char *)data + offset;
					offset += sizeof(char);
					cout << *cont;
				}
			}
			else if (recordDescriptor[i].type == 1) {
				cont = (char *)data + offset;
				offset += sizeof(float);
				cout << recordDescriptor[i].name << ": " << *(float *) cont;
			}
			else {
				cont = (char *)data + offset;
				offset += sizeof(int);
				cout << recordDescriptor[i].name << ": " << *(int *) cont;
			}
		}
		cout << "    ";
	}
	cout << endl;
	return 0;
}

RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid) {
	void *memory = malloc(PAGE_SIZE);
	void *pageReader = malloc(PAGE_SIZE);
	RC rc = fileHandle.readPage(rid.pageNum, memory);

	// Check the invalid input RID
	if (rc != 0) {
		return -1;
	}
	memcpy(pageReader, (char *)memory + PAGE_SIZE - 4, 4);
	short int numberOfSlot = *(short int *)pageReader;
	if (rid.slotNum >= (unsigned)numberOfSlot) {
		return -1;
	}
	short int flag = *(short int *)((char *)pageReader + 2);

	memcpy(pageReader, (char *)memory + PAGE_SIZE - (rid.slotNum + 2) * 4, 4);
	short int head = *(short int *)pageReader;
	// Check whether this record has been deleted
	if (head == -1) {
		return -1;
	}
	short int length = *(short int *)((char *)pageReader + 2);
	short int invalidHead = -1;
	short int invalidLength = -1;
	int pageNum = (int)rid.pageNum;
	short int slotNum = (short int)rid.slotNum;

	// Judge whether the corresponding record is actually a pointer pointing to another page
	short int fl = *(short int *)((char *)memory + head);
	while (fl == -2) {
		// Delete the pointer and update the slot in the current page
		flag += 8;
		memcpy((char *)memory + PAGE_SIZE - 2, &flag, 2);
		short int maxHead, maxLength, firstOffset;
		short int maxOffset = -3;
		for (short int j = 0; j < numberOfSlot; j ++) {
			memcpy(pageReader, (char *)memory + PAGE_SIZE - 4 * (j + 2), 4);
			maxHead = *(short int *) pageReader;
			maxLength = *(short int *)((char *)pageReader + 2);
			// Find max offset record
			if (maxHead + ((maxLength > 8) ? maxLength : 8) > maxOffset) {
				maxOffset = maxHead + ((maxLength > 8) ? maxLength : 8);
			}
		}
		firstOffset = head + 8;
		int pageID = pageNum;
		int slotID = slotNum;
		pageNum = *(int *)((char *)memory + head + 2);
		slotNum = *(short int *)((char *)memory + head + 6);
		memset(pageReader, 0, PAGE_SIZE);
		// The last one is the deleted one
		if (firstOffset == maxOffset){
			memset((char *)memory + head, 0, 8);
		}
		else {
			memcpy(pageReader, (char *)memory + head + 8, maxOffset - firstOffset);
			// Move the latter records forward
			memcpy((char *)memory + head, pageReader, maxOffset - firstOffset);
			memset((char *)memory + head + maxOffset - firstOffset, 0, 8);
			// Update the slot information of the latter records
			memset(pageReader, 0, PAGE_SIZE);
			for (short int j = 0; j < numberOfSlot; j ++) {
				memcpy(pageReader, (char *)memory + PAGE_SIZE - 4 * (j + 2), 2);
				if (*(short int *)pageReader != -1 && *(short int *)pageReader > head) {
					short int newHead = *(short int *)pageReader - 8;
					memcpy((char *)memory + PAGE_SIZE - 4 * (j + 2), &newHead, 2);
				}
			}
		}
		// Update the pointer slot value
		short int invalid = -1;
		memcpy((char *)memory + PAGE_SIZE - (slotID + 2) * 4, &invalid, 2);
		memcpy((char *)memory + PAGE_SIZE - (slotID + 2) * 4 + 2, &invalid, 2);
		fileHandle.writePage(pageID, memory);

		// Read the pointed page
		fileHandle.readPage(pageNum, memory);
		memcpy(pageReader, (char *)memory + PAGE_SIZE - 4, 4);
		numberOfSlot = *(short int *)pageReader;
		flag = *(short int *)((char *)pageReader + 2);
		memcpy(pageReader, (char *)memory + PAGE_SIZE - (slotNum + 2) * 4, 4);
		head = *(short int *)pageReader;
		if (head == -1) {
			return -1;
		}
		length = *(short int *)((char *)pageReader + 2);
		fl = *(short int *)((char *)memory + head);
	}
	memset(pageReader, 0, PAGE_SIZE);

	// Copy the rest records behind the deleted record into pageReader
	// Go through the slot table to find the max offset of record (i.e. find the end of record part)
	short int maxHead, maxLength, firstOffset;
	short int maxOffset = -3;
	for (short int j = 0; j < numberOfSlot; j ++) {
		memcpy(pageReader, (char *)memory + PAGE_SIZE - 4 * (j + 2), 4);
		maxHead = *(short int *) pageReader;
		maxLength = *(short int *)((char *)pageReader + 2);
		// Find max offset record
		if (maxHead + ((maxLength > 8) ? maxLength : 8) > maxOffset) {
			maxOffset = maxHead + ((maxLength > 8) ? maxLength : 8);
		}
	}
	firstOffset = head + ((length > 8) ? length : 8);

	memset(pageReader, 0, PAGE_SIZE);
	// The last one is the deleted one
	if (firstOffset == maxOffset){
		memset((char *)memory + head, 0, length);
	}
	else {
		// ((length > 8) ? length : 8)
		memcpy(pageReader, (char *)memory + head + ((length > 8) ? length : 8), maxOffset - firstOffset);
		// Move the latter records forward
		memcpy((char *)memory + head, pageReader, maxOffset - firstOffset);
		memset((char *)memory + head + maxOffset - firstOffset, 0, ((length > 8) ? length : 8));
		// Update the slot information of the latter records
		memset(pageReader, 0, PAGE_SIZE);
		for (short int j = 0; j < numberOfSlot; j ++) {
			memcpy(pageReader, (char *)memory + PAGE_SIZE - 4 * (j + 2), 2);
			if (*(short int *)pageReader != -1 && *(short int *)pageReader > head) {
				short int newHead = *(short int *)pageReader - ((length > 8) ? length : 8);
				memcpy((char *)memory + PAGE_SIZE - 4 * (j + 2), &newHead, 2);
			}
		}
	}

	// Set the original slot data invalid
	memcpy((char *)memory + PAGE_SIZE - (slotNum + 2) * 4, &invalidHead, sizeof(short int));
	memcpy((char *)memory + PAGE_SIZE - (slotNum + 2) * 4 + 2, &invalidLength, sizeof(short int));

	// Update the flag number
	flag += ((length > 8) ? length : 8);
	memcpy((char *)memory + PAGE_SIZE - 2, &flag, 2);

	fileHandle.writePage(pageNum, memory);
	free(memory);
	free(pageReader);
	return 0;
}

RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid){
	// First, convert the record representation
	void *record = malloc(PAGE_SIZE);
	short int recordSize;
	recordConverter(recordDescriptor, data, record, recordSize);

	// Next, locate the record
	void *memory = malloc(PAGE_SIZE);
	void *pageReader = malloc(PAGE_SIZE);
	RC rc = fileHandle.readPage(rid.pageNum, memory);
	// Check the invalid input RID
	if (rc != 0) {
		return -1;
	}
	memcpy(pageReader, (char *)memory + PAGE_SIZE - 4, 4);
	int numberOfSlot = *(short int *)pageReader;
	if (rid.slotNum >= (unsigned)numberOfSlot) {
		return -1;
	}
	short int flag = *(short int *)((char *)pageReader + 2);
	memcpy(pageReader, (char *)memory + PAGE_SIZE - (rid.slotNum + 2) * 4, 4);
	short int head = *(short int *)pageReader;
	// Check whether this record has been deleted
	if (head == -1) {
		return -1;
	}
	short int length = *(short int *)((char *)pageReader + 2);
	int pageNum = (int)rid.pageNum;
	short int slotNum = (short int)rid.slotNum;

	// Judge whether the corresponding record is actually a pointer pointing to another page
	short int fl = *(short int *)((char *)memory + head);
	while (fl == -2) {
		pageNum = *(int *)((char *)memory + head + 2);
		slotNum = *(short int *)((char *)memory + head + 6);
		fileHandle.readPage(pageNum, memory);
		memcpy(pageReader, (char *)memory + PAGE_SIZE - 4, 4);
		numberOfSlot = *(short int *)pageReader;
		flag = *(short int *)((char *)pageReader + 2);
		memcpy(pageReader, (char *)memory + PAGE_SIZE - (slotNum + 2) * 4, 4);
		head = *(short int *)pageReader;
		if (head == -1) {
			return -1;
		}
		length = *(short int *)((char *)pageReader + 2);
		fl = *(short int *)((char *)memory + head);
	}

	// Then, compare the length of the original record size with the updated one
	if (length == recordSize || (length < 8 && recordSize < 8)) {
		memcpy((char *)memory + head, record, length);
	}
	else if (recordSize < length) {//smaller
		short int diff = length - ((recordSize > 8) ? recordSize : 8);
		memcpy((char *)memory + head, record, recordSize);
		if (recordSize < 8) {
			memset((char *) memory + head + recordSize, 0, 8 - recordSize);
		}
		// Copy the rest records behind the deleted record into pageReader
		memset(pageReader, 0, PAGE_SIZE);

		// Go through the slot table to find the max offset of record (i.e. find the end of record part)
		short int maxHead, maxLength, firstOffset;
		short int maxOffset = -3;
		for (short int j = 0; j < numberOfSlot; j ++) {
			memcpy(pageReader, (char *)memory + PAGE_SIZE - 4 * (j + 2), 4);
			maxHead = *(short int *) pageReader;
			maxLength = *(short int *)((char *)pageReader + 2);
			// Find max offset record
			if (maxHead + ((maxLength > 8) ? maxLength : 8) > maxOffset) {
				maxOffset = maxHead + ((maxLength > 8) ? maxLength : 8);
			}
		}
		//length must bigger than 8 in this situation
		firstOffset = head + length;

		memset(pageReader, 0, PAGE_SIZE);
		// The last one is the updated one
		if (firstOffset == maxOffset){
			memset((char *)memory + head + ((recordSize > 8) ? recordSize : 8), 0, diff);
			if (recordSize < 8) {
				memset((char *) memory + head + recordSize, 0, 8 - recordSize);
			}
		}
		else {
			memcpy(pageReader, (char *)memory + firstOffset, maxOffset - firstOffset);
			// Move the latter records forward
			memcpy((char *)memory + head + ((recordSize > 8) ? recordSize : 8), pageReader, maxOffset - firstOffset);
			memset((char *)memory + head + ((recordSize > 8) ? recordSize : 8) + maxOffset - firstOffset, 0, diff);
			if (recordSize < 8) {
				memset((char *) memory + head + recordSize, 0, 8 - recordSize);
			}

			// Update the slot information of the latter records
			memset(pageReader, 0, PAGE_SIZE);
			for (short int j = 0; j < numberOfSlot; j ++) {
				memcpy(pageReader, (char *)memory + PAGE_SIZE - 4 * (j + 2), 2);
				if (*(short int *)pageReader != -1 && *(short int *)pageReader > head) {
					short int newHead = *(short int *)pageReader - diff;
					memcpy((char *)memory + PAGE_SIZE - 4 * (j + 2), &newHead, 2);
				}
			}
		}
		// Update the length of the updated record
		short int newLength = ((recordSize > 8) ? recordSize : 8);
		memcpy((char *)memory + PAGE_SIZE - (slotNum + 2) * 4 + 2, &newLength, sizeof(short int));
		// Update the flag number
		flag += diff;
		memcpy((char *)memory + PAGE_SIZE - 2, &flag, 2);
	}
	else {
		short int diff = recordSize - ((length > 8) ? length : 8);
		if (flag >= diff) {
			// Copy the rest records behind the updated record into pageReader
			memset(pageReader, 0, PAGE_SIZE);
			// Go through the slot table to find the max offset of record (i.e. find the end of record part)
			short int maxHead, maxLength, firstOffset;
			short int maxOffset = -3;
			for (short int j = 0; j < numberOfSlot; j ++) {
				memcpy(pageReader, (char *)memory + PAGE_SIZE - 4 * (j + 2), 4);
				maxHead = *(short int *) pageReader;
				maxLength = *(short int *)((char *)pageReader + 2);
				// Find max offset record
				if (maxHead + ((maxLength > 8) ? maxLength : 8) > maxOffset) {
					maxOffset = maxHead + ((maxLength > 8) ? maxLength : 8);
				}
			}
			firstOffset = head + ((length > 8) ? length : 8);
			memset(pageReader, 0, PAGE_SIZE);
			// The last one is the updated one
			if (firstOffset == maxOffset){
				memcpy((char *)memory + head, record, recordSize);
			}
			else {
				memcpy(pageReader, (char *)memory + firstOffset, maxOffset - firstOffset);
				// Move the latter records backward
				memcpy((char *)memory + head + recordSize, pageReader, maxOffset - firstOffset);
				memcpy((char *)memory + head, record, recordSize);

				// Update the slot information of the latter records
				memset(pageReader, 0, PAGE_SIZE);
				for (short int j = 0; j < numberOfSlot; j ++) {
					memcpy(pageReader, (char *)memory + PAGE_SIZE - 4 * (j + 2), 2);
					if (*(short int *)pageReader != -1 && *(short int *)pageReader > head) {
						short int newHead = *(short int *)pageReader + diff;
						memcpy((char *)memory + PAGE_SIZE - 4 * (j + 2), &newHead, 2);
					}
				}
			}
			// Update the length of the updated record
			short int newLength = recordSize;
			memcpy((char *)memory + PAGE_SIZE - (slotNum + 2) * 4 + 2, &newLength, 2);
			// Update the flag number
			flag -= diff;
			memcpy((char *)memory + PAGE_SIZE - 2, &flag, 2);
		}
		else {
			short int diff = ((length > 8) ? length : 8) - 8;
			RID newRID;
			// Insert the record on another page
			insertRecord(fileHandle, recordDescriptor, data, newRID);

			// Construct the pointer
			void *pointer = malloc(8);
			short int invalid = -2;
			int newPage = newRID.pageNum;
			short int newSlot = newRID.slotNum;
			memcpy(pointer, &invalid, 2);
			memcpy((char *)pointer + 2, &newPage, 4);
			memcpy((char *)pointer + 6, &newSlot, 2);
			memcpy((char *)memory + head, pointer, 8);

			// Update the flag number
			flag += diff;
			memcpy((char *)memory + PAGE_SIZE - 2, &flag, 2);

			// Copy the rest records behind the updated record into pageReader
			memset(pageReader, 0, PAGE_SIZE);
			// Go through the slot table to find the max offset of record (i.e. find the end of record part)
			short int maxHead, maxLength, firstOffset;
			short int maxOffset = -3;
			for (short int j = 0; j < numberOfSlot; j ++) {
				memcpy(pageReader, (char *)memory + PAGE_SIZE - 4 * (j + 2), 4);
				maxHead = *(short int *) pageReader;
				maxLength = *(short int *)((char *)pageReader + 2);
				// Find max offset record
				if (maxHead + ((maxLength > 8) ? maxLength : 8) > maxOffset) {
					maxOffset = maxHead + ((maxLength > 8) ? maxLength : 8);
				}
			}
			firstOffset = head + ((length > 8) ? length : 8);
			memset(pageReader, 0, PAGE_SIZE);
			// The last one is the updated one
			if (firstOffset == maxOffset){
				memset((char *)memory + head + 8, 0, diff);
			}
			else {
				memcpy(pageReader, (char *)memory + firstOffset, maxOffset - firstOffset);
				// Move the latter records forward
				memcpy((char *)memory + head + 8, pageReader, maxOffset - firstOffset);
				memset((char *)memory + head + 8 + maxOffset - firstOffset, 0, diff);

				// Update the slot information of the latter records
				memset(pageReader, 0, PAGE_SIZE);
				for (short int j = 0; j < numberOfSlot; j ++) {
					memcpy(pageReader, (char *)memory + PAGE_SIZE - 4 * (j + 2), 2);
					if (*(short int *)pageReader != -1 && *(short int *)pageReader > head) {
						short int newHead = *(short int *)pageReader - diff;
						memcpy((char *)memory + PAGE_SIZE - 4 * (j + 2), &newHead, 2);
					}
				}
			}
			// Update the length of the updated record
			short int newLength = 8;
			memcpy((char *)memory + PAGE_SIZE - (slotNum + 2) * 4 + 2, &newLength, 2);
			free(pointer);
		}
	}
	fileHandle.writePage(rid.pageNum, memory);
	free(memory);
	free(record);
	free(pageReader);
	return 0;
}

RC RecordBasedFileManager::readAttribute(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, const string &attributeName, void *data) {
	// First, locate the record according to the provided RID
	void *memory = malloc(PAGE_SIZE);
	void *pageReader = malloc(PAGE_SIZE);
	RC rc = fileHandle.readPage(rid.pageNum, memory);

	// Check the invalid input RID
	if (rc != 0) {
		return -1;
	}
	memcpy(pageReader, (char *)memory + PAGE_SIZE - 4, 2);
	if (rid.slotNum >= (unsigned)*(short int *)pageReader) {
		return -1;
	}

	memcpy(pageReader, (char *)memory + PAGE_SIZE - (rid.slotNum + 2) * 4, 4);
	short int head = *(short int *)pageReader;
	short int length = *(short int *)((char *)pageReader + 2);
	// Check whether this record has been deleted
	if (head == -1) {
		return -1;
	}
	int pageNum = (int)rid.pageNum;
	short int slotNum = (short int)rid.slotNum;

	// Judge whether the corresponding record is actually a pointer pointing to another page
	short int fl = *(short int *)((char *)memory + head);
	while (fl == -2) {
		pageNum = *(int *)((char *)memory + head + 2);
		slotNum = *(short int *)((char *)memory + head + 6);
		fileHandle.readPage(pageNum, memory);
		memcpy(pageReader, (char *)memory + PAGE_SIZE - (slotNum + 2) * 4, 4);
		head = *(short int *)pageReader;
		if (head == -1) {
			return -1;
		}
		length = *(short int *)((char *)pageReader + 2);
		fl = *(short int *)((char *)memory + head);
	}
	// Read the corresponding record into the pageReader
	memset(pageReader, 0, PAGE_SIZE);
	memcpy(pageReader, (char *)memory + head, length);
	// Read the assigned attribute
	unsigned int i;
	int index = -1;
	for (i = 0; i < recordDescriptor.size(); i++) {
		if (recordDescriptor[i].name == attributeName) {
			index = i;
			break;
		}
	}
	if (index == -1) {
		return -1;
	}
	AttrType type = recordDescriptor[index].type;
	short int attributeHead = *(short int *)((char *)pageReader + index * 2);

	// Check whether this attribute is NULL
	if (attributeHead != -1) {
		if (type == 0) {
			int n = *(int *)((char *)pageReader + attributeHead);
			memcpy((char *)data + 1, &n, sizeof(int));
		}
		else if (type == 1) {
			float n = *(float *)((char *)pageReader + attributeHead);
			memcpy((char *)data + 1, &n, sizeof(float));
		}
		else {
			unsigned int len = 0;
			string s = "";
			for (i = index + 1; i < recordDescriptor.size(); i++) {
				short int attributeEnd = *(short int *)((char *)pageReader + i * 2);
				if (attributeEnd != -1) {
					len = attributeEnd - attributeHead;
					break;
				}
			}
			if (i == recordDescriptor.size() && len == 0) {
				len = length - attributeHead;
			}
			for (i = 0; i < len; i++) {
				char ch = *((char *)pageReader + attributeHead + i);
				s += ch;
			}
			memcpy((char *)data + 1, &len, sizeof(int));
			memcpy((char *)data + 5, s.c_str(), len);
		}
	}
	void *comp = malloc(PAGE_SIZE);
	if (attributeHead == -1 || memcmp(data, comp, PAGE_SIZE) == 0) {
		int nulls = 128;
		memcpy(data, &nulls, 1);
	}
	else {
		int nulls = 0;
		memcpy(data, &nulls, 1);
	}
	free(comp);
	free(memory);
	free(pageReader);
	return 0;
}

bool RecordBasedFileManager::Comp(void *m1, const void *m2, AttrType type, const CompOp compOp) {
	if (type == 0) {
		int n1 = *(int *)((char *)m1 + 1);
		int n2 = *(int *)m2;
		if (compOp == 0) {
			if (n1 == n2)
				return true;
			else
				return false;
		}
		else if (compOp == 1) {
			if (n1 < n2)
				return true;
			else
				return false;
		}
		else if (compOp == 2) {
			if (n1 <= n2)
				return true;
			else
				return false;
		}
		else if (compOp == 3) {
			if (n1 > n2)
				return true;
			else
				return false;
		}
		else if (compOp == 4) {
			if (n1 >= n2)
				return true;
			else
				return false;
		}
		else if (compOp == 5) {
			if (n1 != n2)
				return true;
			else
				return false;
		}
		else {
			return true;
		}
	}

	else if (type == 1) {
		float n1 = *(float *)((char *)m1 + 1);
		float n2 = *(float *)m2;
		if (compOp == 0) {
			if (n1 == n2)
				return true;
			else
				return false;
		}
		else if (compOp == 1) {
			if (n1 < n2)
				return true;
			else
				return false;
		}
		else if (compOp == 2) {
			if (n1 <= n2)
				return true;
			else
				return false;
		}
		else if (compOp == 3) {
			if (n1 > n2)
				return true;
			else
				return false;
		}
		else if (compOp == 4) {
			if (n1 >= n2)
				return true;
			else
				return false;
		}
		else if (compOp == 5) {
			if (n1 != n2)
				return true;
			else
				return false;
		}
		else {
			return true;
		}
	}

	else {
		string s1 = "";
		string s2 = "";
		int length1 = *(int *)((char *)m1 + 1);
		int length2 = *(int *)m2;
		for (int i = 0; i < length1; i++) {
			char ch = *((char *)m1 + 5 + i);
			s1 += ch;
		}
		for (int i = 0; i < length2; i++) {
			char ch = *((char *)m2 + 4 + i);
			s2 += ch;
		}
		if (compOp == 0) {
			if (s1 == s2)
				return true;
			else
				return false;
		}
		else if (compOp == 1) {
			if (s1 < s2)
				return true;
			else
				return false;
		}
		else if (compOp == 2) {
			if (s1 <= s2)
				return true;
			else
				return false;
		}
		else if (compOp == 3) {
			if (s1 > s2)
				return true;
			else
				return false;
		}
		else if (compOp == 4) {
			if (s1 >= s2)
				return true;
			else
				return false;
		}
		else if (compOp == 5) {
			if (s1 != s2)
				return true;
			else
				return false;
		}
		else {
			return true;
		}
	}
}


RC RecordBasedFileManager::scan(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const string &conditionAttribute,
								const CompOp compOp, const void *value, const vector<string> &attributeNames, RBFM_ScanIterator &rbfm_ScanIterator) {
	void *memory = malloc(PAGE_SIZE);
	void *pageReader = malloc(PAGE_SIZE);
	void *record = malloc(PAGE_SIZE);
	void *comp = malloc(PAGE_SIZE);
	memset(comp, -2, PAGE_SIZE);
	int numberOfPages = fileHandle.getNumberOfPages();
	AttrType type;
	if (compOp != NO_OP) {
		for (unsigned int i = 0; i < recordDescriptor.size(); i++) {
			if (recordDescriptor[i].name == conditionAttribute) {
				type = recordDescriptor[i].type;
				break;
			}
		}
	} else {
		type = TypeVarChar;
	}
	// Interpret the required attributes into a Attribute vector
	vector<Attribute> attrs;
	for (unsigned int i = 0; i < attributeNames.size(); i++) {
		for (unsigned int j = 0; j < recordDescriptor.size(); j++) {
			if (attributeNames[i] == recordDescriptor[j].name) {
				attrs.push_back(recordDescriptor[j]);
				break;
			}
		}
	}
	RID rid;
	short int numberOfSlots;
	for (int i = 0; i < numberOfPages; i++) {
		fileHandle.readPage(i, memory);
		numberOfSlots = *(short int *)((char *)memory + PAGE_SIZE - 4);
		for (int j = 0; j < numberOfSlots; j++) {
			rid.pageNum = i;
			rid.slotNum = j;
			memcpy(pageReader, (char *)memory + PAGE_SIZE - (rid.slotNum + 2) * 4, 4);
			short int head = *(short int *)pageReader;
			memset(pageReader, 0, 4);
			// Check whether this record has been deleted
			if (head == -1) {
				continue;
			}
			// Judge whether the corresponding record is actually a pointer pointing to another pagerbfm_ScanIterator
			short int fl = *(short int *)((char *)memory + head);
			if (fl == -2) {
				// If this record is a pointer, then jump to the next record
				continue;
			}
			memset(pageReader, -2, PAGE_SIZE);
			if (compOp != NO_OP) {
				readAttribute(fileHandle, recordDescriptor, rid, conditionAttribute, pageReader);
				// Check whether this attribute is NULL
				if (memcmp((char *)pageReader + 1, (char *)comp + 1, PAGE_SIZE - 1) == 0) {
					continue;
				}
				if (Comp(pageReader, value, type, compOp)) {
					rbfm_ScanIterator.returnedRID.push_back(rid);
					memset(pageReader, -2, PAGE_SIZE);
					int numberOfAttr = attributeNames.size();
					short int attrHead = 2 * numberOfAttr;
					short int recordSize = attrHead;
					for (int k = 0; k < numberOfAttr; k++) {
						string s = attrs[k].name;
						readAttribute(fileHandle, recordDescriptor, rid, s, pageReader);
						if (memcmp((char *)pageReader + 1, (char *)comp + 1, PAGE_SIZE - 1) != 0) {
							short int p = attrHead;
							memcpy((char *)record + 2 * k, &p, sizeof(short int));
							int len = 4;
							if (attrs[k].type == 2) {
								len = *(int *)((char *)pageReader + 1);
								memcpy((char *)record + attrHead, (char *)pageReader + 5, len);
							}
							else {
								memcpy((char *)record + attrHead, (char *)pageReader + 1, len);
							}
							attrHead += len;
							recordSize = attrHead;
							memset(pageReader, -2, PAGE_SIZE);
						}
						else {
							short int p = -1;
							memcpy((char *)record + 2 * k, &p, sizeof(short int));
						}

					}
					void *data = malloc(PAGE_SIZE);
					int newSize;
					vector<char> vData;
					recordWrapper(attrs, record, data, recordSize, newSize);
					for (int i = 0; i < newSize; i++) {
						char ch = *((char *)data + i);
						vData.push_back(ch);
					}
					rbfm_ScanIterator.returnedData.push_back(vData);
					free(data);
				}
			} else {
				rbfm_ScanIterator.returnedRID.push_back(rid);
				memset(pageReader, -2, PAGE_SIZE);
				int numberOfAttr = attributeNames.size();
				short int attrHead = 2 * numberOfAttr;
				short int recordSize = attrHead;
				for (int k = 0; k < numberOfAttr; k++) {
					string s = attrs[k].name;
					readAttribute(fileHandle, recordDescriptor, rid, s, pageReader);
					if (memcmp((char *)pageReader + 1, (char *)comp + 1, PAGE_SIZE - 1) != 0) {
						short int p = attrHead;
						memcpy((char *)record + 2 * k, &p, sizeof(short int));
						int len = 4;
						if (attrs[k].type == 2) {
							len = *(int *)((char *)pageReader + 1);
							memcpy((char *)record + attrHead, (char *)pageReader + 5, len);
						}
						else {
							memcpy((char *)record + attrHead, (char *)pageReader + 1, len);
						}
						attrHead += len;
						recordSize = attrHead;
						memset(pageReader, -2, PAGE_SIZE);
					}
					else {
						short int p = -1;
						memcpy((char *)record + 2 * k, &p, sizeof(short int));
					}

				}
				void *data = malloc(PAGE_SIZE);
				int newSize;
				// Storage the byte information into char type in a vector
				vector<char> vData;
				recordWrapper(attrs, record, data, recordSize, newSize);
				for (int i = 0; i < newSize; i++) {
					char ch = *((char *)data + i);
					vData.push_back(ch);
				}
				rbfm_ScanIterator.returnedData.push_back(vData);
				free(data);
			}

			memset(pageReader, 0, PAGE_SIZE);
			memset(record, 0, PAGE_SIZE);
		}
	}
	free(memory);
	free(pageReader);
	free(record);
	free(comp);
	return 0;
}

