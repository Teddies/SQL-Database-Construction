
#include "ix.h"

IndexManager* IndexManager::_index_manager = 0;

IndexManager* IndexManager::instance()
{
    if(!_index_manager)
        _index_manager = new IndexManager();

    return _index_manager;
}

IndexManager::IndexManager()
{
}

IndexManager::~IndexManager()
{
}

RC IndexManager::createFile(const string &fileName)
{
	return PagedFileManager::createFile(fileName);
}

RC IndexManager::destroyFile(const string &fileName)
{
	return PagedFileManager::destroyFile(fileName);
}

RC IndexManager::openFile(const string &fileName, IXFileHandle &ixfileHandle)
{
	if (ixfileHandle.ifp != NULL) {
		return -1;
	}
	RC rc = PagedFileManager::openFile(fileName, ixfileHandle.fileHandle);
	if (rc == 0) {
		ixfileHandle.ifp = ixfileHandle.fileHandle.fp;
		ixfileHandle.ixReadPageCounter = ixfileHandle.fileHandle.readPageCounter;
		ixfileHandle.ixWritePageCounter = ixfileHandle.fileHandle.writePageCounter;
		ixfileHandle.ixAppendPageCounter = ixfileHandle.fileHandle.appendPageCounter;
		ixfileHandle.indexFileName = fileName;
	}
	return rc;
}

RC IndexManager::closeFile(IXFileHandle &ixfileHandle)
{
	ixfileHandle.fileHandle.fp = ixfileHandle.ifp;
	ixfileHandle.fileHandle.readPageCounter = ixfileHandle.ixReadPageCounter;
	ixfileHandle.fileHandle.writePageCounter = ixfileHandle.ixWritePageCounter;
	ixfileHandle.fileHandle.appendPageCounter = ixfileHandle.ixAppendPageCounter;
	RC rc = PagedFileManager::closeFile(ixfileHandle.fileHandle);
	ixfileHandle.ifp = NULL;
	ixfileHandle.indexFileName = "";
	return rc;
}

RC IndexManager::varcharComparison(void *memory, const void *key, const short int beginOffset,
                                   const short int endOffset, bool &comparison, bool &equal) {
    for (int j = 0; j < endOffset - beginOffset; ++ j) {
        // case 1: Tree index is "abcde" and key is "abc", where "abcde" > "abc"
        if (j >= *(int *)key) {
            comparison = true;
            break;
        }
        // case 2: Tree index is "abc" and key is "adc", where "abc" < "adc"
        else if (*((char *)memory + beginOffset + j) < *((char *)key + 4 + j)) {
            comparison = false;
            break;
        }
        // case 3: Tree index is "adc" and key is "abc", where "adc" > "abc"
        else if (*((char *)memory + beginOffset + j) > *((char *)key + 4 + j)) {
            comparison = true;
            break;
        }
        // case 4: Tree index is "abc" and key is "abc", where "abc" == "abc"
        else if (j + 1 == *(int *)key && j + 1 == endOffset - beginOffset) {
            equal = true;
            comparison = true;
            break;
        }
        // case 5: Tree index is "abc" and key is "abcde", where "abc" < "abcde"
        else if (j + 1 == endOffset - beginOffset && j + 1 < *(int *)key) {
            comparison = false;
            break;
        }
    }
    return 0;
}

RC IndexManager::searchInNonLeaf(IXFileHandle &ixfileHandle, void *memory, PageNum &page, char &leafIndicator,
                                 const AttrType type, const void *key, vector<PageNum> &trace, bool flag, int &index)
{
	if (key == NULL) {
		page = *(int *)((char *)memory + 5);
		ixfileHandle.readPage(page, memory);
		leafIndicator = *(char *)memory;
		return 0;
	}
	short int numberOfNodes = *(short int *)((char *)memory + 1);
	// A comparator compares two string, true means the index on the tree is greater than or equal to key,
	// where we terminate the loop and get its left pointer
	if (type == TypeVarChar) {
		bool comparison = false;
        bool equal = false;
		short int beginOffset = 9;
        // There is no "next" point for non-leaf page, so the directory starts from the 4094 byte
		short int endOffset = *(short int *)((char *)memory + 4094);
		short int i = 0;
		while (!comparison && i < numberOfNodes) {
			varcharComparison(memory, key, beginOffset, endOffset, comparison, equal);
            // If they are not equal and the index on tree is greater, return the left pointer
			if (comparison && !equal) {
				if (flag) {
					page = *(int *)((char *)memory + beginOffset - 4);
					trace.push_back(page);
					ixfileHandle.readPage(page, memory);
					leafIndicator = *(char *)memory;
				}
				else {
					index = i;
				}
				return 0;
			}
            // If they are equal, return the right pointer
            else if (equal) {
            	if (flag) {
					page = *(int *)((char *)memory + endOffset);
					trace.push_back(page);
					ixfileHandle.readPage(page, memory);
					leafIndicator = *(char *)memory;
            	}
            	else {
            		return -1;
            	}
                return 0;
            }
            else {
                i ++;
                beginOffset = endOffset + 4;
                endOffset = *(short int *)((char *)memory + 4094 - i * 2);
            }
		}
		// All the index on the tree are smaller than the new key, it will go to the rightmost pointer
		if (!comparison) {
			if (flag) {
				page = *(int *)((char *)memory + beginOffset - 4);
				trace.push_back(page);
				ixfileHandle.readPage(page, memory);
				leafIndicator = *(char *)memory;
			}
			else {
				index = i;
			}
		}
	}
	else if (type == TypeInt){
		short int offset = 9;
		for (int i = 0; i < numberOfNodes; ++ i) {
			if (*(int *)((char *)memory + offset) > *(int *)key) {
				if (flag) {
					page = *(int *)((char *)memory + offset - 4);
					trace.push_back(page);
					ixfileHandle.readPage(page, memory);
					leafIndicator = *(char *)memory;
				}
				else {
					index = i;
				}
				return 0;
			}
			offset += 8;
		}
		if (flag) {
			page = *(int *)((char *)memory + offset - 4);
			trace.push_back(page);
			ixfileHandle.readPage(page, memory);
			leafIndicator = *(char *)memory;
		}
		else {
			index = numberOfNodes;
		}
	}
	else {
		short int offset = 9;
		for (int i = 0; i < numberOfNodes; ++ i) {
			if (*(float *)((char *)memory + offset) > *(float *)key) {
				if (flag) {
					page = *(int *)((char *)memory + offset - 4);
					trace.push_back(page);
					ixfileHandle.readPage(page, memory);
					leafIndicator = *(char *)memory;
				}
				else {
					index = i;
				}
				return 0;
			}
			offset += 8;
		}
		if (flag) {
			page = *(int *)((char *)memory + offset - 4);
			trace.push_back(page);
			ixfileHandle.readPage(page, memory);
			leafIndicator = *(char *)memory;
		}
		else {
			index = numberOfNodes;
		}
	}
	return 0;
}

RC IndexManager::searchInLeaf(void *memory, const AttrType &type, const void *key, short int &moveBegin, short int &moveEnd, int &index)
{
    short int numberOfNodes = *(short int *)((char *)memory + 1);
    // In case this is an empty page
    if (numberOfNodes == 0) {
    	moveBegin = 5;
    	moveEnd = 5;
    	index = 0;
    	return 0;
    }
    if (type == TypeVarChar) {
    	if (key == NULL) {
    		moveBegin = 5;
    		moveEnd = *(short int *)((char *)memory + 4092 - numberOfNodes * 2);
    		index = 0;
    		return 0;
    	}
        bool comparison = false;
        bool equal = false;
        // The start offset of the string
        short int beginOffset = 5;
        // The end offset of the string
        short int endOffset = *(short int *)((char *)memory + 4090) - 8;
        short int i = 0;
        while (!comparison && i < numberOfNodes) {
            varcharComparison(memory, key, beginOffset, endOffset, comparison, equal);
            if (comparison) {
                // The start offset of the moving nodes is from the current node
                moveBegin = beginOffset;
                moveEnd = *(short int *)((char *)memory + 4092 - numberOfNodes * 2);
                index = i;
                return 0;
            }
            i ++;
            beginOffset = endOffset + 8;
            endOffset = *(short int *)((char *)memory + 4090 - i * 2) - 8;
        }
        // All the index on the tree are smaller than the new key, so it will be inserted at the rightmost place
        if (!comparison) {
            moveBegin = beginOffset;
            moveEnd = moveBegin;
        }
    }
    else if (type == TypeInt) {
    	if (key == NULL) {
    		moveBegin = 5;
    		moveEnd = PAGE_SIZE - *(short int *)((char *)memory + 3) - 4;
    		index = 0;
    		return 0;
    	}
        short int offset = 5;
        for (int i = 0; i < numberOfNodes; ++ i) {
            if (*(int *)((char *)memory + offset) >= *(int *)key) {
                moveBegin = offset;
                moveEnd = PAGE_SIZE - *(short int *)((char *)memory + 3) - 4;
                index = i;
                return 0;
            }
            offset += 12;
        }
        moveBegin = offset;
        moveEnd = offset;
    }
    else {
    	if (key == NULL) {
    	    moveBegin = 5;
    	    moveEnd = PAGE_SIZE - *(short int *)((char *)memory + 3) - 4;
    	    index = 0;
    	    return 0;
    	}
        short int offset = 5;
        for (int i = 0; i < numberOfNodes; ++ i) {
            if (*(float *)((char *)memory + offset) >= *(float *)key) {
                moveBegin = offset;
                moveEnd = PAGE_SIZE - *(short int *)((char *)memory + 3) - 4;
                index = i;
                return 0;
            }
            offset += 12;
        }
        moveBegin = offset;
        moveEnd = offset;
    }
    index = numberOfNodes;
    return 0;
}

