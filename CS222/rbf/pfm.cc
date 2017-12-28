#include "pfm.h"

PagedFileManager* PagedFileManager::_pf_manager = 0;

PagedFileManager* PagedFileManager::instance()
{
    if(!_pf_manager)
        _pf_manager = new PagedFileManager();

    return _pf_manager;
}


PagedFileManager::PagedFileManager()
{
}


PagedFileManager::~PagedFileManager()
{
}


RC PagedFileManager::createFile(const string &fileName)
{
	FILE *fp = fopen(fileName.c_str(), "rb+");
	if (fp != NULL) {
		fclose(fp);
		return -1;
	}
	else {
		fp = fopen(fileName.c_str(), "wb+");
		void *memory = malloc(PAGE_SIZE); 									// Initialize a virtual "first" page that could not
		memset(memory, 0, PAGE_SIZE);										// be accessed from outside to record file information
		string s = "sign"; 													// Mark the files that are created by calling this method
		string idx = "idx";
		string idxSign = "tree";
		int readCount = 0, writeCount = 0, appendCount = 0;					// Initial values of the three counters
		memcpy((char *)memory, s.c_str(), 4);
		memcpy((char *)memory + 4, &readCount, sizeof(int));
		memcpy((char *)memory + 8, &writeCount, sizeof(int));
		memcpy((char *)memory + 12, &appendCount, sizeof(int));
		// Write the catalog id into the virtual "first" page
		if (fileName == "Tables" || fileName == "Columns") {
			int ID = 0;
			memcpy((char *)memory + 16, &ID, sizeof(int));
		}
		if (fileName.find(idx) != string::npos) {							//flag of index file
			memcpy((char *)memory + 20, idxSign.c_str(), 4);
		}
		fwrite(memory, sizeof(char), PAGE_SIZE, fp);
		fclose(fp);
		free(memory);
		return 0;
	}
}

RC PagedFileManager::destroyFile(const string &fileName)
{
	FILE *fp = fopen(fileName.c_str(), "rb+");
	if (fp == NULL) {
		return -1;
	}
	else {
		void *memory = malloc(24);
		fread (memory, sizeof(char), 24, fp);
		string s = "";
		string idx = "idx";
		string idxSign = "";
		for (int i = 0; i < 4; i++) {
			char ch = *((char *) memory + i);
			s += ch;
		}
		if (s != "sign") {
			return -1;
		}
		if (fileName.find(idx) != string::npos) {							//flag of index file
			for (int i = 0; i < 4; i++) {
				char ch = *((char *) memory + 20 + i);
				idxSign += ch;
			}
			if (idxSign != "tree") {
				return -1;
			}
		}
		fclose(fp);
		remove(fileName.c_str());
		free(memory);
		return 0;
	}
}


RC PagedFileManager::openFile(const string &fileName, FileHandle &fileHandle)
{
	FILE *fp = fopen(fileName.c_str(), "rb+");
	if (fp != NULL) {
		fseek(fp, 0L, 2);
		if (ftell(fp) < PAGE_SIZE) {
			return -1;
		}
		rewind(fp);
		void *memory = malloc(24);
		fread (memory, sizeof(char), 24, fp);
		string s = "";
		string idx = "idx";
		string idxSign = "";
		for (int i = 0; i < 4; i++) {
			char ch = *((char *) memory + i);
			s += ch;
		}
		if (s != "sign") {
			return -1;
		}
		fileHandle.readPageCounter = *(int *)((char *)memory + 4);
		fileHandle.writePageCounter = *(int *)((char *)memory + 8);
		fileHandle.appendPageCounter = *(int *)((char *)memory + 12);
		if (fileName == "Tables" || fileName == "Columns") {
			fileHandle.id = *(int *)((char *)memory + 16);
		}
		if (fileName.find(idx) != string::npos) {
			for (int i = 0; i < 4; i++) {
				char ch = *((char *) memory + 20 + i);
				idxSign += ch;
			}
			if (idxSign != "tree") {
				return -1;
			}
		}
		fileHandle.fileName = fileName;
		fileHandle.fp = fp;
		free(memory);
		return 0;
	}
	else {
		return -1;
	}
}


RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
	if (fileHandle.fp == NULL) {
		return -1;
	}
	else {
		void *memory = malloc(16);
		memset(memory, 0, 16);
		int rc, wc, ac, catalogId, root;
		fileHandle.readRoot(root);
		rc = fileHandle.readPageCounter;
		wc = fileHandle.writePageCounter;
		ac = fileHandle.appendPageCounter;
		if (fileHandle.fileName == "Tables" || fileHandle.fileName == "Columns") {
			catalogId = fileHandle.id;
			memcpy((char *)memory + 12, &catalogId, sizeof(int));
		}
		else {
			memcpy((char *)memory + 12, &root, sizeof(int));
		}
		memcpy(memory, &rc, sizeof(int));
		memcpy((char *)memory + 4, &wc, sizeof(int));
		memcpy((char *)memory + 8, &ac, sizeof(int));
		fseek(fileHandle.fp, 4, 0);
		fwrite(memory, sizeof(char), 16, fileHandle.fp);					// Write the updated counters into the virtual page
		free(memory);
		rewind(fileHandle.fp);
		fclose(fileHandle.fp);
		fileHandle.fileName = "";
		fileHandle.fp = NULL;
		fileHandle.id = 0;
		return 0;
	}
}


FileHandle::FileHandle()
{
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
    fileName = "";
    fp = NULL;
    id = 0;
}


FileHandle::~FileHandle()
{
}


RC FileHandle::readPage(PageNum pageNum, void *data)
{
	if (fileName != "" && pageNum < this->getNumberOfPages()) {
		fseek(fp, (pageNum + 1) * PAGE_SIZE, 0);
	    fread(data, sizeof(char), PAGE_SIZE, fp);
	    readPageCounter++;
	    rewind(fp);
	    return 0;
	}
	else {
	   return -1;
	}
}


RC FileHandle::writePage(PageNum pageNum, const void *data)
{
	if (fileName != "" && pageNum < this->getNumberOfPages()) {
		fseek(fp, (pageNum + 1) * PAGE_SIZE, 0);
	    fwrite(data, sizeof(char), PAGE_SIZE, fp);
	    writePageCounter++;
	    rewind(fp);
	    return 0;
	}
	else {
	    return -1;
	}
}


RC FileHandle::appendPage(const void *data)
{
    if (fileName != "") {
    	fseek(fp, 0L, 2);
    	fwrite(data, sizeof(char), PAGE_SIZE, fp);
    	appendPageCounter++;
    	rewind(fp);
    	return 0;
    }
    else {
    	return -1;
    }
}


unsigned FileHandle::getNumberOfPages()
{
	if (fileName != "") {
	    fseek(fp, 0L, 2);
	    double numberOfBytes = (double) ftell(fp);
	    rewind(fp);
	    return ceil(numberOfBytes / PAGE_SIZE) - 1;							// Exclude the virtual "first" page
	}
	else {
	    return -1;
	}
}

RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
	readPageCount = this->readPageCounter;
	writePageCount = this->writePageCounter;
	appendPageCount = this->appendPageCounter;
    return 0;
}

RC FileHandle::updateRoot(int root)
{
	void *memory = malloc(PAGE_SIZE);
	memset(memory, 0, PAGE_SIZE);
	rewind(fp);
	fread(memory, sizeof(char), PAGE_SIZE, fp);
	rewind(fp);
	memcpy((char *)memory + 16, &root, sizeof(int));
	fwrite(memory, sizeof(char), PAGE_SIZE, fp);
	rewind(fp);
	free(memory);
	return 0;
}

RC FileHandle::readRoot(int &root)
{
	void *data = malloc(4);
	rewind(fp);
	fseek(fp, 16, 0);
	fread(data, sizeof(int), 1, fp);
	rewind(fp);
	root = *(int *)data;
	free(data);
	return 0;
}