RC IndexManager::leafSplit(IXFileHandle &ixfileHandle, void *memory, void *newPage, AttrType type, vector<PageNum> &trace, void *midKey)
{
    trace.pop_back();
    short int numberOfNodes = *(short int *)((char *)memory + 1);
    short int flag = *(short int *)((char *)memory + 3);
    int newPageNumber = ixfileHandle.getNumberOfPages();
    short int numberOfNodesOld = numberOfNodes / 2;
    short int numberOfNodesNew = numberOfNodes - numberOfNodesOld;
    if (type == TypeVarChar) {
    	// Avoid the split operation separating two identical keys into two pages
    	short int lastEnd;
    	short int splitOffset = *(short int *)((char *)memory + 4092 - 2 * numberOfNodesOld);
    	if (numberOfNodesOld != 1)
    		lastEnd = *(short int *)((char *)memory + 4094 - 2 * numberOfNodesOld);
    	else
    		lastEnd = 5;
    	short int nextRightEnd = *(short int *)((char *)memory + 4090 - 2 * numberOfNodesOld);
    	void *leftLastKey = malloc(PAGE_SIZE);
    	void *rightFirstKey = malloc(PAGE_SIZE);
    	memcpy(leftLastKey, (char *)memory + lastEnd, splitOffset - 8 - lastEnd);
    	memcpy(rightFirstKey, (char *)memory + splitOffset, nextRightEnd - 8 - splitOffset);
    	short int originalSplit = splitOffset;
    	// Move the split boundary to left
    	while (memcmp(leftLastKey, rightFirstKey, splitOffset - 8 - lastEnd) == 0 && numberOfNodesOld >= 1){
    		splitOffset = lastEnd;
    		numberOfNodesOld --;
    		numberOfNodesNew ++;
    		if (numberOfNodesOld != 1)
				lastEnd = *(short int *)((char *)memory + 4094 - 2 * numberOfNodesOld);
			else
				lastEnd = 5;
    		nextRightEnd = *(short int *)((char *)memory + 4090 - 2 * numberOfNodesOld);
    		memset(leftLastKey, 0, PAGE_SIZE);
    		memset(rightFirstKey, 0, PAGE_SIZE);
    		memcpy(leftLastKey, (char *)memory + lastEnd, splitOffset - 8 - lastEnd);
    		memcpy(rightFirstKey, (char *)memory + splitOffset, nextRightEnd - 8 - splitOffset);
    	}
    	// Move the split boundary to right
    	if (numberOfNodesOld == 0) {
    		splitOffset = originalSplit;
    		numberOfNodesOld = numberOfNodes / 2;
    		numberOfNodesNew = numberOfNodes - numberOfNodesOld;
    		if (numberOfNodesOld != 1)
    		    lastEnd = *(short int *)((char *)memory + 4094 - 2 * numberOfNodesOld);
    		else
    		    lastEnd = 5;
    		nextRightEnd = *(short int *)((char *)memory + 4090 - 2 * numberOfNodesOld);
    		memset(leftLastKey, 0, PAGE_SIZE);
    		memset(rightFirstKey, 0, PAGE_SIZE);
    		memcpy(leftLastKey, (char *)memory + lastEnd, splitOffset - 8 - lastEnd);
    		memcpy(rightFirstKey, (char *)memory + splitOffset, nextRightEnd - 8 - splitOffset);
    		while (memcmp(leftLastKey, rightFirstKey, splitOffset - 8 - lastEnd) == 0 && numberOfNodesNew >= 1){
    			splitOffset = nextRightEnd;
				numberOfNodesOld ++;
				numberOfNodesNew --;
				lastEnd = *(short int *)((char *)memory + 4094 - 2 * numberOfNodesOld);
				nextRightEnd = *(short int *)((char *)memory + 4090 - 2 * numberOfNodesOld);
				memset(leftLastKey, 0, PAGE_SIZE);
				memset(rightFirstKey, 0, PAGE_SIZE);
				memcpy(leftLastKey, (char *)memory + lastEnd, splitOffset - 8 - lastEnd);
				memcpy(rightFirstKey, (char *)memory + splitOffset, nextRightEnd - 8 - splitOffset);
    		}
    	}
    	free(leftLastKey);
    	free(rightFirstKey);

    	short int moveBegin = *(short int *)((char *)memory + 4092 - 2 * numberOfNodesOld);
    	short int moveEnd = *(short int *)((char *)memory + 4092 - 2 * numberOfNodes);
    	for (int i = numberOfNodesOld + 1; i <= numberOfNodes; ++ i) {
    		short int newOffset = *(short int *)((char *)memory + 4092 - 2 * i);
    		newOffset = newOffset - moveBegin + 5;
    		memcpy((char *)memory + 4092 - 2 * i, &newOffset, sizeof(short int));
    	}
    	void *directory = malloc(2 * numberOfNodesNew);
    	memcpy(directory, (char *)memory + 4092 - 2 * numberOfNodes, 2 * numberOfNodesNew);
    	memset((char *)memory + 4092 - 2 * numberOfNodes, 0, 2 * numberOfNodesNew);
    	flag += moveEnd - moveBegin + 2 * numberOfNodesNew;
    	void *data = malloc(moveEnd - moveBegin);
    	memcpy(data, (char *)memory + moveBegin, moveEnd - moveBegin);
    	memset((char *)memory + moveBegin, 0, moveEnd - moveBegin);
    	memcpy((char *)memory + 1, &numberOfNodesOld, sizeof(short int));
    	memcpy((char *)memory + 3, &flag, sizeof(short int));
    	int nextPage = *(int *)((char *)memory + 4092);
    	memcpy((char *)memory + 4092, &newPageNumber, sizeof(int));
    	// Set up a new page
    	char leafIndicator = '1';
    	flag = PAGE_SIZE - (9 + moveEnd - moveBegin + 2 * numberOfNodesNew);
    	memcpy((char *)newPage + 4092, &nextPage, sizeof(int));
    	memcpy((char *)newPage + 4092 - 2 * numberOfNodesNew, directory, 2 * numberOfNodesNew);
    	memcpy(newPage, &leafIndicator, sizeof(char));
    	memcpy((char *)newPage + 1, &numberOfNodesNew, sizeof(short int));
    	memcpy((char *)newPage + 3, &flag, sizeof(short int));
    	memcpy((char *)newPage + 5, data, moveEnd - moveBegin);
    	int firstKeyEnd = *(short int *)((char *)newPage + 4090);
    	firstKeyEnd -= 13;
    	memcpy(midKey, &firstKeyEnd, sizeof(int));
    	memcpy((char *)midKey + 4, (char *)newPage + 5, firstKeyEnd);
    	free(data);
    	free(directory);
    }
    else {
    	// Avoid the split operation separating two identical keys into two pages
    	short int originalSplitOffset, splitOffset;
    	splitOffset = 5 + 12 * numberOfNodesOld;
    	originalSplitOffset = splitOffset;
    	void *leftLastKey = malloc(4);
    	void *rightFirstKey = malloc(4);
    	memcpy(leftLastKey, (char *)memory + splitOffset - 12, 4);
    	memcpy(rightFirstKey, (char *)memory + splitOffset, 4);
    	while (memcmp(leftLastKey, rightFirstKey, 4) == 0 && numberOfNodesOld >= 1) {
    		splitOffset -= 12;
    		numberOfNodesOld --;
    		numberOfNodesNew ++;
    		memcpy(leftLastKey, (char *)memory + splitOffset - 12, 4);
    		memcpy(rightFirstKey, (char *)memory + splitOffset, 4);
    	}
    	if (numberOfNodesOld == 0) {
    		splitOffset = originalSplitOffset;
    		numberOfNodesOld = numberOfNodes/ 2;
    		numberOfNodesNew = numberOfNodes - numberOfNodesOld;
    		memcpy(leftLastKey, (char *)memory + splitOffset - 12, 4);
    		memcpy(rightFirstKey, (char *)memory + splitOffset, 4);
    		while (memcmp(leftLastKey, rightFirstKey, 4) == 0 && numberOfNodesNew >= 1) {
    			splitOffset += 12;
				numberOfNodesOld ++;
				numberOfNodesNew --;
				memcpy(leftLastKey, (char *)memory + splitOffset, 4);
				memcpy(rightFirstKey, (char *)memory + splitOffset + 12, 4);
    		}
    	}
    	free(leftLastKey);
    	free(rightFirstKey);

        short int moveBegin = 5 + 12 * numberOfNodesOld;
        short int moveEnd = 5 + 12 * numberOfNodes;
        flag += moveEnd - moveBegin;
        void *data = malloc(moveEnd - moveBegin);
        memcpy(data, (char *)memory + moveBegin, moveEnd - moveBegin);
        memset((char *)memory + moveBegin, 0, moveEnd - moveBegin);
        memcpy((char *)memory + 1, &numberOfNodesOld, sizeof(short int));
        memcpy((char *)memory + 3, &flag, sizeof(short int));
        int nextPage = *(int *)((char *)memory + 4092);
        memcpy((char *)memory + 4092, &newPageNumber, sizeof(int));
        // Set up a new page
        char leafIndicator = '1';
        flag = PAGE_SIZE - (9 + moveEnd - moveBegin);
        memcpy((char *)newPage + 4092, &nextPage, sizeof(int));
        memcpy(newPage, &leafIndicator, sizeof(char));
        memcpy((char *)newPage + 1, &numberOfNodesNew, sizeof(short int));
        memcpy((char *)newPage + 3, &flag, sizeof(short int));
        memcpy((char *)newPage + 5, data, moveEnd - moveBegin);
        memcpy(midKey, (char *)newPage + 5, 4);
        free(data);
    }
    return 0;
}

RC IndexManager::indexSplit(IXFileHandle &ixfileHandle, void *memory, void *newPage, AttrType type, vector<PageNum> &trace, void *leftPageLastKey, void *rightPageFirstKey)
{
	trace.pop_back();
	short int numberOfNodes = *(short int *)((char *)memory + 1);
	short int flag = *(short int *)((char *)memory + 3);
	short int numberOfNodesOld = numberOfNodes / 2;
	short int numberOfNodesNew = numberOfNodes - numberOfNodesOld;
	if (type == TypeVarChar) {
		short int moveBegin = *(short int *)((char *)memory + 4096 - 2 * numberOfNodesOld);
		short int moveEnd = *(short int *)((char *)memory + 4096 - 2 * numberOfNodes) + 4;
		for (int i = numberOfNodesOld + 1; i <= numberOfNodes; ++ i) {
			short int newOffset = *(short int *)((char *)memory + 4096 - 2 * i);
			newOffset = newOffset - moveBegin + 5;
			memcpy((char *)memory + 4096 - 2 * i, &newOffset, sizeof(short int));
		}
		void *directory = malloc(2 * numberOfNodesNew);
		memcpy(directory, (char *)memory + 4096 - 2 * numberOfNodes, 2 * numberOfNodesNew);
		memset((char *)memory + 4096 - 2 * numberOfNodes, 0, 2 * numberOfNodesNew);
		flag += moveEnd - moveBegin + 2 * numberOfNodesNew - 4;
		void *data = malloc(moveEnd - moveBegin);
		memcpy(data, (char *)memory + moveBegin, moveEnd - moveBegin);
		memset((char *)memory + moveBegin, 0, moveEnd - moveBegin);
		memcpy((char *)memory + 1, &numberOfNodesOld, sizeof(short int));
		memcpy((char *)memory + 3, &flag, sizeof(short int));
		void *lastPointer = malloc(4);
		memcpy(lastPointer, data, 4);
		memcpy((char *)memory + moveBegin, lastPointer, 4);
		free(lastPointer);
		// Set up a new page
		char leafIndicator = '0';
		flag = PAGE_SIZE - (5 + moveEnd - moveBegin + 2 * numberOfNodesNew);
		memcpy((char *)newPage + 4096 - 2 * numberOfNodesNew, directory, 2 * numberOfNodesNew);
		memcpy(newPage, &leafIndicator, sizeof(char));
		memcpy((char *)newPage + 1, &numberOfNodesNew, sizeof(short int));
		memcpy((char *)newPage + 3, &flag, sizeof(short int));
		memcpy((char *)newPage + 5, data, moveEnd - moveBegin);
		int firstKeyEnd = *(short int *)((char *)newPage + 4094);
		firstKeyEnd -= 9;
		// Get the first key of the right page
		memcpy(rightPageFirstKey, &firstKeyEnd, sizeof(int));
		memcpy((char *)rightPageFirstKey + 4, (char *)newPage + 9, firstKeyEnd);
		// Get the last key of the left page
		int len;
		if (numberOfNodesOld == 1) {
			len = *(short int *)((char *)memory + 4094) - 9;
		}
		else {
			len = *(short int *)((char *)memory + 4096 - 2 * numberOfNodesOld) - *(short int *)((char *)memory + 4096 - 2 * (numberOfNodesOld - 1)) - 4;
		}
		memcpy(leftPageLastKey, &len, sizeof(int));
		memcpy((char *)leftPageLastKey + 4, (char *)memory + moveBegin - len, len);
		free(data);
		free(directory);
	}
	else {
		short int moveBegin = 5 + 8 * numberOfNodesOld + 4;
		short int moveEnd = 5 + 8 * numberOfNodes + 4;
		flag += moveEnd - moveBegin;
		void *data = malloc(moveEnd - moveBegin + 4);
		memcpy(data, (char *)memory + moveBegin - 4, moveEnd - moveBegin + 4);
		memset((char *)memory + moveBegin, 0, moveEnd - moveBegin);
		memcpy((char *)memory + 1, &numberOfNodesOld, sizeof(short int));
		memcpy((char *)memory + 3, &flag, sizeof(short int));
		// Set up a new page
		char leafIndicator = '0';
		flag = PAGE_SIZE - (9 + moveEnd - moveBegin);
		memcpy(newPage, &leafIndicator, sizeof(char));
		memcpy((char *)newPage + 1, &numberOfNodesNew, sizeof(short int));
		memcpy((char *)newPage + 3, &flag, sizeof(short int));
		memcpy((char *)newPage + 5, data, moveEnd - moveBegin + 4);
		memcpy(rightPageFirstKey, (char *)newPage + 9, 4);
		memcpy(leftPageLastKey, (char *)memory + moveBegin - 8, 4);
		free(data);
	}
	return 0;
}

RC IndexManager::insertOnLeafPage(void *memory, const void * key, AttrType type, const RID &rid)
{
	short int flag = *(short int *)((char *)memory + 3);
	short int numberOfNodes = *(short int *)((char *)memory + 1);
	if (type == TypeVarChar) {
		int length = *(int *)key;
		flag -= length + 10;
		numberOfNodes ++;
		short int moveBegin, moveEnd;
		int index = 0;
		searchInLeaf(memory, type, key, moveBegin, moveEnd, index);
		// Move the data
		if (moveBegin != moveEnd) {
			void *data = malloc(moveEnd - moveBegin);
		    memcpy(data, (char *)memory + moveBegin, moveEnd - moveBegin);
		    memcpy((char *)memory + moveBegin + 8 + length, data, moveEnd - moveBegin);
		    free(data);
		    void *directory = malloc((numberOfNodes - 1 - index) * 2);
		    for (int i = index; i < numberOfNodes - 1; ++ i) {
		        short int newOffset = *(short int *)((char *)memory + 4090 - i * 2) + 8 + length;
		        memcpy((char *)memory + 4090 - i * 2, &newOffset, 2);
		    }
		    memcpy(directory, (char *)memory + 4092 - (numberOfNodes - 1) * 2, (numberOfNodes - 1 - index) * 2);
		    memcpy((char *)memory + 4092 - 2 * numberOfNodes, directory, (numberOfNodes - 1 - index) * 2);
		    free(directory);
		}
		 // Insert the new node in the leaf
		short int end = moveBegin + 8 + length;
		memcpy((char *)memory + 4090 - index * 2, &end, sizeof(short int));
		memcpy((char *)memory + moveBegin, (char *)key + 4, length);
		memcpy((char *)memory + moveBegin + length, &rid.pageNum, sizeof(unsigned int));
		memcpy((char *)memory + moveBegin + 4 + length, &rid.slotNum, sizeof(unsigned int));
		// Update the flag and numberOfNodes
		memcpy((char *)memory + 1, &numberOfNodes, sizeof(short int));
		memcpy((char *)memory + 3, &flag, sizeof(short int));
	}
	else {
		flag -= 12;
		numberOfNodes ++;
		short int moveBegin, moveEnd;
		int index;
		searchInLeaf(memory, type, key, moveBegin, moveEnd, index);
		// Move the data
		if (moveBegin != moveEnd) {
		    void *data = malloc(moveEnd - moveBegin);
		    memcpy(data, (char *)memory + moveBegin, moveEnd - moveBegin);
		    memcpy((char *)memory + moveBegin + 12, data, moveEnd - moveBegin);
		    free(data);
		}
		// Insert the new node in the leaf
		memcpy((char *)memory + moveBegin, key, 4);
		memcpy((char *)memory + moveBegin + 4, &rid.pageNum, sizeof(unsigned int));
		memcpy((char *)memory + moveBegin + 8, &rid.slotNum, sizeof(unsigned int));
		// Update the flag and numberOfNodes
		memcpy((char *)memory + 1, &numberOfNodes, sizeof(short int));
		memcpy((char *)memory + 3, &flag, sizeof(short int));
	}
	return 0;
}

RC IndexManager::insertOnIndexPage(IXFileHandle &ixfileHandle, void *memory, const void *key, AttrType type, PageNum newPageNumber)
{
	short int flag = *(short int *)((char *)memory + 3);
	short int numberOfNodes = *(short int *)((char *)memory + 1);
	short int moveBegin, moveEnd;
	int index = 0;
	vector<PageNum> trace;
	PageNum page = 0;
	char leafIndicator = '1';
	if (type == TypeVarChar) {
		int length = *(int *)key;
		flag -= length + 6;
		searchInNonLeaf(ixfileHandle, memory, page, leafIndicator, type, key, trace, false, index);
		if (index == 0) {
			moveBegin = 9;
		}
		else {
			moveBegin = *(short int *)((char *)memory + 4096 - 2 * index) + 4;
		}
		moveEnd = *(short int *)((char *)memory + 4096 - 2 * numberOfNodes) + 4;
		// Move the data
		if (moveBegin != moveEnd) {
			void *data = malloc(moveEnd - moveBegin);
			memcpy(data, (char *)memory + moveBegin, moveEnd - moveBegin);
			memset((char *)memory + moveBegin, 0, moveEnd- moveBegin);
			memcpy((char *)memory + moveBegin + length + 4, data, moveEnd - moveBegin);
			free(data);
		}
		memcpy((char *)memory + moveBegin, (char *)key + 4, length);
		memcpy((char *)memory + moveBegin + length, &newPageNumber, sizeof(int));
		// Move the directory
		moveBegin = 4096 - 2 * index;
		moveEnd = 4096 - 2 * numberOfNodes;
		void *directory = malloc(moveBegin - moveEnd);
		memcpy(directory, (char *)memory + 4096 - 2 * numberOfNodes, moveBegin - moveEnd);
		memset((char *)memory + 4096 - 2 * numberOfNodes, 0, moveBegin - moveEnd);
		short int lastPointer;
		if (index == 0) {
			lastPointer = 9 + length;
		}
		else {
			lastPointer = *(short int *)((char *)memory + moveBegin);
			lastPointer += 4 + length;
		}
		memcpy((char *)memory + moveBegin - 2, &lastPointer, sizeof(short int));
		for (int i = 0; i < numberOfNodes - index; ++ i) {
			lastPointer = *(short int *)((char *)directory + 2 * i);
			lastPointer += 4 + length;
			memcpy((char *)directory + 2 * i, &lastPointer, sizeof(short int));
		}
		memcpy((char *)memory + 4096 - 2 * (numberOfNodes + 1), directory, moveBegin - moveEnd);
		free(directory);
	}
	else {
		flag -= 8;
		searchInNonLeaf(ixfileHandle, memory, page, leafIndicator, type, key, trace, false, index);
		moveBegin = 5 + 8 * index + 4;
		moveEnd = 5 + 8 * numberOfNodes + 4;
		// Move the data
		if (moveBegin != moveEnd) {
			void *data = malloc(moveEnd - moveBegin);
			memcpy(data, (char *)memory + moveBegin, moveEnd - moveBegin);
			memset((char *)memory + moveBegin, 0, moveEnd- moveBegin);
			memcpy((char *)memory + moveBegin + 8, data, moveEnd - moveBegin);
			free(data);
		}
		memcpy((char *)memory + moveBegin, key, 4);
		memcpy((char *)memory + moveBegin + 4, &newPageNumber, sizeof(int));
	}
	// Update the flag and numberOfNodes
	numberOfNodes ++;
	memcpy((char *)memory + 1, &numberOfNodes, sizeof(short int));
	memcpy((char *)memory + 3, &flag, sizeof(short int));
	return 0;
}

RC IndexManager::updateIndexPage(IXFileHandle &ixfileHandle, void *memory, const void *key, AttrType type, void *midKey, bool leftIndicator, PageNum newPageNumber)
{
	short int numberOfNodes = *(short int *)((char *)memory + 1);
	short int flag = *(short int *)((char *)memory + 3);
	numberOfNodes --;
	if (leftIndicator) {
		if (type == TypeVarChar) {
			short int end = *(short int *)((char *)memory + 4096 - 2 * numberOfNodes) + 4;
			// The length of the last key and the pointer after it
			short int len = *(short int *)((char *)memory + 4096 - 2 * (numberOfNodes + 1)) + 4 - end;
			int keyLength = len - 4;
			memcpy(midKey, &keyLength, sizeof(int));
			memcpy((char *)midKey + 4, (char *)memory + end, keyLength);
			memset((char *)memory + end, 0, len);
			memset((char *)memory + 4096 - 2 * (numberOfNodes + 1), 0, 2);
			flag += len + 2;
		}
		else {
			memcpy(midKey, (char *)memory + 5 + 8 * numberOfNodes + 4, sizeof(int));
			memset((char *)memory + 5 + 8 * numberOfNodes + 4, 0, 8);
			flag += 8;
		}
	}
	else {
		if (type == TypeVarChar) {
			int len = *(short int *)((char *)memory + 4094) - 9;
			memcpy(midKey, &len, sizeof(int));
			memcpy((char *)midKey + 4, (char *)memory + 9, len);
			short int moveBegin = 9 + len;
			short int moveEnd = *(short int *)((char *)memory + 4096 - 2 * (numberOfNodes + 1)) + 4;
			void *data = malloc(moveEnd - moveBegin);
			memcpy(data, (char *)memory + moveBegin, moveEnd - moveBegin);
			memset((char *)memory + moveBegin, 0, moveEnd - moveBegin);
			memcpy((char *)memory + 5, data, moveEnd - moveBegin);
			free(data);
			for (int i = 2; i <= numberOfNodes + 1; ++ i) {
				short int newOffset = *(short int *)((char *)memory + 4096 - i * 2);
				newOffset -= len + 4;
				memcpy((char *)memory + 4096 - i * 2, &newOffset, sizeof(short int));
			}
			void *directory = malloc(2 * numberOfNodes);
			memcpy(directory, (char *)memory + 4096 - 2 * (numberOfNodes + 1), 2 * numberOfNodes);
			memset((char *)memory + 4096 - 2 * (numberOfNodes + 1), 0, 2);
			memcpy((char *)memory + 4096 - 2 * numberOfNodes, directory, 2 * numberOfNodes);
			free(directory);
			flag += 4 + len + 2;
		}
		else {
			void *data = malloc(8 * numberOfNodes);
			memcpy(midKey, (char *)memory + 9, sizeof(int));
			memcpy(data, (char *)memory + 13, 8 * numberOfNodes);
			memset((char *)memory + 13, 0, 8 * numberOfNodes);
			memcpy((char *)memory + 5, data, 8 * numberOfNodes);
			free(data);
			flag += 8;
		}
	}
	memcpy((char *)memory + 1, &numberOfNodes, sizeof(short int));
	memcpy((char *)memory + 3, &flag, sizeof(short int));
	insertOnIndexPage(ixfileHandle, memory, key, type, newPageNumber);
	return 0;
}

RC IndexManager::generateRoot(IXFileHandle &ixfileHandle, void *midKey, AttrType type, PageNum left, PageNum right)
{
	void *rootPage = malloc(PAGE_SIZE);
	memset(rootPage, 0, PAGE_SIZE);
	char leafIndicator = '0';
	short int numberOfNodes = 1;
	short int flag;
	memcpy(rootPage, &leafIndicator, sizeof(char));
	memcpy((char *)rootPage + 1, &numberOfNodes, sizeof(short int));
	if (type == TypeVarChar) {
		int length = *(int *)midKey;
		flag = PAGE_SIZE - 15 - length;
		memcpy((char *)rootPage + 3, &flag, sizeof(short int));
		int pointerLeft = (int) left;
		int pointerRight = (int) right;
		memcpy((char *)rootPage + 5, &pointerLeft, sizeof(int));
		memcpy((char *)rootPage + 9, (char *)midKey + 4, length);
		memcpy((char *)rootPage + 9 + length, &pointerRight, sizeof(int));
		short int endOffset = 9 + length;
		memcpy((char *)rootPage + 4094, &endOffset, sizeof(short int));
	}
	else {
		flag = PAGE_SIZE - 17;
		memcpy((char *)rootPage + 3, &flag, sizeof(short int));
		int pointerLeft = (int) left;
		int pointerRight = (int) right;
		memcpy((char *)rootPage + 5, &pointerLeft, sizeof(int));
		memcpy((char *)rootPage + 9, midKey, sizeof(int));
		memcpy((char *)rootPage + 13, &pointerRight, sizeof(int));
	}
	// Insert this root page and update the root page in the first virtual page
	ixfileHandle.appendPage(rootPage);
	int root = ixfileHandle.getNumberOfPages() - 1;
	ixfileHandle.fileHandle.updateRoot(root); ixfileHandle.fileHandle.updateRoot(root);
	free(rootPage);
	return 0;
}

bool IndexManager::comp(const void *m1, void *m2, AttrType type)
{
	if (type == TypeVarChar) {
		int l1 = *(int *)m1;
		int l2 = *(int *)m2;
		int l = l1 < l2 ? l1 : l2;
		for (int i = 0; i < l; ++ i) {
			if (*((char *)m1 + 4 + i) < *((char *)m2 + 4 + i))
				return false;
			else if (*((char *)m1 + 4 + i) > *((char *)m2 + 4 + i))
				return true;
		}
		if (l1 < l2)
			return false;
		else
			return true;
	}
	else if (type == TypeInt) {
		int n1 = *(int *)m1;
		int n2 = *(int *)m2;
		return n1 >= n2;
	}
	else {
		float n1 = *(float *)m1;
		float n2 = *(float *)m2;
		return n1 >= n2;
	}
}

RC IndexManager::insertEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
    void *memory = malloc(PAGE_SIZE);
    memset(memory, 0, PAGE_SIZE);
    int root = 0;
    // There is no root page number in the virtual first page
    if (ixfileHandle.getNumberOfPages() == 0) {
    	// Set this page as the root page, which actually is also a leaf page
    	ixfileHandle.fileHandle.updateRoot(root);
    	// Prepare for the header and nextPage pointer for this page
    	void *leaf = malloc(PAGE_SIZE);
    	memset(leaf, 0, PAGE_SIZE);
    	short int numberOfNodes = 0;
    	short int leafFlag = 4087;
    	int nextPage = -1;
    	char leafIndicator = '1';
    	memcpy(leaf, &leafIndicator, sizeof(char));
        memcpy((char *)leaf + 1, &numberOfNodes, sizeof(short int));
        memcpy((char *)leaf + 3, &leafFlag, sizeof(short int));
        memcpy((char *)leaf + 4092, &nextPage, sizeof(int));
        // A new file, no search required, directly insert
        insertOnLeafPage(leaf, key, attribute.type, rid);
        ixfileHandle.appendPage(leaf);
    	free(leaf);
    }
    // The file already had pages
    else {
    	int root;
        int length = *(int *)key;
        int index;
        vector<PageNum> trace;
    	PageNum page;
        if (ixfileHandle.getNumberOfPages() != 1) {
            ixfileHandle.fileHandle.readRoot(root);

            trace.push_back((PageNum)root);
            ixfileHandle.readPage((PageNum)root, memory);
            char leafIndicator = *(char *)memory;
            while(leafIndicator != '1') {
                searchInNonLeaf(ixfileHandle, memory, page, leafIndicator, attribute.type, key, trace, true, index);
            }
        }
        else {
            page = 0;
            trace.push_back(page);
            ixfileHandle.readPage(page, memory);
        }

    	// Check whether there is enough space to insert the new key
        if (attribute.type == TypeVarChar) {
            // If there is enough space in the leaf
            if (*(short int *)((char *)memory + 3) >= length + 10) {
            	insertOnLeafPage(memory, key, attribute.type, rid);
            	ixfileHandle.writePage(page, memory);
            }
            // If not, split the leaf
            else {
                void *newPage = malloc(PAGE_SIZE);
                memset(newPage, 0, PAGE_SIZE);
                void *midKey = malloc(PAGE_SIZE);
                void *leftKey = malloc(PAGE_SIZE);
                void *rightKey = malloc(PAGE_SIZE);
                leafSplit(ixfileHandle, memory, newPage, attribute.type, trace, midKey);

                // The new split page must be the last page
                PageNum newPageNum = ixfileHandle.getNumberOfPages();
                // Insert the new leaf
                int midKeyLength = *(int *)midKey;
                // Judge which leaf page the new leaf node should be inserted
                if (comp(key, midKey, attribute.type)) {
                	insertOnLeafPage(newPage, key, attribute.type, rid);
                }
                else {
                	insertOnLeafPage(memory, key, attribute.type, rid);
                }
                // Flush these two pages to the disk
                ixfileHandle.writePage(page, memory);
                ixfileHandle.appendPage(newPage);

                memset(newPage, 0, PAGE_SIZE);
                void *lowLayerKey = malloc(PAGE_SIZE);
                int len = *(int *)key;
                memcpy(lowLayerKey, key, 4 + len);
                // Update the root
                if (trace.empty()) {
                	// Generate a root index page
                	generateRoot(ixfileHandle, midKey, attribute.type, page, newPageNum);
                }
                else {
                	page = (PageNum)trace.back();
                	ixfileHandle.readPage(page, memory);
                	short int flag = *(short int *)((char *)memory + 3);
                	if (midKeyLength + 6 <= flag) {
                		// Insert the new Key in the index page
                		insertOnIndexPage(ixfileHandle, memory, midKey, attribute.type, newPageNum);
                		ixfileHandle.writePage(page, memory);
                	}
                	else {
                		while (midKeyLength + 6 > flag) {
                			// lowLayerKey = midKey;
                			len = *(int *)midKey;
                			memset(lowLayerKey, 0, PAGE_SIZE);
                			memcpy(lowLayerKey, midKey, 4 + len);
                			memset(midKey, 0, PAGE_SIZE);
                			memset(leftKey, 0, PAGE_SIZE);
                			memset(rightKey, 0, PAGE_SIZE);
                			// Split the index
                			indexSplit(ixfileHandle, memory, newPage, attribute.type, trace, leftKey, rightKey);
                			PageNum newPageNum = ixfileHandle.getNumberOfPages();

                			// Insert the index and flush the two pages to the disk
                			// Smaller than the last key of the left page, then insert it in the left page
                			if (!comp(lowLayerKey, leftKey, attribute.type)) {
                				updateIndexPage(ixfileHandle, memory, lowLayerKey, attribute.type, midKey, true, newPageNum);
                			}
                			// Greater than the first key of the right page, then insert it in the right page
                			else if (comp(lowLayerKey, rightKey, attribute.type)) {
                				updateIndexPage(ixfileHandle, newPage, lowLayerKey, attribute.type, midKey, false, newPageNum);
                			}
                			// Between the left last key and the right first key, then it is the mid key which will be upgraded to the upper layer
                			else {
                				memcpy(midKey, lowLayerKey, PAGE_SIZE);
                				int newPointer = ixfileHandle.getNumberOfPages() - 1;
                				memcpy((char *)newPage + 5, &newPointer, sizeof(int));
                			}
                			ixfileHandle.writePage(page, memory);
                			ixfileHandle.appendPage(newPage);
                			PageNum newIndexPageNum = ixfileHandle.getNumberOfPages() - 1;
                			memset(memory, 0, PAGE_SIZE);
                			memset(newPage, 0, PAGE_SIZE);

                			if (trace.empty()) {
                				// Generate a root index page
                				generateRoot(ixfileHandle, midKey, attribute.type, page, newIndexPageNum);
                				break;
                			}
                		   else {
                			   page = (PageNum)trace.back();
                		        ixfileHandle.readPage(page, memory);
                		        trace.pop_back();
                		        flag = *(short int *)((char *)memory + 3);
                		        if (midKeyLength + 6 <= flag) {
                		            // Insert the new Key in the index page
                		            insertOnIndexPage(ixfileHandle, memory, midKey, attribute.type, newIndexPageNum);
                		        	ixfileHandle.writePage(page, memory);
                		        }
                			}
                		}
                	}
                }
                free(newPage);
                free(midKey);
                free(leftKey);
                free(rightKey);
                free(lowLayerKey);
            }
        }
        // TypeInt or TypeReal
        else {
            short int flag = *(short int *)((char *)memory + 3);
            // If there is enough space in the leaf page
            if (flag >= 12) {
            	insertOnLeafPage(memory, key, attribute.type, rid);
            	ixfileHandle.writePage(page, memory);
            }
            // If not, split the leaf
            else {
                void *newPage = malloc(PAGE_SIZE);
                memset(newPage, 0, PAGE_SIZE);
                void *midKey = malloc(4);
                void *leftKey = malloc(4);
                void *rightKey = malloc(4);
                leafSplit(ixfileHandle, memory, newPage, attribute.type, trace, midKey);
                // The new split page must be the last page
                PageNum newPageNum = ixfileHandle.getNumberOfPages();

                // Judge which leaf page the new leaf node should be inserted
                if (comp(key, midKey, attribute.type)) {
                	insertOnLeafPage(newPage, key, attribute.type, rid);
                }
                else {
                	insertOnLeafPage(memory, key, attribute.type, rid);
                }
                // Flush these two pages to the disk
                ixfileHandle.writePage(page, memory);
                ixfileHandle.appendPage(newPage);

                memset(newPage, 0, PAGE_SIZE);
                void *lowLayerKey = malloc(4);
                memcpy(lowLayerKey, key, 4);
                // Update the root
                if (trace.empty()) {
                	// Generate a root index page
                	generateRoot(ixfileHandle, midKey, attribute.type, page, newPageNum);
                }
                else {
                	page = (PageNum)trace.back();
                	ixfileHandle.readPage(page, memory);
                	flag = *(short int *)((char *)memory + 3);
                	if (flag >= 8) {
                		// Insert the new Key in the index page
                		insertOnIndexPage(ixfileHandle, memory, midKey, attribute.type, newPageNum);
                		ixfileHandle.writePage(page, memory);
                	}
                	else {
                		while (flag < 8) {
                			PageNum oldIndexPageNum = page;
                			// lowLayerKey = midKey;
                			memset(lowLayerKey, 0, 4);
                			memcpy(lowLayerKey, midKey, 4);
                			memset(midKey, 0, 4);
                			memset(leftKey, 0, 4);
                			memset(rightKey, 0, 4);
                			// Split the index
                			indexSplit(ixfileHandle, memory, newPage, attribute.type, trace, leftKey, rightKey);
                			// Insert the index and flush the two pages to the disk
                			// Smaller than the last key of the left page, then insert it in the left page
                			if (!comp(lowLayerKey, leftKey, attribute.type)) {
                				updateIndexPage(ixfileHandle, memory, lowLayerKey, attribute.type, midKey, true, newPageNum);
                			}
                			// Greater than the first key of the right page, then insert it in the right page
                			else if (comp(lowLayerKey, rightKey, attribute.type)) {
                				updateIndexPage(ixfileHandle, newPage, lowLayerKey, attribute.type, midKey, false, newPageNum);
                			}
                			// Between the left last key and the right first key, then it is the mid key which will be upgraded to the upper layer
                			else {
                				memcpy(midKey, lowLayerKey, 4);
                				int newPointer = ixfileHandle.getNumberOfPages() - 1;
                				memcpy((char *)newPage + 5, &newPointer, sizeof(int));
                			}

                			ixfileHandle.writePage(oldIndexPageNum, memory);
                			PageNum newIndexPageNum = ixfileHandle.getNumberOfPages();
                			ixfileHandle.appendPage(newPage);

                			memset(memory, 0, PAGE_SIZE);
                			memset(newPage, 0, PAGE_SIZE);

                			if (trace.empty()) {
                				// Generate a root index page
                				generateRoot(ixfileHandle, midKey, attribute.type, oldIndexPageNum, newIndexPageNum);
                				break;
                			}
                		    else {
                		    	page = trace.back();
                		        ixfileHandle.readPage(page, memory);
                		        trace.pop_back();
                		        flag = *(short int *)((char *)memory + 3);
                		        if (flag >= 8) {
                		            // Insert the new Key in the index page
                		            insertOnIndexPage(ixfileHandle, memory, midKey, attribute.type, newIndexPageNum);
                		        	ixfileHandle.writePage(page, memory);
                		        }
                			}
                		}
                	}
                }
                free(newPage);
                free(midKey);
                free(leftKey);
                free(rightKey);
                free(lowLayerKey);
            }
        }

    }
    free(memory);
	return 0;
}

RC IndexManager::deleteOnLeafPage(void *memory, short int moveBegin, short int moveEnd, int len, int index, AttrType type)
{
	short int flag = *(short int *)((char *)memory + 3);
	short int numberOfNodes = *(short int *)((char *)memory + 1);
	if (moveBegin == moveEnd) {
		memset((char *)memory + moveBegin - 8 - len, 0, 8 + len);
		if (type == TypeVarChar)
			memset((char *)memory + 4092 - 2 * numberOfNodes, 0, 2);
	}
	else {
		void *data = malloc(moveEnd - moveBegin);
		memcpy(data, (char *)memory + moveBegin, moveEnd - moveBegin);
		memcpy((char *)memory + moveBegin - 8 - len, data, moveEnd - moveBegin);
		free(data);
		if (type == TypeVarChar) {
			moveBegin = 4092 - 2 * numberOfNodes;
			moveEnd = 4090 - 2 * index;
			void *directory = malloc(moveEnd - moveBegin);
			memcpy(directory, (char *)memory + moveBegin, moveEnd - moveBegin);
			memset((char *)memory + moveBegin, 0, moveEnd - moveBegin);
			for (int i = 0; i < (moveEnd - moveBegin) / 2; ++ i) {
				short int offset = *(short int *)((char *)directory + 2 * i);
				offset -= 8 + len;
				memcpy((char *)directory + 2 * i, &offset, 2);
			}
			memcpy((char *)memory + moveBegin + 2, directory, moveEnd - moveBegin);
			free(directory);
		}
	}
	numberOfNodes --;
	if (type == TypeVarChar)
		flag += 10 + len;
	else
		flag += 12;
	memcpy((char *)memory + 3, &flag, sizeof(short int));
	memcpy((char *)memory + 1, &numberOfNodes, sizeof(short int));
	return 0;
}

RC IndexManager::deleteEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
    void *memory = malloc(PAGE_SIZE);
    memset(memory, 0, PAGE_SIZE);
    int root = 0, index = 0;
    short int moveBegin, moveEnd;
    PageNum page;
    if (ixfileHandle.getNumberOfPages() == 0) {
    	free(memory);
    	return -1;
    }
    else {
    	if (ixfileHandle.getNumberOfPages() != 1) {
    		ixfileHandle.fileHandle.readRoot(root);
    		ixfileHandle.readPage(root, memory);
    		char leafIndicator = '0';
    		int index;
    		vector<PageNum> trace;
    		while (leafIndicator != '1') {
    			searchInNonLeaf(ixfileHandle, memory, page, leafIndicator, attribute.type, key, trace, true, index);
    		}
    	}
    	else {
    		page = 0;
    	}
    	ixfileHandle.readPage(page, memory);
    	short int numberOfNodes = *(short int *)((char *)memory + 1);
    	searchInLeaf(memory, attribute.type, key, moveBegin, moveEnd, index);
		if (moveBegin == moveEnd) {
			free(memory);
			return -1;
		}
    	if (attribute.type == TypeVarChar) {
    		short int end = *(short int *)((char *)memory + 4090 - 2 * index) - 8;
    		void *nextString = malloc(end - moveBegin);
    		memcpy(nextString, (char *)memory + moveBegin, end - moveBegin);
    		if (memcmp(nextString, (char *)key + 4, end - moveBegin) != 0) {
    			free(memory);
    			free(nextString);
    			return -1;
    		}
    		int len = end - moveBegin;
    		unsigned int pageNum, slotNum;
    		pageNum = *(unsigned int *)((char *)memory + end);
    		slotNum = *(unsigned int *)((char *)memory + end + 4);
    		while (pageNum != rid.pageNum || slotNum != rid.slotNum) {
    			index ++;
    			if (index >= numberOfNodes) {
    				free(memory);
    				free(nextString);
    				return -1;
    			}
    			moveBegin = end + 8;
    			end = *(short int *)((char *)memory + 4090 - 2 * index);
    			if (end - moveBegin != len) {
    				free(memory);
    				free(nextString);
    				return -1;
    			}
    			memcpy(nextString, (char *)memory + moveBegin, end - moveBegin);
    			if (memcmp(nextString, (char *)key + 4, end - moveBegin) != 0) {
    				free(memory);
    				free(nextString);
    			    return -1;
    			}
    			pageNum = *(unsigned int *)((char *)memory + end);
    			slotNum = *(unsigned int *)((char *)memory + end + 4);
    		}
    		deleteOnLeafPage(memory, end + 8, moveEnd, len, index, attribute.type);
    		free(nextString);
    	}
    	else {
    		void *nextKey = malloc(4);
    		memcpy(nextKey, (char *)memory + moveBegin, 4);
    		if (memcmp(nextKey, key, 4) != 0) {
    			free(memory);
    			free(nextKey);
    			return -1;
    		}
    		unsigned int pageNum, slotNum;
    		pageNum = *(unsigned int *)((char *)memory + moveBegin + 4);
    		slotNum = *(unsigned int *)((char *)memory + moveBegin + 8);
    		while (pageNum != rid.pageNum || slotNum != rid.slotNum) {
    		    index ++;
    		    if (index >= numberOfNodes) {
    		    	free(memory);
    		    	free(nextKey);
    		    	return -1;
    		    }
    		    moveBegin += 12;
    		    memcpy(nextKey, (char *)memory + moveBegin, 4);
    		    if (memcmp(nextKey, key, 4) != 0) {
    		    	free(memory);
    		    	free(nextKey);
    		    	return -1;
    		    }
    		    pageNum = *(unsigned int *)((char *)memory + moveBegin + 4);
    		    slotNum = *(unsigned int *)((char *)memory + moveBegin + 8);
    		}
    		deleteOnLeafPage(memory, moveBegin + 12, moveEnd, 4, index, attribute.type);
    		free(nextKey);
    	}
    	ixfileHandle.writePage(page, memory);
    }
	free(memory);
	return 0;
}

RC IndexManager::scan(IXFileHandle &ixfileHandle,
        const Attribute &attribute,
        const void      *lowKey,
        const void      *highKey,
        bool			lowKeyInclusive,
        bool        	highKeyInclusive,
        IX_ScanIterator &ix_ScanIterator)
{
	AttrType type = attribute.type;
	ix_ScanIterator.type = type;
	int rootPage;
	if (ixfileHandle.ifp == NULL) {
		return -1;
	}
	//int situation
	if (type == TypeInt) {
    // lowKey == highKey && ()
    if (lowKey != NULL && highKey != NULL && *(int *)(char *)lowKey == *(int *)(char *)highKey && !lowKeyInclusive && !highKeyInclusive) {
      return 0;
    }
    // readRootPage:
    // situation 1 normal readPage();
    // situation 2 only one leaf in total file, readRoot -> return 0; (the phsical page # of root)
    ixfileHandle.fileHandle.readRoot(rootPage);
    void * memory = malloc(PAGE_SIZE);
    ixfileHandle.readPage(rootPage, memory);
    unsigned page = (unsigned) rootPage;
    char leafIndicator = *(char *)memory;
    vector<PageNum> trace;
    bool flag = true;
    int index;
    while (leafIndicator != '1') {
      //RC IndexManager::searchInNonLeaf(IXFileHandle &ixfileHandle, void *memory, PageNum &page, char &leafIndicator, const AttrType type, const void *key, vector<PageNum> &trace)
      /*
       * ixfileHandle : handler of index file
       * memory : in&out whole page in page 'page'
       * leafIndicator : '0' nonleaf, '1' leaf
       */
       // while until find a leaf node
    	   //RC IndexManager::searchInNonLeaf(IXFileHandle &ixfileHandle, void *memory, PageNum &page, char &leafIndicator,
       //const AttrType type, const void *key, vector<PageNum> &trace, bool flag, int &index)
       searchInNonLeaf(ixfileHandle, memory, page, leafIndicator, type, lowKey, trace, flag, index);
    }
    //solve leaf node
    short int moveBegin, moveEnd;
    index = 0;//index of lowKey # in leaf node page (from 0,...,numberOfNodes - 1)
    //searchInLeaf(void *memory, const AttrType &type, const void *key, short int &moveBegin, short int &moveEnd)
    //target is in [moveBegin, moveEnd], and the element in moveBegin is >= lowKey
    searchInLeaf(memory, type, lowKey, moveBegin, moveEnd, index);

    int nextPageNum = 0;
    int offset = moveBegin;
    RID rid;//rid to be saved in the vector
    //put the satisfied record into the ix_ScanIterator's rid vector to be retrieved iteratively
    short int numberOfNodes = *(short int *)((char *)memory + 1);
    //to skip the equal lowKey if necessary
    if (!lowKeyInclusive) {
      while (*(int *)((char *)memory + offset) == *(int *)(char *)lowKey) {
        index++;
        offset += 12;
      }
      //if all nodes are same from moveBegin to the end of that page (they are all equal to lowKey)
      if (index + 1 > numberOfNodes) {
        //to skip to the next page, read 'nextPageNum' flag from memory
        nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);
        if (nextPageNum == -1) {
          return 0;
        }
        ixfileHandle.readPage(nextPageNum, memory);
        numberOfNodes = *(short int *)((char *)memory + 1);
        offset = 5;
        index = 0;
      }
    }
    // go through the leaf node from the lowKey beginning to the rightmost end of the leaf node chain
    if (highKey == NULL) {
      while (1) {
    	    if (index + 1 <= numberOfNodes) {
    	        rid.pageNum = *(unsigned *)((char *)memory + offset + 4);
    	        rid.slotNum = *(unsigned *)((char *)memory + offset + 8);
    	        ix_ScanIterator.returnedRID.push_back(rid);
    	        vector<char> vKey;
    	        for (int i = 0; i < 4; i++) {
    				char ch = *((char *)memory + offset + i);
    				vKey.push_back(ch);
    			}
    	        ix_ScanIterator.returnedKey.push_back(vKey);
    	        offset += 12;
    	        index++;
    	    }
    	    else {
          nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);
          if (nextPageNum == -1) {
            free(memory);
            return 0; //to the rightmost leaf node, time for function to return
          }
          ixfileHandle.readPage(nextPageNum, memory);
          numberOfNodes = *(short int *)((char *)memory + 1);
          offset = 5;
          index = 0;
        }
      }
    }
    // when highKey is not null, go through to the highKey position
    else {
      // to put the satisfied element into the RID vector in ix_ScanIterator
      while (*(int *)((char *)memory + offset) < *(int *)((char *)highKey)) {
    	  	 if (index + 1 <= numberOfNodes) {
    	         rid.pageNum = *(unsigned *)((char *)memory + offset + 4);
    	         rid.slotNum = *(unsigned *)((char *)memory + offset + 8);
    	         ix_ScanIterator.returnedRID.push_back(rid);
    	         vector<char> vKey;
    	 		for (int i = 0; i < 4; i++) {
    	 			char ch = *((char *)memory + offset + i);
    	 			vKey.push_back(ch);
    	 		}

    	 		void * data = malloc(4);
    	 		memset(data, 0, 4);
    	 		for (unsigned int i = 0; i < vKey.size(); i++) {
    	 		  memcpy((char *)data + i, &vKey[i], sizeof(char));
    	 	  }
    	 		free(data);

    	 		ix_ScanIterator.returnedKey.push_back(vKey);
    	         offset += 12;
    	         index++;
    	  	 }
    	  	 else {//find a next leaf page to traversal
          nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);
          if (nextPageNum == -1) {
            free(memory);
            return 0;
          }
          ixfileHandle.readPage(nextPageNum, memory);
          numberOfNodes = *(short int *)((char *)memory + 1);
          offset = 5;
          index = 0;
        }

      }
      //to include the high key
      if (highKeyInclusive) {
        while (*(int *)((char *)memory + offset) == *(int *)((char *)highKey)) {
        	if (index + 1 <= numberOfNodes) {
			rid.pageNum = *(unsigned *)((char *)memory + offset + 4);
			rid.slotNum = *(unsigned *)((char *)memory + offset + 8);
			ix_ScanIterator.returnedRID.push_back(rid);
			vector<char> vKey;
			for (int i = 0; i < 4; i++) {
			  char ch = *((char *)memory + offset + i);
			  vKey.push_back(ch);
			}
			ix_ScanIterator.returnedKey.push_back(vKey);
			offset += 12;
			index++;
        	}
        	else{
            nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);
            if (nextPageNum == -1) {
              return 0;
            }
            ixfileHandle.readPage(nextPageNum, memory);
            numberOfNodes = *(int *)((char *)memory + 1);
            offset = 5;
            index = 0;
          }
        }
      }
    }
    free(memory);
    //TypeInt end
  }
  //real situation
  else if (type == TypeReal) {
    // lowKey == highKey && ()
    if (lowKey != NULL && highKey != NULL && *(float *)(char *)lowKey == *(float *)(char *)highKey && !lowKeyInclusive && !highKeyInclusive) {
      return 0;
    }
    // readRootPage:
    // 1 readPage();
    // 2 only one leaf in total file, readPage -> return 0; (the phsical page # of root)
    ixfileHandle.fileHandle.readRoot(rootPage);
    void * memory = malloc(PAGE_SIZE);
    ixfileHandle.readPage(rootPage, memory);
    unsigned page = (unsigned) rootPage;
    char leafIndicator = *(char *)memory;
    vector<PageNum> trace;
    bool flag = true;
    int index;
    while (leafIndicator != '1') {
      //	RC IndexManager::searchInNonLeaf(IXFileHandle &ixfileHandle, void *memory, PageNum &page, char &leafIndicator,
    	  //                               const AttrType type, const void *key, vector<PageNum> &trace, bool flag, int &index)
      /*
       * ixfileHandle : handler of index file
       * memory : in&out whole page in page 'page'
       * leafIndicator : '0' nonleaf, '1' leaf
       */
       // while until find a leaf node
       searchInNonLeaf(ixfileHandle, memory, page, leafIndicator, type, lowKey, trace, flag, index);
    }
    //solve leaf node
    short int moveBegin, moveEnd;
    index = 0;//index of lowKey # in leaf node page (from 0,...,numOfSlot - 1)
    //searchInLeaf(void *memory, const AttrType &type, const void *key, short int &moveBegin, short int &moveEnd)
    //target is in [moveBegin, moveEnd], and the element in moveBegin is >= lowKey
    searchInLeaf(memory, type, lowKey, moveBegin, moveEnd, index);

    int nextPageNum = 0;
    int offset = moveBegin;
    RID rid;//rid to be saved in the vector
    //put the satisfied record into the ix_ScanIterator's rid vector to be retrieved iteratively
    short int numberOfNodes = *(short int *)((char *)memory + 1);
    //to skip the equal lowKey if necessary
    if (!lowKeyInclusive) {
      while (*(float *)((char *)memory + offset) == *(float *)(char *)lowKey) {
        index++;
        offset += 12;
      }
      //all nodes are same from moveBegin to the end of that page
      if (index + 1 > numberOfNodes) {
        //to skip to the next page, read 'nextPageNum' flag from memory
        nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);
        if (nextPageNum == -1) {
          free(memory);
          return 0;
        }
        ixfileHandle.readPage(nextPageNum, memory);
        numberOfNodes = *(short int *)((char *)memory + 1);
        offset = 5;
        index = 0;
      }
    }
    // go through the leaf node from the lowKey beginning to the rightmost end of the leaf node chain
    if (highKey == NULL) {
      while (1) {
    	  	if (index + 1 <= numberOfNodes) {
    	        rid.pageNum = *(unsigned *)((char *)memory + offset + 4);
    	        rid.slotNum = *(unsigned *)((char *)memory + offset + 8);
    	        ix_ScanIterator.returnedRID.push_back(rid);
    	        vector<char> vKey;
    			for (int i = 0; i < 4; i++) {
    				char ch = *((char *)memory + offset + i);
    				vKey.push_back(ch);
    			}
    			ix_ScanIterator.returnedKey.push_back(vKey);
    	        offset += 12;
    	        index++;
    	  	}
    	  	else {
          nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);
          if (nextPageNum == -1) {
            free(memory);
            return 0;
          }
          ixfileHandle.readPage(nextPageNum, memory);
          numberOfNodes = *(short int *)((char *)memory + 1);
          offset = 5;
          index = 0;
        }
      }
    }
    // when highKey is not null, go through to the highKey position
    else {
      // to put the satisfied element into the RID vector in ix_ScanIterator
      while (*(float *)((char *)memory + offset) < *(float *)((char *)highKey)) {
    	    if (index + 1 <= numberOfNodes) {
    	        rid.pageNum = *(unsigned *)((char *)memory + offset + 4);
    	        rid.slotNum = *(unsigned *)((char *)memory + offset + 8);
    	        ix_ScanIterator.returnedRID.push_back(rid);
    	        vector<char> vKey;
    			for (int i = 0; i < 4; i++) {
    				char ch = *((char *)memory + offset + i);
    				vKey.push_back(ch);
    			}
    			ix_ScanIterator.returnedKey.push_back(vKey);
    	        offset += 12;
    	        index++;
    	    }
    	    else {//find a next leaf page to traversal
          nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);
          if (nextPageNum == -1) {
            free(memory);
            return 0;
          }
          ixfileHandle.readPage(nextPageNum, memory);
          numberOfNodes = *(short int *)((char *)memory + 1);
          offset = 5;
          index = 0;
        }
      }
      //to include the high key
      if (highKeyInclusive) {
        while (*(float *)((char *)memory + offset) == *(float *)((char *)highKey)) {
        	  if (index + 1 <= numberOfNodes) {
			  rid.pageNum = *(unsigned *)((char *)memory + offset + 4);
			  rid.slotNum = *(unsigned *)((char *)memory + offset + 8);
			  ix_ScanIterator.returnedRID.push_back(rid);
			  vector<char> vKey;
			  for (int i = 0; i < 4; i++) {
				  char ch = *((char *)memory + offset + i);
				  vKey.push_back(ch);
			  }
			  ix_ScanIterator.returnedKey.push_back(vKey);
			  offset += 12;
			  index++;
        	  }
        	  else {
            nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);
            if (nextPageNum == -1) {
              free(memory);
              return 0;
            }
            ixfileHandle.readPage(nextPageNum, memory);
            numberOfNodes = *(int *)((char *)memory + 1);
            offset = 5;
            index = 0;
          }
        }
      }
    }

    free(memory);
    //TypeReal end
  }
  //varchar situation
  else {
    //structure of lowKey, highKey : [length of varchar][varchar]
    //directory: 2 byte slot which points to the end of each node
    // lowKey == highKey && ()
	int lowLength;
	if (lowKey != NULL) {
		lowLength = *(int *)lowKey;
	}
    int entryVarCharLength;
    bool comparison, equal = false;
    if (lowKey != NULL && highKey != NULL) {
    		varcharComparison((void *)lowKey, highKey, 4, lowLength + 4, comparison, equal);
    	    if (equal == true && !lowKeyInclusive && !highKeyInclusive) {
    	      return 0;
    	    }
    }
    // readRootPage:
    // 1 readPage();
    // 2 only one leaf in total file, readPage -> return 0; (the phsical page # of root)
    ixfileHandle.fileHandle.readRoot(rootPage);
    void * memory = malloc(PAGE_SIZE);
    ixfileHandle.readPage(rootPage, memory);
    unsigned page = (unsigned) rootPage;
    char leafIndicator = *(char *)memory;
    vector<PageNum> trace;
    bool flag = true;
    int index;
    while (leafIndicator != '1') {
      //	RC IndexManager::searchInNonLeaf(IXFileHandle &ixfileHandle, void *memory, PageNum &page, char &leafIndicator,
      //                               const AttrType type, const void *key, vector<PageNum> &trace, bool flag, int &index)
      /*
       * ixfileHandle : handler of index file
       * memory : in&out whole page in page 'page'
       * leafIndicator : '0' nonleaf, '1' leaf
       */
       // while until find a leaf node
       searchInNonLeaf(ixfileHandle, memory, page, leafIndicator, type, lowKey, trace, flag, index);
    }
    //solve leaf node
    short int moveBegin, moveEnd;
    index = 0;//index of lowKey # in leaf node page (from 0,...,numOfSlot - 1)
    //searchInLeaf(void *memory, const AttrType &type, const void *key, short int &moveBegin, short int &moveEnd)
    //target is in [moveBegin, moveEnd], and the element in moveBegin is >= lowKey
    searchInLeaf(memory, type, lowKey, moveBegin, moveEnd, index);

    int nextPageNum = 0;
    int offset = moveBegin;
    RID rid;//rid to be saved in the vector
    //put the satisfied record into the ix_ScanIterator's rid vector to be retrieved iteratively
    short int numberOfNodes = *(short int *)((char *)memory + 1);
    // now have numberOfNodes, offset, index
    //to skip the equal lowKey if necessary
    if (!lowKeyInclusive) {
      entryVarCharLength = *(short int *)((char *)memory + PAGE_SIZE - 4 - (index + 1) * 2) - offset - 8;
      equal = false;
      // compare lowKey with memory's exact place
      varcharComparison(memory, lowKey, offset, offset + entryVarCharLength, comparison, equal);
      while (equal == true) {
        offset += entryVarCharLength + 8;
        index++;
        entryVarCharLength = *(short int *)((char *)memory + PAGE_SIZE - 4 - (index + 1) * 2) - offset - 8;
        equal = false;
        varcharComparison(memory, lowKey, offset, offset + entryVarCharLength, comparison, equal);
      }
      if (index + 1 > numberOfNodes) {
        nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);
        if (nextPageNum == -1) {
          free(memory);
          return 0;
        }
        ixfileHandle.readPage(nextPageNum, memory);
        numberOfNodes = *(short int *)((char *)memory + 1);
        offset = 5;
        index = 0;

      }
    }

    if (highKey == NULL) {
      entryVarCharLength = *(short int *)((char *)memory + PAGE_SIZE - 4 - (index + 1) * 2) - offset - 8;
      while (1) {
    	    if (index + 1 <= numberOfNodes) {
    	        rid.pageNum = *(unsigned *)((char *)memory + offset + entryVarCharLength);
    	        rid.slotNum = *(unsigned *)((char *)memory + offset + entryVarCharLength + 4);
    	        ix_ScanIterator.returnedRID.push_back(rid);
    	        vector<char> vKey;
    	        for (int i = 0; i < entryVarCharLength; i++) {
    			   char ch = *((char *)memory + offset + i);
    			   vKey.push_back(ch);
    		    }
    		    ix_ScanIterator.returnedKey.push_back(vKey);
    	        offset += entryVarCharLength + 8;
    	        index++;
    	    }
    	    else {
          nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);
          if (nextPageNum == -1) {
            free(memory);
            return 0;
          }
          ixfileHandle.readPage(nextPageNum, memory);
          numberOfNodes = *(short int *)((char *)memory + 1);
          offset = 5;
          index = 0;
        }
        entryVarCharLength = *(short int *)((char *)memory + PAGE_SIZE - 4 - (index + 1) * 2) - offset - 8;
      }
    }
    else {
      //go through from beginning to the left of highKey
      comparison = true;
      entryVarCharLength = *(short int *)((char *)memory + PAGE_SIZE - 4 - (index + 1) * 2) - offset - 8;
      varcharComparison(memory, highKey, offset, offset + entryVarCharLength, comparison, equal);
      while (comparison == false) {
    	    if (index + 1 <= numberOfNodes) {

    	        rid.pageNum = *(unsigned *)((char *)memory + offset + entryVarCharLength);
    	        rid.slotNum = *(unsigned *)((char *)memory + offset + entryVarCharLength + 4);
    	        ix_ScanIterator.returnedRID.push_back(rid);
    	        vector<char> vKey;
    	        for (int i = 0; i < entryVarCharLength; i++) {
    			   char ch = *((char *)memory + offset + i);
    			   vKey.push_back(ch);
    		    }
    		    ix_ScanIterator.returnedKey.push_back(vKey);

    	        offset += entryVarCharLength + 8;
    	        index++;
    	        if (index + 1 > numberOfNodes) {
    	            nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);
    	            if (nextPageNum == -1) {
    	              free(memory);
    	              return 0;
    	            }
    	            ixfileHandle.readPage(nextPageNum, memory);
    	            numberOfNodes = *(short int *)((char *)memory + 1);
    	            offset = 5;
    	            index = 0;
    	        }
    	    }
    	    else {
          nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);
          if (nextPageNum == -1) {
            free(memory);
            return 0;
          }
          ixfileHandle.readPage(nextPageNum, memory);
          numberOfNodes = *(short int *)((char *)memory + 1);
          offset = 5;
          index = 0;
        }
        comparison = true;
        entryVarCharLength = *(short int *)((char *)memory + PAGE_SIZE - 4 - (index + 1) * 2) - offset - 8;
        varcharComparison(memory, highKey, offset, offset + entryVarCharLength, comparison, equal);
      }

      //to include the high Key
      if (highKeyInclusive) {
        equal = false;
        entryVarCharLength = *(short int *)((char *)memory + PAGE_SIZE - 4 - (index + 1) * 2) - offset - 8;
        varcharComparison(memory, highKey, offset, offset + entryVarCharLength, comparison, equal);
        while (equal == true) {
        	 if (index + 1 <= numberOfNodes) {
               rid.pageNum = *(unsigned *)((char *)memory + offset + entryVarCharLength);
               rid.slotNum = *(unsigned *)((char *)memory + offset + entryVarCharLength + 4);
               ix_ScanIterator.returnedRID.push_back(rid);
               vector<char> vKey;
               for (int i = 0; i < entryVarCharLength; i++) {
     			   char ch = *((char *)memory + offset + i);
     			   vKey.push_back(ch);
     		  }
     	      ix_ScanIterator.returnedKey.push_back(vKey);
               offset += entryVarCharLength + 8;
               index++;
        	 }
        	 else {
            nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);
            if (nextPageNum == -1) {
              free(memory);
              return 0;
            }
            ixfileHandle.readPage(nextPageNum, memory);
            numberOfNodes = *(int *)((char *)memory + 1);
            offset = 5;
            index = 0;
          }
          entryVarCharLength = *(short int *)((char *)memory + PAGE_SIZE - 4 - (index + 1) * 2) - offset - 8;
          equal = false;
		  varcharComparison(memory, highKey, offset, offset + entryVarCharLength, comparison, equal);
        }
      }
    }

    free(memory);
    //varchar end
  }

  return 0;
}

void IndexManager::printBtree(IXFileHandle &ixfileHandle, const Attribute &attribute) const {
	int rootPage;
	ixfileHandle.fileHandle.readRoot(rootPage);
	//get root page
	void *memory = malloc(PAGE_SIZE);
	ixfileHandle.readPage(rootPage, memory);

	unsigned page = (unsigned) rootPage;
	string ans = serializeTree(ixfileHandle, attribute.type, page, memory);
	cout << ans << endl;
	memset(memory, 0, PAGE_SIZE);
	free(memory);
	return;
}

string IndexManager::serializeTree(IXFileHandle &ixfileHandle, const AttrType type, PageNum root, void *memory) const
{
  int numberOfNodes = *(short int *)((char *)memory + 1);
  char leafIndicator = *(char *)memory;
  int index;
  string s = "";
  if (numberOfNodes == 0) {
    return "";
  }

  if (leafIndicator == '0') {
    s += "{\"keys\":[";
    int offset = 5;
    // int branch : length = 4, point = 2
    if (type == TypeInt) {
      int tempKey = *(int *)((char *)memory + offset + 4);//////
      s += "\"" + to_string(tempKey) + "\"";
      for (index = 1; index < numberOfNodes; index++) {
        tempKey = *(int *)((char *)memory + offset + 4 + 8 * index);////
        s += ",\"" + to_string(tempKey) + "\"";
      }
      s += "],\"children\":[";
      void *tempPage = malloc(PAGE_SIZE);
      PageNum tempPageNum = *(unsigned *)((char *)memory + offset);
      ixfileHandle.readPage(tempPageNum, tempPage);
      s += serializeTree(ixfileHandle, type, tempPageNum, tempPage);

      for (index = 0; index < numberOfNodes; index++) {
        tempPageNum = *(unsigned *)((char *)memory + offset + 8 * (1 + index));////
        //memset();
        ixfileHandle.readPage(tempPageNum, tempPage);
        s += ",";
        s += serializeTree(ixfileHandle, type, tempPageNum, tempPage);
      }
      s += "]}";
      free(tempPage);
    }
    // float branch : length = 4, point = 2
    else if (type == TypeReal) {
      int tempKey = *(float *)((char *)memory + offset + 4);//////
      s += "\"" + to_string(tempKey) + "\"";
      for (index = 1; index < numberOfNodes; index++) {
        tempKey = *(float *)((char *)memory + offset + 4 + 8 * index);////
        s += ",\"" + to_string(tempKey) + "\"";
      }
      s += "],\"children\":[";
      void *tempPage = malloc(PAGE_SIZE);
      PageNum tempPageNum = *(unsigned *)((char *)memory + offset);
      ixfileHandle.readPage(tempPageNum, tempPage);
      s += serializeTree(ixfileHandle, type, tempPageNum, tempPage);

      for (index = 0; index < numberOfNodes; index++) {
        tempPageNum = *(unsigned *)((char *)memory + offset + 8 * (1 + index));////
        //memset();
        ixfileHandle.readPage(tempPageNum, tempPage);
        s += ",";
        s += serializeTree(ixfileHandle, type, tempPageNum, tempPage);
      }
      s += "]}";
      free(tempPage);
    }
    // varchar branch : ending offset saved in the end directory, length can be calculated by (ending offset - beginning - 4)
    else {
      //use varchar directory to calculate the length of each element
      string tempKey = "";
      short int tailOffset = *(short int *)((char *)memory + PAGE_SIZE - 2);
      int varcharLength = tailOffset - offset - 4;
      for (int i = 0; i < varcharLength; i++) {
				char ch = *((char *)memory + offset + 4 + i);
				tempKey += ch;
			}
      //int tempKey = *(int *)((char *)memory + offset + 4);
      s += "\"" + tempKey + "\"";
      for (index = 1; index < numberOfNodes; index++) {
        tempKey = "";
        tailOffset = *(short int *)((char *)memory + PAGE_SIZE - 2 * (index + 1));
        varcharLength = tailOffset - *(short int *)((char *)memory + PAGE_SIZE - 2 * index) - 4;
        for (int i = 0; i < varcharLength; i++) {
  				char ch = *((char *)memory + *(short int *)((char *)memory + PAGE_SIZE - 2 * index) + 4 + i);
  				tempKey += ch;
  			}
        //tempKey = *(int *)((char *)memory + offset + 4 + 8 * index);////
        s += ",\"" + tempKey + "\"";
      }
      s += "],\"children\":[";
      void *tempPage = malloc(PAGE_SIZE);
      PageNum tempPageNum = *(unsigned *)((char *)memory + offset);
      ixfileHandle.readPage(tempPageNum, tempPage);
      s += serializeTree(ixfileHandle, type, tempPageNum, tempPage);

      for (index = 0; index < numberOfNodes; index++) {
        tailOffset = *(short int *)((char *)memory + PAGE_SIZE - 2 * (index + 1));
        tempPageNum = *(unsigned *)((char *)memory + tailOffset);////
        //memset();
        ixfileHandle.readPage(tempPageNum, tempPage);
        s += ",";
        s += serializeTree(ixfileHandle, type, tempPageNum, tempPage);
      }
      s += "]}";
      free(tempPage);
    }
  }
  //leafIndicator == '1', leaf node
  else {
    s += "{\"keys\":[";
    int offset = 5;
    index = 0;
    //Key length = 4 bytes, RID = 4 bytes
    if (type == TypeInt) {
      //handle the first one, no ","
      int tempKey = *(int *)((char *)memory + offset);
      RID tempRID;
      tempRID.pageNum = *(unsigned *)((char *)memory + offset + 4);
      tempRID.slotNum = *(unsigned *)((char *)memory + offset + 8);
      index = 1;
      s += "\"" + to_string(tempKey) + ":[(" + to_string(tempRID.pageNum) + "," + to_string(tempRID.slotNum) + ")";
      int before = tempKey;
      for (; index < numberOfNodes;) {
        tempKey = *(int *)((char *)memory + offset + 12 * index);
        if (tempKey == before) {
          tempRID.pageNum = *(unsigned *)((char *)memory + offset + 12 * index + 4);
          tempRID.slotNum = *(unsigned *)((char *)memory + offset + 12 * index + 8);
          index++;
          s += ",(" + to_string(tempRID.pageNum) + "," + to_string(tempRID.slotNum) + ")";
        } else {
          break;
        }
      }

      //handle the rest elements which have a "," in the beginning part
      while (index < numberOfNodes) {
        int tempKey = *(int *)((char *)memory + offset + 12 * index);
        tempRID.pageNum = *(unsigned *)((char *)memory + offset + 12 * index + 4);
        tempRID.slotNum = *(unsigned *)((char *)memory + offset + 12 * index + 8);
        if (tempKey != before) {
          s += "]\",\"" + to_string(tempKey) + ":[(" + to_string(tempRID.pageNum) + "," + to_string(tempRID.slotNum) + ")";
          before = tempKey;
        }
        else {
          s += ",(" + to_string(tempRID.pageNum) + "," + to_string(tempRID.slotNum) + ")";
        }
        index++;
      }
      s += "]\"]}";

      // int nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);

    }
    else if (type == TypeReal) {
      //handle the first one, no ","
      int tempKey = *(float *)((char *)memory + offset);
      RID tempRID;
      tempRID.pageNum = *(unsigned *)((char *)memory + offset + 4);
      tempRID.slotNum = *(unsigned *)((char *)memory + offset + 8);
      index = 1;
      s += "\"" + to_string(tempKey) + ":[(" + to_string(tempRID.pageNum) + "," + to_string(tempRID.slotNum) + ")";
      int before = tempKey;
      for (; index < numberOfNodes;) {
        tempKey = *(float *)((char *)memory + offset + 12 * index);
        if (tempKey == before) {
          tempRID.pageNum = *(unsigned *)((char *)memory + offset + 12 * index + 4);
          tempRID.slotNum = *(unsigned *)((char *)memory + offset + 12 * index + 8);
          index++;
          s += ",(" + to_string(tempRID.pageNum) + "," + to_string(tempRID.slotNum) + ")";
        } else {
          break;
        }
      }

      //handle the rest elements which have a "," in the beginning part
      while (index < numberOfNodes) {
        int tempKey = *(float *)((char *)memory + offset + 12 * index);
        tempRID.pageNum = *(unsigned *)((char *)memory + offset + 8 * index + 4);
        tempRID.slotNum = *(unsigned *)((char *)memory + offset + 8 * index + 8);
        if (tempKey != before) {
          s += "]\",\"" + to_string(tempKey) + ":[(" + to_string(tempRID.pageNum) + "," + to_string(tempRID.slotNum) + ")";
          before = tempKey;
        }
        else {
          s += ",(" + to_string(tempRID.pageNum) + "," + to_string(tempRID.slotNum) + ")";
        }
        index++;
      }
      s += "]\"]}";

      //int nextPageNum = *(int *)((char *)memory + PAGE_SIZE - 4);
    }
    //varchar situation
    else {
      //handle the first one, no ","
      string tempKey = "";
      short int tailOffset = *(short int *)((char *)memory + PAGE_SIZE - 6);
      int varcharLength = tailOffset - offset - 8;
      for (int i = 0; i < varcharLength; i++) {
				char ch = *((char *)memory + offset + i);
				tempKey += ch;
			}
      //int tempKey = *(int *)((char *)memory + offset);
      RID tempRID;
      tempRID.pageNum = *(unsigned *)((char *)memory + tailOffset - 8);
      tempRID.slotNum = *(unsigned *)((char *)memory + tailOffset - 4);
      index = 1;
      s += "\"" + tempKey + ":[(" + to_string(tempRID.pageNum) + "," + to_string(tempRID.slotNum) + ")";
      string before = tempKey;
      for (; index < numberOfNodes;) {
        tailOffset = *(short int *)((char *)memory + PAGE_SIZE - 4 - 2 * (index + 1));
        varcharLength = tailOffset - *(short int *)((char *)memory + PAGE_SIZE - 4 - 2 * index) - 8;
        for (int i = 0; i < varcharLength; i++) {
  				char ch = *((char *)memory + tailOffset - varcharLength - 8 + i);
  				tempKey += ch;
  			}
        //tempKey = *(int *)((char *)memory + offset + 8 * index);
        if (tempKey == before) {
          tempRID.pageNum = *(unsigned *)((char *)memory + tailOffset - 8);
          tempRID.slotNum = *(unsigned *)((char *)memory + tailOffset - 4);
          index++;
          s += ",(" + to_string(tempRID.pageNum) + "," + to_string(tempRID.slotNum) + ")";
        } else {
          break;
        }
      }

      //handle the rest elements which have a "," in the beginning part
      while (index < numberOfNodes) {
        tempKey = "";
        tailOffset = *(short int *)((char *)memory + PAGE_SIZE - 4 - 2 * (index + 1));
        varcharLength = tailOffset - *(short int *)((char *)memory + PAGE_SIZE - 4 - 2 * index) - 8;
        for (int i = 0; i < varcharLength; i++) {
          char ch = *((char *)memory + tailOffset - varcharLength - 8 + i);
          tempKey += ch;
        }
        tempRID.pageNum = *(unsigned *)((char *)memory + tailOffset - 8);
        tempRID.slotNum = *(unsigned *)((char *)memory + tailOffset - 4);
        if (tempKey != before) {
          s += "]\",\"" + tempKey + ":[(" + to_string(tempRID.pageNum) + "," + to_string(tempRID.slotNum) + ")";
          before = tempKey;
        }
        else {
          s += ",(" + to_string(tempRID.pageNum) + "," + to_string(tempRID.slotNum) + ")";
        }
        index++;
      }
      s += "]\"]}";
    }
  }

  return s;
}


IX_ScanIterator::IX_ScanIterator()
{
	index = 0;
}

IX_ScanIterator::~IX_ScanIterator()
{
}

RC IX_ScanIterator::getNextEntry(RID &rid, void *key)
{
  if (index >= (int)returnedRID.size()) {
	  return IX_EOF;
  }
  rid = returnedRID[index];
  vector<char> vKey = returnedKey[index];
  int offset = 0;
  if (this->type == TypeVarChar) {
	  offset = 4;
	  int vKeyLength = vKey.size();
	  memcpy(key, &vKeyLength, 4);
  }
  for (unsigned int i = 0; i < vKey.size(); i++) {
    memcpy((char *)key + offset + i, &vKey[i], sizeof(char));
  }
  index ++;
  return 0;
}

RC IX_ScanIterator::close()

{
	returnedRID.clear();
	returnedKey.clear();
	index = 0;
	return 0;
}


IXFileHandle::IXFileHandle()
{
    ixReadPageCounter = 0;
    ixWritePageCounter = 0;
    ixAppendPageCounter = 0;
    ifp = NULL;
}

IXFileHandle::~IXFileHandle()
{
}

RC IXFileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
	readPageCount = this->ixReadPageCounter;
	writePageCount = this->ixWritePageCounter;
	appendPageCount = this->ixAppendPageCounter;
	return 0;
}

RC IXFileHandle::readPage(PageNum pageNum, void *data)
{
	ixReadPageCounter++;
	return fileHandle.readPage(pageNum, data);
}

RC IXFileHandle::writePage(PageNum pageNum, const void *data)
{
	ixWritePageCounter++;
	return fileHandle.writePage(pageNum, data);
}

RC IXFileHandle::appendPage(const void *data)
{
	ixAppendPageCounter++;
	return fileHandle.appendPage(data);
}

unsigned IXFileHandle::getNumberOfPages()
{
	if (indexFileName != "") {
	    fseek(fileHandle.fp, 0L, 2);
	    double numberOfBytes = (double) ftell(fileHandle.fp);
	    rewind(fileHandle.fp);
	    return ceil(numberOfBytes / PAGE_SIZE) - 1;							// Exclude the virtual "first" page
	}
	else {
	    return -1;
	}
}
