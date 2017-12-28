
#include "qe.h"
#include <cfloat>

bool Filter::compareByCondition(vector<Attribute> descriptor, void *data, const Condition &condition) {
	//data = nullIndicator + attributes (INT 4bytes, Real 4bytes, VarChar 4bytes Length + String)
	if (descriptor.empty()) {
		return false;
	}
	void *lValue;
	AttrType lType;
	int lFlag = -1;
	bool isLeftIthNull;
	void *rValue;
	AttrType rType;
	int rFlag = -1;
	bool isRightIthNull;

	unsigned int actualByte = ceil((double) descriptor.size() / 8);
	int offset = actualByte;
	int attrLength = 0;

	//from attributes to get the matched attribute with condition.rhsValue.data
	for (unsigned i = 0; i < descriptor.size(); i++) {
		bool nullFlag = filterIter->isIthAttributeNull(i, data);
		if (!nullFlag && descriptor[i].type == TypeInt) {
			attrLength = 4;
		}
		else if (!nullFlag && descriptor[i].type == TypeReal) {
			attrLength = 4;
		}
		else if (!nullFlag && descriptor[i].type == TypeVarChar) {
			int varCharLength;
			memcpy(&varCharLength, (char *)data + offset, 4);
			attrLength = 4 + varCharLength;
		}
		//get the ith attribute (match the left data)
		//eg. A.salary > 8000
		if (condition.lhsAttr.compare(descriptor[i].name) == 0) {
			isLeftIthNull = filterIter->isIthAttributeNull(i, data);
			lValue = (char *)data + offset;
			lType = descriptor[i].type;
			lFlag = 0;
		}
		//eg. A.salary > A.age
		else if (condition.bRhsIsAttr && condition.rhsAttr.compare(descriptor[i].name) == 0) {
			isRightIthNull = isIthAttributeNull(i, data);
			rValue = (char *)data + offset;
			rType = descriptor[i].type;
			rFlag = 0;
		}
		offset += attrLength;
	}
	if (lFlag == -1) {
		return -1;
	} else if (condition.bRhsIsAttr && rFlag == -1) {
		return -1;
	}

	if (!condition.bRhsIsAttr) {
		if (condition.rhsValue.data == NULL) {
			isRightIthNull = true;
		} else {
			isRightIthNull = false;
		}
		rValue = condition.rhsValue.data;
		rType = condition.rhsValue.type;
	}

    //cannot compare
	if (lType != rType) {
		return false;
	}

	// left value = NULL && right value = NULL when compOp is not no_condition
	if (isLeftIthNull && isRightIthNull && condition.op != NO_OP) {
		return false;
	}
	//compare the lValue with the rValue
	return compareTwoValue(lValue, condition.op, rValue, lType);
}

bool Iterator::isIthAttributeNull(int i, void *data) {
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

bool Filter::compareTwoValue(const void *lValue, CompOp op, const void *rValue, AttrType type) {
	if (type == TypeInt) {
		int lInt = *(int *)lValue;
		int rInt = *(int *)rValue;
		switch (op) {
			case EQ_OP://=
				return (lInt == rInt);
			case LT_OP://<
				return (lInt < rInt);
			case GT_OP://>
				return (lInt > rInt);
			case LE_OP://<=
				return (lInt <= rInt);
			case GE_OP://>=
				return (lInt >= rInt);
			case NE_OP://!=
				return (lInt != rInt);
			case NO_OP://no condition
				return true;
		}
	}
	else if (type == TypeReal) {
		float lFloat = *(float *)lValue;
		float rFloat = *(float *)rValue;
		switch (op) {
			case EQ_OP://=
				return (lFloat == rFloat);
			case LT_OP://<
				return (lFloat < rFloat);
			case GT_OP://>
				return (lFloat > rFloat);
			case LE_OP://<=
				return (lFloat <= rFloat);
			case GE_OP://>=
				return (lFloat >= rFloat);
			case NE_OP://!=
				return (lFloat != rFloat);
			case NO_OP://no condition
				return true;
		}
	}
	else if (type == TypeVarChar){
		int ll = *(int *)lValue;
		int lr = *(int *)rValue;
		string lstring = "";
		string rstring = "";
		for (int i = 0; i < ll; i++) {
			char ch = *(char *)lValue + 4 + i;
			lstring += ch;
		}
		for (int i = 0; i < lr; i++) {
			char ch = *(char *)rValue + 4 + i;
			rstring += ch;
		}
		switch (op) {
			case EQ_OP://=
				return (lstring == rstring);
			case LT_OP://<
				return (lstring < rstring);
			case GT_OP://>
				return (lstring > rstring);
			case LE_OP://<=
				return (lstring <= rstring);
			case GE_OP://>=
				return (lstring >= rstring);
			case NE_OP://!=
				return (lstring != rstring);
			case NO_OP://no condition
				return true;
		}
	}
	return false;
}

//Filter part
Filter::Filter(Iterator* input, const Condition &condition) {
	//call child's open()
	filterIter = input;
	filterCondition = condition;
	filterIter->getAttributes(filterAttributes);
}

Filter::~Filter() {
	//call child's close()
	//filterIter->~Iterator();
	filterAttributes.clear();
}

RC Filter::getNextTuple(void *data) {
	//data = nullIndicator + attributes (INT 4bytes, Real 4bytes, VarChar 4bytes Length + String)
	while (filterIter->getNextTuple(data) != -1) {
		if (compareByCondition(filterAttributes, data, filterCondition)) {
			return 0;
		}
	}
	return QE_EOF;
}

void Filter::getAttributes(vector<Attribute> &attrs) const{
	filterIter->getAttributes(attrs);
}

//Project part
Project::Project(Iterator *input, const vector<string> &attrNames) {
	//child's open()
	projectIter = input;
	vector<Attribute> attrs;
	projectIter->getAttributes(attrs);
	for (unsigned i = 0; i < attrs.size(); i++) {
		for (unsigned j = 0; j < attrNames.size(); j++) {
			if (attrs[i].name.compare(attrNames[j]) == 0) {
				projectionAttributes.push_back(attrs[i]);
			}
		}
	}
}

Project::~Project() {
	projectionAttributes.clear();
}

RC Project::getNextTuple(void *data) {
	void *tuple = malloc(PAGE_SIZE);
	while (projectIter->getNextTuple(tuple) != -1) {
		vector<Attribute> attrs;
		projectIter->getAttributes(attrs);
		projectAttributes(tuple, attrs, data, projectionAttributes);
		free(tuple);
		return 0;
	}
	free(tuple);
	return QE_EOF;
}

void Project::getAttributes(vector<Attribute> &attrs) const {
	for (unsigned i = 0; i < projectionAttributes.size(); i++) {
		attrs.push_back(projectionAttributes[i]);
	}
}

void Project::projectAttributes(const void *data, vector<Attribute> attrs,
								void *output, vector<Attribute> projectionAttributes) {
	unsigned int dataActualByte = ceil((double) attrs.size() / 8);
	unsigned int projectActualByte = ceil((double) projectionAttributes.size() / 8);
	bool isNull = false;
	char *dataCont;
	vector<unsigned int> dataNullsIndicator;
	vector<unsigned int> outputNullsIndicator;
	int dataOffset = 0, dataIndex = 0;

	//get the null indicator vector of data
	for (unsigned k = 0; k < dataActualByte; k++) {
		dataCont = (char *)data + dataOffset;
		char d = *dataCont;
		if ((d & 0x80) == 0x80)
			dataNullsIndicator.push_back(dataOffset * 8);
		if ((d & 0x40) == 0x40)
			dataNullsIndicator.push_back(dataOffset * 8 + 1);
		if ((d & 0x20) == 0x20)
			dataNullsIndicator.push_back(dataOffset * 8 + 2);
		if ((d & 0x10) == 0x10)
			dataNullsIndicator.push_back(dataOffset * 8 + 3);
		if ((d & 0x08) == 0x08)
			dataNullsIndicator.push_back(dataOffset * 8 + 4);
		if ((d & 0x04) == 0x04)
			dataNullsIndicator.push_back(dataOffset * 8 + 5);
		if ((d & 0x02) == 0x02)
			dataNullsIndicator.push_back(dataOffset * 8 + 6);
		if ((d & 0x01) == 0x01)
			dataNullsIndicator.push_back(dataOffset * 8 + 7);
		dataOffset += 1;
	}


	//copy data to output
	int pOffset = projectActualByte;
	int dOffset = dataActualByte;
	unsigned j = 0;
	for (unsigned i = 0; i < attrs.size(); i++) {
		//null
		if (!dataNullsIndicator.empty() && dataIndex < (int)dataNullsIndicator.size() && i == dataNullsIndicator[dataIndex]) {
			dataIndex ++;
			isNull = true;
		}

		if (j >= projectionAttributes.size()) {
			break;
		}
		//pointer method : if (attrs[i] matches), move the pointer of projectionAttributes
		if (attrs[i].name.compare(projectionAttributes[j].name) == 0) {
			if (isNull) {
				//set j'th of output as NULL
				outputNullsIndicator.push_back(j);
			} else {
				if (attrs[i].type == projectionAttributes[j].type) {
					if (attrs[i].type == TypeVarChar) {
						int varcharLength = *(int *)((char *)data + dOffset);
						memcpy((char *)output + pOffset, &varcharLength, 4);
						pOffset += 4;
						for (int i = 0; i < varcharLength; i++) {
							memcpy((char *)output + pOffset + i, (char *)data + dOffset + 4 + i, sizeof(char));
						}
						pOffset += varcharLength;
					}
					else {
						memcpy((char *)output + pOffset, (char *)data + dOffset, 4);
						pOffset += 4;
					}
				}
			}
			j++;
		}

		if (isNull == false) {
			if (attrs[i].type == TypeInt) {
				dOffset += 4;
			}
			else if (attrs[i].type == TypeReal) {
				dOffset += 4;
			}
			else {
				int varcharLength = *(int *)((char *)data + dOffset);
				dOffset += 4;
				dOffset += varcharLength;
			}
		}
		isNull = false;
	}

	char proj[projectActualByte];
	for (unsigned i = 0; i < projectActualByte; i++) {
		proj[i] = 0x00;
	}
	int projectOffset = 0;
	int bitOffset = 0;
	for (unsigned k = 0; k < outputNullsIndicator.size(); k++) {
		projectOffset = outputNullsIndicator[k] / 8;
		bitOffset = outputNullsIndicator[k] % 8;
		if (bitOffset == 0) {
			proj[projectOffset] = proj[projectOffset] | 0x80;
		}
		if (bitOffset == 1) {
			proj[projectOffset] = proj[projectOffset] | 0x40;
		}
		if (bitOffset == 2) {
			proj[projectOffset] = proj[projectOffset] | 0x20;
		}
		if (bitOffset == 3) {
			proj[projectOffset] = proj[projectOffset] | 0x10;
		}
		if (bitOffset == 4) {
			proj[projectOffset] = proj[projectOffset] | 0x08;
		}
		if (bitOffset == 5) {
			proj[projectOffset] = proj[projectOffset] | 0x04;
		}
		if (bitOffset == 6) {
			proj[projectOffset] = proj[projectOffset] | 0x02;
		}
		if (bitOffset == 7) {
			proj[projectOffset] = proj[projectOffset] | 0x01;
		}
	}
	int offset = 0;
	for (unsigned i = 0; i < projectActualByte; i++) {
		memcpy((char *)output + offset, &proj[i], sizeof(char));
		offset++;
	}
}

// BNLJoin Part
BNLJoin::BNLJoin(Iterator *leftIn, TableScan *rightIn, const Condition &condition, const unsigned numPages) {
	this->outputBuffer = malloc(PAGE_SIZE);
	memset(outputBuffer, 0, PAGE_SIZE);
	leftIn->getAttributes(attrLeft);
	rightIn->getAttributes(attrRight);
	this->leftActualByte = ceil((double)attrLeft.size() / 8);
	this->rightActualByte = ceil((double)attrRight.size() / 8);
	this->leftTable = leftIn;
	this->rightTable = rightIn;
	this->cond = condition;
	this->numPages = numPages;
	this->rest = false;
	this->index = 0;

	for (unsigned i = 0; i < attrLeft.size(); ++ i) {
		if (condition.lhsAttr == attrLeft[i].name) {
			this->type = attrLeft[i].type;
			break;
		}
	}
}

BNLJoin::~BNLJoin() {
	free(outputBuffer);
	this->intHash.clear();
	this->realHash.clear();
	this->varCharHash.clear();
}

RC BNLJoin::buildHash() {
	string tuple = "";
	if (type == TypeInt) {
		intHash.clear();
		void *record = malloc(PAGE_SIZE);
		void *attribute = malloc(4);
		int size, space = 0;
		while (space < (int)(numPages * PAGE_SIZE)) {
			if (leftTable->getNextTuple(record) != 0) {
				free(record);
				free(attribute);
				if (intHash.empty())
					return -1;
				return 0;
			}
			readAttribute(record, attrLeft, cond.lhsAttr, attribute, size);
			space += size;
			// Read out the key value
			int key = *(int *)attribute;
			// Copy out the whole data as the table value, cast it into string
			tuple.clear();
			for (int i = 0; i < size; ++ i) {
				tuple += *((char *)record + i);
			}
			// Construct the hash table
			intHash[key].push_back(tuple);
			memset(record, 0, PAGE_SIZE);
			memset(attribute, 0, 4);
		}
		free(record);
		free(attribute);
	}
	else if (type == TypeReal) {
		realHash.clear();
		void *record = malloc(PAGE_SIZE);
		void *attribute = malloc(4);
		int size = 0, space = 0;
		while (space < (int)(numPages * PAGE_SIZE)) {
			if (leftTable->getNextTuple(record) != 0) {
				free(record);
				free(attribute);
				if (realHash.empty())
					return -1;
				return 0;
			}
			readAttribute(record, attrLeft, cond.lhsAttr, attribute, size);
			space += size;
			// Read out the key value
			float key = *(float *)attribute;
			// Copy out the whole data as the table value, cast it into string
			tuple.clear();
			for (int i = 0; i < size; ++ i) {
				tuple += *((char *)record + i);
			}
			// Construct the hash table
			realHash[key].push_back(tuple);
			memset(record, 0, PAGE_SIZE);
			memset(attribute, 0, 4);
		}
		free(record);
		free(attribute);
	}
	else {
		varCharHash.clear();
		void *record = malloc(PAGE_SIZE);
		void *attribute = malloc(PAGE_SIZE);
		int size = 0, space = 0;
		while (space < (int)(numPages * PAGE_SIZE)) {
			if (leftTable->getNextTuple(record) != 0) {
				free(record);
				free(attribute);
				if (varCharHash.empty())
					return -1;
				return 0;
			}
			readAttribute(record, attrLeft, cond.lhsAttr, attribute, size);
			space += size;
			// Read out the key value
			int len = *(int *)attribute;
			string key = "";
			for (int i = 0; i < len; ++ i) {
				char ch = *((char *)attribute + 4 + i);
				key += ch;
			}
			// Copy out the whole data as the table value, cast it into string
			tuple.clear();
			for (int i = 0; i < size; ++ i) {
				tuple += *((char *)record + i);
			}
			// Construct the hash table
			varCharHash[key].push_back(tuple);
			memset(record, 0, PAGE_SIZE);
			memset(attribute, 0, PAGE_SIZE);
		}
		free(record);
		free(attribute);
	}
	return 0;
}

RC Iterator::readAttribute(void *record, vector<Attribute> attrs, string name, void *attribute, int &size) {
	int actualByte = ceil((double)attrs.size() / 8);
	size = actualByte;
	for (unsigned i = 0; i < attrs.size(); ++ i) {
		if (attrs[i].name == name) {
			if (type == TypeVarChar) {
				int length = 4 + *(int *)((char *)record + size);
				memcpy(attribute, (char *)record + size, length);
			}
			else {
				memcpy(attribute, (char *)record + size, 4);
			}
		}
		if (!isIthAttributeNull(i, record)) {
			if (attrs[i].type != TypeVarChar) {
				size += 4;
			}
			else {
				size += *(int *)((char *)record + size);
				size += 4;
			}
		}
	}
	return 0;
}

RC BNLJoin::outputJointRecord(void *jointRecord, int size) {
	short int flag = *(short int *)((char *)outputBuffer + PAGE_SIZE - 2);
	short int numberOfRecords = *(short int *)((char *)outputBuffer + PAGE_SIZE - 4);
	if (PAGE_SIZE - flag < size + 2) {
		return -1;
	}
	if (flag == 0)
		flag = 4;
	flag += size + 2;
	numberOfRecords ++;
	short int lastEndOffset = numberOfRecords == 0 ? 0 : *(short int *)((char *)outputBuffer + PAGE_SIZE - 2 * (numberOfRecords + 1));
	memcpy((char *)outputBuffer + lastEndOffset, jointRecord, size);
	lastEndOffset += size;
	memcpy((char *)outputBuffer + PAGE_SIZE - 2 * (numberOfRecords + 2), &lastEndOffset, 2);
	memcpy((char *)outputBuffer + PAGE_SIZE - 4, &numberOfRecords, 2);
	memcpy((char *)outputBuffer + PAGE_SIZE - 2, &flag, 2);
	return 0;
}

RC Iterator::joinNullsIndicator(void *nullsIndicator, void *left, void *right) {
	bitset<8> bit(0);
	memcpy(nullsIndicator, left, leftActualByte);
	if (attrLeft.size() % 8 == 0) {
		memcpy((char *)nullsIndicator + leftActualByte, right, rightActualByte);
		return 0;
	}
	char p = *((char *)nullsIndicator + leftActualByte - 1);
	char m = *((char *)right);

	for (int i = 0; i < 8; ++ i) {
		if (i < (int)attrLeft.size() % 8 && (bool)(p & (1 << (7 - i)))) {
			bit[7 - i] = 1;
		}
		else if (i >= (int)attrLeft.size() % 8 && (bool)(m & (1 << (7 + attrLeft.size() - i)))) {
			bit[7 - i] = 1;
		}
	}
	int n = bit.to_ulong();
	int offset = attrLeft.size() - 1;
	memset((char *)nullsIndicator + leftActualByte - 1, n, 1);
	for (int i = 0; i < rightActualByte - 1; ++ i) {
		bit.reset();
		p = m;
		m = *((char *)right + i + 1);
		for (int j = 0; j < 8; ++ j) {
			if (offset - j >= 0 && (bool)(p & (1 << (offset - j)))) {
				bit[7 - j] = 1;
			}
			else if ((bool)(m & (1 << (7 + attrLeft.size() - j)))) {
				bit[7 - j] = 1;
			}
		}
		n = bit.to_ulong();
		memset((char *)nullsIndicator + leftActualByte + i, n, 1);
	}
	return 0;
}

RC Iterator::joinTuples(void *leftTuple, void *rightTuple, int leftSize, int rightSize, void *jointRecord) {
	int actualByte = ceil((double)(attrLeft.size() + attrRight.size()) / 8);
	void *nullsIndicator = malloc(actualByte);
	memcpy(jointRecord, leftTuple, leftActualByte);
	joinNullsIndicator(nullsIndicator, jointRecord, rightTuple);
	memcpy(jointRecord, nullsIndicator, actualByte);
	memcpy((char *)jointRecord + actualByte, (char *)leftTuple + leftActualByte, leftSize - leftActualByte);
	memcpy((char *)jointRecord + actualByte + leftSize - leftActualByte, (char *)rightTuple + rightActualByte, rightSize - rightActualByte);
	free(nullsIndicator);
	return 0;
}


RC Iterator::printRecord(vector<Attribute> recordDescriptor, const void *data) {
	char *cont;
	vector<unsigned int> nullsIndicator;
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

RC BNLJoin::lookUpInHash(void *tuple) {
	int actualByte = ceil((double)(attrLeft.size() + attrRight.size()) / 8);
	if (type != TypeVarChar) {
		void *attribute = malloc(4);
		int leftSize, rightSize = 0;
		readAttribute(tuple, attrRight, cond.rhsAttr, attribute, rightSize);
		if ((type == TypeInt && intHash.find(*(int *)attribute) != intHash.end()) ||
			(type == TypeReal && realHash.find(*(float *)attribute) != realHash.end())) {
			vector<string> res = type == TypeInt ? intHash.find(*(int *)attribute)->second : realHash.find(*(float *)attribute)->second;
			for (unsigned i = 0; i < res.size(); ++ i) {
				leftSize = res[i].length();
				int jointRecordSize = actualByte + leftSize - leftActualByte + rightSize - rightActualByte;
				void *jointRecord = malloc(jointRecordSize);
				void *leftRecord = malloc(PAGE_SIZE);
				memcpy(leftRecord, res[i].c_str(), leftSize);
				joinTuples(leftRecord, tuple, leftSize, rightSize, jointRecord);
				if (outputJointRecord(jointRecord, jointRecordSize) != 0) {
					rest = true;
					string s = "";
					for (int j = 0; j < jointRecordSize; ++ j) {
						char ch = *((char *)jointRecord + j);
						s += ch;
					}
					restVector.push_back(s);
				}
				free(jointRecord);
				free(leftRecord);
			}
		}
		free(attribute);
	}
	else {
		void *attribute = malloc(PAGE_SIZE);
		int leftSize, rightSize = 0;
		readAttribute(tuple, attrRight, cond.rhsAttr, attribute, rightSize);
		string key = "";
		for (int i = 0; i < *(int *)attribute; ++ i) {
			char ch = *((char *)attribute + 4 + i);
			key += ch;
		}
		if (varCharHash.find(key) != varCharHash.end()) {
			vector<string> res = varCharHash.find(key)->second;
			for (unsigned i = 0; i < res.size(); ++ i) {
				leftSize = res[i].length();
				int jointRecordSize = actualByte + leftSize - leftActualByte + rightSize - rightActualByte;
				void *jointRecord = malloc(jointRecordSize);
				void *leftRecord = malloc(PAGE_SIZE);
				memcpy(leftRecord, res[i].c_str(), leftSize);
				joinTuples(leftRecord, tuple, leftSize, rightSize, jointRecord);
				if (outputJointRecord(jointRecord, jointRecordSize) != 0) {
					rest = true;
					string s = "";
					for (int j = 0; j < jointRecordSize; ++ j) {
						char ch = *((char *)jointRecord + j);
						s += ch;
					}
					restVector.push_back(s);
				}
				free(jointRecord);
				free(leftRecord);
			}
		}
		free(attribute);
	}
	if (rest) {
		return -1;
	}
	return 0;
}

RC BNLJoin::getNextTuple(void *data) {
	void *tuple1 = malloc(PAGE_SIZE);
	void *tuple2 = malloc(PAGE_SIZE);
	short int numberOfRecords = *(short int *)((char *)outputBuffer + PAGE_SIZE - 4);
	// If the outputBuffer has been completely traversed, we need to update the outputBuffer
	if (index >= numberOfRecords) {
		memset(outputBuffer, 0, PAGE_SIZE);
		index = 0;
		if (rest) {
			memcpy(data, restVector[0].c_str(), restVector[0].length());
			restVector.erase(restVector.begin());
			if (restVector.empty()) {
				rest = false;
			}
			free(tuple1);
			free(tuple2);
			return 0;
		}
		RC rc2 = rightTable->getNextTuple(tuple2);
		if (rc2 == 0) {
			// First time goes into this loop
			if (intHash.empty() && realHash.empty() && varCharHash.empty()) {
				buildHash();
			}
			while (lookUpInHash(tuple2) == 0) {
				memset(tuple2, 0, PAGE_SIZE);
				rc2 = rightTable->getNextTuple(tuple2);
				if (rc2 != 0) {
					if (buildHash() == 0) {
						rightTable->setIterator();
					}
					else {
						break;
					}
				}
			}
		}
		else {
			if (buildHash() == 0) {
				rightTable->setIterator();
				RC rc2 = rightTable->getNextTuple(tuple2);
				while (lookUpInHash(tuple2) == 0) {
					memset(tuple2, 0, PAGE_SIZE);
					rc2 = rightTable->getNextTuple(tuple2);
					if (rc2 != 0) {
						if (buildHash() == 0) {
							rightTable->setIterator();
						}
						else {
							break;
						}
					}
				}
			}
			else {
				free(tuple1);
				free(tuple2);
				return QE_EOF;
			}
		}
	}
	index ++;
	short int endOffset = *(short int *)((char *)outputBuffer + PAGE_SIZE - 2 * (2 + index));
	short int beginOffset = index == 1 ? 0 : *(short int *)((char *)outputBuffer + PAGE_SIZE - 2 * (1 + index));
	memcpy(data, (char *)outputBuffer + beginOffset, endOffset - beginOffset);

	free(tuple1);
	free(tuple2);
	return 0;
}

void BNLJoin::getAttributes(vector<Attribute> &attrs) const {
	// Combine the two attributes vectors together
	for (unsigned i = 0; i < attrLeft.size(); ++ i) {
		attrs.push_back(attrLeft[i]);
	}
	for (unsigned i = 0; i < attrRight.size(); ++ i) {
		attrs.push_back(attrRight[i]);
	}
}

// INLJoin part
INLJoin::INLJoin(Iterator *leftIn, IndexScan *rightIn, const Condition &condition) {
	this->leftTable = leftIn;
	this->rightTable = rightIn;
	this->cond = condition;
	leftIn->getAttributes(attrLeft);
	rightIn->getAttributes(attrRight);
	this->leftActualByte = ceil((double)attrLeft.size() / 8);
	this->rightActualByte = ceil((double)attrRight.size() / 8);
	this->index = 0;

	for (unsigned i = 0; i < attrLeft.size(); ++ i) {
		if (condition.lhsAttr == attrLeft[i].name) {
			this->type = attrLeft[i].type;
			break;
		}
	}
}

INLJoin::~INLJoin() {}

RC INLJoin::getNextTuple(void *data) {
	int actualByte = ceil((double)(attrLeft.size() + attrRight.size()) / 8);
	int leftSize, rightSize;
	while (outputRes.empty()) {
		void *tuple1 = malloc(PAGE_SIZE);
		void *tuple2 = malloc(PAGE_SIZE);
		void *attribute = malloc(PAGE_SIZE);
		if (leftTable->getNextTuple(tuple1) != 0) {
			return QE_EOF;
		}
		readAttribute(tuple1, attrLeft, cond.lhsAttr, attribute, leftSize);
		rightTable->setIterator(attribute, attribute, true, true);
		memset(attribute, 0, PAGE_SIZE);
		while(rightTable->getNextTuple(tuple2) == 0) {
			readAttribute(tuple2, attrRight, cond.rhsAttr, attribute, rightSize);
			int jointSize = leftSize + rightSize - leftActualByte - rightActualByte + actualByte;
			void *jointRecord = malloc(jointSize);
			joinTuples(tuple1, tuple2, leftSize, rightSize, jointRecord);
			string record = "";
			for (int i = 0; i < jointSize; ++ i) {
				record += *((char *)jointRecord + i);
			}
			outputRes.push_back(record);
			free(jointRecord);
			memset(tuple2, 0, PAGE_SIZE);
			memset(attribute, 0, PAGE_SIZE);
		}
		free(attribute);
		free(tuple1);
		free(tuple2);
	}
	if (!outputRes.empty()) {
		string jointRecord = outputRes[0];
		memcpy(data, jointRecord.c_str(), jointRecord.length());
		outputRes.erase(outputRes.begin());
	}
	return 0;
}

void INLJoin::getAttributes(vector<Attribute> &attrs) const {
	// Combine the two attributes vectors together
	for (unsigned i = 0; i < attrLeft.size(); ++ i) {
		attrs.push_back(attrLeft[i]);
	}
	for (unsigned i = 0; i < attrRight.size(); ++ i) {
		attrs.push_back(attrRight[i]);
	}
}

//Aggregate part
Aggregate::Aggregate(Iterator *input, Attribute aggAttr, AggregateOp op) {
	aggIter = input;
	this->aggAttr = aggAttr;
	aggType = aggAttr.type;
	this->op = op;
	this->isGroupBy = false;

	aggIter->getAttributes(aggAttrs);
}
Aggregate::~Aggregate() {
	index = 0;
    gIntMap.clear();
    gIntVector.clear();
    gFloatMap.clear();
    gFloatVector.clear();
    gStringMap.clear();
    gStringVector.clear();
    aggAttrs.clear();
}

void Aggregate::getAttributes(vector<Attribute> &attrs) const {
	attrs.push_back(aggAttr);
}

RC Aggregate::getNextTuple(void *data) {
	if (!isGroupBy) {
	    if (op == MIN){
	        return getMin(data);
	    }
	    else if (op == MAX) {
	        return getMax(data);
	    }
	    else if (op == COUNT) {
	        return getCount(data);
	    }
	    else if (op == SUM) {
	        return getSum(data);
	    }
	    else if (op == AVG) {
	        return getAvg(data);
	    }
	    return QE_EOF;
	}
	//group by
	else {
	    if (op == MIN){
	        return getGroupMin(data);
	    }
	    else if (op == MAX) {
	        return getGroupMax(data);
	    }
	    else if (op == COUNT) {
	        return getGroupCount(data);
	    }
	    else if (op == SUM) {
	        return getGroupSum(data);
	    }
	    else if (op == AVG) {
	        return getGroupAvg(data);
	    }
	    return QE_EOF;
	}
}

RC Aggregate::getMin(void *data) {
	AttrType type = aggAttr.type;
	float min = FLT_MAX;
	int attrPro;
	bool nullFlag;
	RC rc = QE_EOF;
	for (unsigned i = 0; i < aggAttrs.size(); i++) {
		if (aggAttrs[i].name == aggAttr.name) {
			attrPro = i;
		}
	}
	while (aggIter->getNextTuple(data) != QE_EOF) {
		rc = 0;
		void *target = malloc(PAGE_SIZE);
		int targetSize;
		nullFlag = aggIter->isIthAttributeNull(attrPro, data);
		aggIter->readAttribute(data, aggAttrs, aggAttr.name, target, targetSize);
		if (type == TypeInt) {
			if ((float)*(int *)target < min) {
				min = (float)*(int *)target;
			}
		}
		else {
			if (*(float *)target < min) {
				min = *(float *)target;
			}
		}
		free(target);
	}
	if (nullFlag) {
		char c = 0x80;
		memcpy(data, &c, sizeof(char));
	} else {
		if (type == TypeInt) {
			char c = 0x00;
			memcpy(data, &c, sizeof(char));
			memcpy((char *)data + 1, &min, sizeof(int));
		}
		else {
			char c = 0x00;
			memcpy(data, &c, sizeof(char));
			memcpy((char *)data + 1, &min, sizeof(float));
		}
	}
	return rc;
}
RC Aggregate::getMax(void *data) {
	AttrType type = aggAttr.type;
	float max = FLT_MIN;
	int attrPro;
	bool nullFlag;
	RC rc = QE_EOF;
	for (unsigned i = 0; i < aggAttrs.size(); i++) {
		if (aggAttrs[i].name == aggAttr.name) {
			attrPro = i;
		}
	}
	while (aggIter->getNextTuple(data) != QE_EOF) {
		rc = 0;
		void *target = malloc(PAGE_SIZE);
		int targetSize;
		nullFlag = aggIter->isIthAttributeNull(attrPro, data);
		aggIter->readAttribute(data, aggAttrs, aggAttr.name, target, targetSize);
		if (type == TypeInt) {
			if ((float)*(int *)target > max) {
				max = (float)*(int *)target;
			}
		}
		else {
			if (*(float *)target > max) {
				max = *(float *)target;
			}
		}
		free(target);
	}
	if (nullFlag) {
		char c = 0x80;
		memcpy(data, &c, sizeof(char));
	} else {
		if (type == TypeInt) {
			char c = 0x00;
			memcpy(data, &c, sizeof(char));
			memcpy((char *)data + 1, &max, sizeof(int));
		}
		else {
			char c = 0x00;
			memcpy(data, &c, sizeof(char));
			memcpy((char *)data + 1, &max, sizeof(float));
		}
	}
	return rc;
}
RC Aggregate::getCount(void *data) {
	AttrType type = aggAttr.type;
	float count = 0;
	int attrPro;
	bool nullFlag;
	RC rc = QE_EOF;
	for (unsigned i = 0; i < aggAttrs.size(); i++) {
		if (aggAttrs[i].name == aggAttr.name) {
			attrPro = i;
		}
	}
	while (aggIter->getNextTuple(data) != QE_EOF) {
		rc = 0;
		void *target = malloc(PAGE_SIZE);
		int targetSize;
		nullFlag = aggIter->isIthAttributeNull(attrPro, data);
		aggIter->readAttribute(data, aggAttrs, aggAttr.name, target, targetSize);
		free(target);
		count++;
	}
	if (nullFlag) {
		char c = 0x80;
		memcpy(data, &c, sizeof(char));
	} else {
		if (type == TypeInt) {
			char c = 0x00;
			memcpy(data, &c, sizeof(char));
			memcpy((char *)data + 1, &count, sizeof(int));
		}
		else {
			char c = 0x00;
			memcpy(data, &c, sizeof(char));
			memcpy((char *)data + 1, &count, sizeof(float));
		}
	}
	return rc;
}
RC Aggregate::getSum(void *data) {
	AttrType type = aggAttr.type;
	float sum = 0;
	int attrPro;
	bool nullFlag;
	RC rc = QE_EOF;
	for (unsigned i = 0; i < aggAttrs.size(); i++) {
		if (aggAttrs[i].name == aggAttr.name) {
			attrPro = i;
		}
	}
	while (aggIter->getNextTuple(data) != QE_EOF) {
		rc = 0;
		void *target = malloc(PAGE_SIZE);
		int targetSize;
		nullFlag = aggIter->isIthAttributeNull(attrPro, data);
		aggIter->readAttribute(data, aggAttrs, aggAttr.name, target, targetSize);
		if (type == TypeInt) {
			sum +=(float) *(int *)target;
		}
		else {
			sum += *(float *)target;
		}
		free(target);
	}
	if (nullFlag) {
		char c = 0x80;
		memcpy(data, &c, sizeof(char));
	} else {
		if (type == TypeInt) {
			char c = 0x00;
			memcpy(data, &c, sizeof(char));
			memcpy((char *)data + 1, &sum, sizeof(int));
		}
		else {
			char c = 0x00;
			memcpy(data, &c, sizeof(char));
			memcpy((char *)data + 1, &sum, sizeof(float));
		}
	}
	return rc;
}
RC Aggregate::getAvg(void *data) {
	AttrType type = aggAttr.type;
	int sumInt = 0;
	float sumFloat = 0.0;
	int count = 0;
	int attrPro;
	bool nullFlag;
	RC rc = QE_EOF;
	for (unsigned i = 0; i < aggAttrs.size(); i++) {
		if (aggAttrs[i].name == aggAttr.name) {
			attrPro = i;
		}
	}
	while (aggIter->getNextTuple(data) != QE_EOF) {
		rc = 0;
		void *target = malloc(PAGE_SIZE);
		int targetSize;
		nullFlag = aggIter->isIthAttributeNull(attrPro, data);
		aggIter->readAttribute(data, aggAttrs, aggAttr.name, target, targetSize);
		if (type == TypeInt) {
			sumInt += *(int *)target;
		}
		else {
			sumFloat += *(float *)target;
		}
		free(target);
		count++;
	}
	if (nullFlag) {
		char c = 0x80;
		memcpy(data, &c, sizeof(char));
	} else {
		if (type == TypeInt) {
			char c = 0x00;
			memcpy(data, &c, sizeof(char));
			float avg = (float) sumInt / count;
			memcpy((char *)data + 1, &avg, sizeof(int));
		}
		else {
			char c = 0x00;
			memcpy(data, &c, sizeof(char));
			float avg = sumFloat / count;
			memcpy((char *)data + 1, &avg, sizeof(float));
		}
	}
	return rc;
}

////////////////////////////////
//Group-based hash aggregation//
////////////////////////////////
Aggregate::Aggregate(Iterator *input, Attribute aggAttr, Attribute groupAttr, AggregateOp op) {
	aggIter = input;
	this->aggAttr = aggAttr;
	aggType = aggAttr.type;
	this->groupAttr = groupAttr;
	this->op = op;
	this->isGroupBy = true;
	aggIter->getAttributes(aggAttrs);
	index = 0;
}

RC Aggregate::getGroupMin(void *data) {
	if (groupAttr.type == TypeInt) {
		if (aggType == TypeInt) {
			void *tuple = malloc(PAGE_SIZE);
			while (aggIter->getNextTuple(tuple) != QE_EOF) {
				//RC readAttribute(void *record, vector<Attribute> attrs, string name, void *attribute, int &size);
				//aggAttrs
				int size;
				void *lKey = malloc(4);
				readAttribute(tuple, aggAttrs, groupAttr.name, lKey, size);
				void *rKey = malloc(4);
				readAttribute(tuple, aggAttrs, aggAttr.name, rKey, size);
				if (gIntMap.find(*(int *)lKey) == gIntMap.end()) {
					gIntMap.insert(std::pair<int,float>(*(int *)lKey, (float)*(int *)rKey));
					gIntVector.push_back(*(int *)lKey);
				} else {
					if (gIntMap[*(int *)lKey] > (float)*(int *)rKey) {
						gIntMap[*(int *)lKey] = (float)*(int *)rKey;
					}
				}
				free(rKey);
				free(lKey);
			}
			free(tuple);
			//change data
			if (index < (int)gIntVector.size()) {
				char c = 0x00;
				memcpy(data, &c, sizeof(char));
				memcpy((char *)data + 1, &gIntVector[index], sizeof(int));
				memcpy((char *)data + 5, &gIntMap[gIntVector[index]], sizeof(float));
				index++;
				return 0;
			} else {
				return QE_EOF;
			}
		}
		else if (aggType == TypeReal) {
			void *tuple = malloc(PAGE_SIZE);
			while (aggIter->getNextTuple(tuple) != QE_EOF) {
				//aggAttrs
				int size;
				void *lKey = malloc(4);
				readAttribute(tuple, aggAttrs, groupAttr.name, lKey, size);
				void *rKey = malloc(4);
				readAttribute(tuple, aggAttrs, aggAttr.name, rKey, size);
				if (gIntMap.find(*(int *)lKey) == gIntMap.end()) {
					gIntMap.insert(std::pair<int,float>(*(int *)lKey, *(float *)rKey));
					gIntVector.push_back(*(int *)lKey);
				} else {
					if (gIntMap[*(int *)lKey] > *(float *)rKey) {
						gIntMap[*(int *)lKey] = *(float *)rKey;
					}
				}
				free(rKey);
				free(lKey);
			}
			free(tuple);
			//change data
			if (index < (int)gIntVector.size()) {
				char c = 0x00;
				memcpy(data, &c, sizeof(char));
				memcpy((char *)data + 1, &gIntVector[index], sizeof(int));
				memcpy((char *)data + 5, &gIntMap[gIntVector[index]], sizeof(float));
				index++;
				return 0;
			} else {
				return QE_EOF;
			}
		}
	}
	else if (groupAttr.type == TypeReal) {
		if (aggType == TypeInt) {
			void *tuple = malloc(PAGE_SIZE);
			while (aggIter->getNextTuple(tuple) != QE_EOF) {
				//RC readAttribute(void *record, vector<Attribute> attrs, string name, void *attribute, int &size);
				//aggAttrs
				int size;
				void *lKey = malloc(4);
				readAttribute(tuple, aggAttrs, groupAttr.name, lKey, size);
				void *rKey = malloc(4);
				readAttribute(tuple, aggAttrs, aggAttr.name, rKey, size);
				if (gFloatMap.find(*(float *)lKey) == gFloatMap.end()) {
					gFloatMap.insert(std::pair<float,float>(*(float *)lKey, (float)*(int *)rKey));
					gFloatVector.push_back(*(float *)lKey);
				} else {
					if (gFloatMap[*(float *)lKey] > (float)*(int *)rKey) {
						gFloatMap[*(float *)lKey] = (float)*(int *)rKey;
					}
				}
				free(rKey);
				free(lKey);
			}
			free(tuple);
			//change data
			if (index < (int)gFloatVector.size()) {
				char c = 0x00;
				memcpy(data, &c, sizeof(char));
				memcpy((char *)data + 1, &gFloatVector[index], sizeof(int));
				memcpy((char *)data + 5, &gFloatMap[gFloatVector[index]], sizeof(float));
				index++;
				return 0;
			} else {
				return QE_EOF;
			}
		}
		else if (aggType == TypeReal) {
			void *tuple = malloc(PAGE_SIZE);
			while (aggIter->getNextTuple(tuple) != QE_EOF) {
				//RC readAttribute(void *record, vector<Attribute> attrs, string name, void *attribute, int &size);
				//aggAttrs
				int size;
				void *lKey = malloc(4);
				readAttribute(tuple, aggAttrs, groupAttr.name, lKey, size);
				void *rKey = malloc(4);
				readAttribute(tuple, aggAttrs, aggAttr.name, rKey, size);
				if (gFloatMap.find(*(float *)lKey) == gFloatMap.end()) {
					gFloatMap.insert(std::pair<float,float>(*(float *)lKey, *(float *)rKey));
					gFloatVector.push_back(*(float *)lKey);
				} else {
					if (gFloatMap[*(float *)lKey] > *(float *)rKey) {
						gFloatMap[*(float *)lKey] = *(float *)rKey;
					}
				}
				free(rKey);
				free(lKey);
			}
			free(tuple);
			//change data
			if (index < (int)gFloatVector.size()) {
				char c = 0x00;
				memcpy(data, &c, sizeof(char));
				memcpy((char *)data + 1, &gFloatVector[index], sizeof(float));
				memcpy((char *)data + 5, &gFloatMap[gFloatVector[index]], sizeof(float));
				index++;
				return 0;
			} else {
				return QE_EOF;
			}
		}
	}
	else if (groupAttr.type == TypeVarChar) {
		if (aggType == TypeInt) {

		}
		else if (aggType == TypeReal) {

		}
	}
	return 0;
}

RC Aggregate::getGroupMax(void *data) {
	if (groupAttr.type == TypeInt) {
		if (aggType == TypeInt) {

		}
		else if (aggType == TypeReal) {
			void *tuple = malloc(PAGE_SIZE);
			while (aggIter->getNextTuple(tuple) != QE_EOF) {
				int size;
				void *lKey = malloc(4);
				readAttribute(tuple, aggAttrs, groupAttr.name, lKey, size);
				void *rKey = malloc(4);
				readAttribute(tuple, aggAttrs, aggAttr.name, rKey, size);
				if (gIntMap.find(*(int *)lKey) == gIntMap.end()) {
					gIntMap.insert(std::pair<int,float>(*(int *)lKey, *(float *)rKey));
					gIntVector.push_back(*(int *)lKey);
				} else {
					if (gIntMap[*(int *)lKey] < *(float *)rKey) {
						gIntMap[*(int *)lKey] = *(float *)rKey;
					}
				}
				free(rKey);
				free(lKey);
			}
			free(tuple);
			//change data
			if (index < (int)gIntVector.size()) {
				char c = 0x00;
				memcpy(data, &c, sizeof(char));
				memcpy((char *)data + 1, &gIntVector[index], sizeof(int));
				memcpy((char *)data + 5, &gIntMap[gIntVector[index]], sizeof(float));
				index++;
				return 0;
			} else {
				return QE_EOF;
			}
		}
	}
	else if (groupAttr.type == TypeReal) {
		
	}
	else if (groupAttr.type == TypeVarChar) {
		
	}
	return 0;
}

RC Aggregate::getGroupCount(void *data) {
	if (groupAttr.type == TypeInt) {
		if (aggType == TypeInt) {
			void *tuple = malloc(PAGE_SIZE);
			while (aggIter->getNextTuple(tuple) != QE_EOF) {
				//RC readAttribute(void *record, vector<Attribute> attrs, string name, void *attribute, int &size);
				//aggAttrs
				int size;
				void *lKey = malloc(4);
				readAttribute(tuple, aggAttrs, groupAttr.name, lKey, size);
				void *rKey = malloc(4);
				readAttribute(tuple, aggAttrs, aggAttr.name, rKey, size);
				if (gIntMap.find(*(int *)lKey) == gIntMap.end()) {
					gIntMap.insert(std::pair<int,float>(*(int *)lKey, (float)1));
					gIntVector.push_back(*(int *)lKey);
				} else {
					gIntMap[*(int *)lKey]++;
				}
				free(rKey);
				free(lKey);
			}
			free(tuple);
			//change data
			if (index < (int)gIntVector.size()) {
				char c = 0x00;
				memcpy(data, &c, sizeof(char));
				memcpy((char *)data + 1, &gIntVector[index], sizeof(int));
				memcpy((char *)data + 5, &gIntMap[gIntVector[index]], sizeof(float));
				index++;
				return 0;
			} else {
				return QE_EOF;
			}
		}
		else if (aggType == TypeReal) {
			
		}
	}
	else if (groupAttr.type == TypeReal) {
		
	}
	else if (groupAttr.type == TypeVarChar) {
		
	}
	return 0;
}

RC Aggregate::getGroupSum(void *data) {
	if (groupAttr.type == TypeInt) {
		if (aggType == TypeInt) {
			void *tuple = malloc(PAGE_SIZE);
			while (aggIter->getNextTuple(tuple) != QE_EOF) {
				int size;
				void *lKey = malloc(4);
				readAttribute(tuple, aggAttrs, groupAttr.name, lKey, size);
				void *rKey = malloc(4);
				readAttribute(tuple, aggAttrs, aggAttr.name, rKey, size);
				if (gIntMap.find(*(int *)lKey) == gIntMap.end()) {
					gIntMap.insert(std::pair<int,float>(*(int *)lKey, (float)*(int *)rKey));
					gIntVector.push_back(*(int *)lKey);
				} else {
					gIntMap[*(int *)lKey] += (float)*(int *)rKey;
				}
				free(rKey);
				free(lKey);
			}
			free(tuple);
			//change data
			if (index < (int)gIntVector.size()) {
				char c = 0x00;
				memcpy(data, &c, sizeof(char));
				memcpy((char *)data + 1, &gIntVector[index], sizeof(int));
				memcpy((char *)data + 5, &gIntMap[gIntVector[index]], sizeof(float));
				index++;
				return 0;
			} else {
				return QE_EOF;
			}
		}
		else if (aggType == TypeReal) {

		}
	}
	else if (groupAttr.type == TypeReal) {

	}
	else {

	}
	return 0;
}

RC Aggregate::getGroupAvg(void *data) {
	return 0;
}

// GHJoin part
GHJoin::GHJoin(Iterator *leftIn, Iterator *rightIn, const Condition &condition, const unsigned numPartitions) {
	this->leftTable = leftIn;
	this->rightTable = rightIn;
	this->cond = condition;
	this->numPartitions = numPartitions;
	leftIn->getAttributes(attrLeft);
	rightIn->getAttributes(attrRight);
	this->leftActualByte = ceil((double)attrLeft.size() / 8);
	this->rightActualByte = ceil((double)attrRight.size() / 8);
	this->index = 0;

	for (unsigned i = 0; i < attrLeft.size(); ++ i) {
		if (condition.lhsAttr == attrLeft[i].name) {
			this->type = attrLeft[i].type;
			break;
		}
	}
}

GHJoin::~GHJoin() {}

int GHJoin::hashFunction(void *attribute) {
	if (type == TypeInt) {
		int att = *(int *)attribute;
		return att % numPartitions;
	}
	else if (type ==TypeReal) {
		float att = *(float *)attribute;
		int n = ceil((double)att);
		return n % numPartitions;
	}
	else {
		int len = *(int *)attribute;
		return len % numPartitions;
	}
}

RC GHJoin::partitionRecord(void *record, void *inputBuffer, int bucket, bool leftOrRight) {
	void *memory = malloc(PAGE_SIZE);
	memset(memory, 0, PAGE_SIZE);
	void *data = malloc(PAGE_SIZE);
	short int size;
	memcpy(memory, (char *)inputBuffer + bucket * PAGE_SIZE, PAGE_SIZE);
	short int flag = *(short int *)((char *)memory + PAGE_SIZE - 2);
	short int numberOfRecords = *(short int *)((char *)memory + PAGE_SIZE - 4);
	if (leftOrRight) {
		_rbf_manager->recordConverter(attrLeft, record, data, size);
	}
	else {
		_rbf_manager->recordConverter(attrRight, record, data, size);
	}
	// The page is full, then flush it to disk
	if (flag < size + 4) {
		FileHandle fileHandle;
		string fileName;
		if (leftOrRight) {
			fileName = "left_join" + to_string(bucket + 1) + "_" + to_string(suffix);
		}
		else {
			fileName = "right_join" + to_string(bucket + 1) + "_" + to_string(suffix);
		}
		_rbf_manager->openFile(fileName, fileHandle);
		fileHandle.appendPage(memory);
		_rbf_manager->closeFile(fileHandle);
		memset(memory, 0, PAGE_SIZE);
		flag = 4092;
		numberOfRecords = 0;
	}

	flag -= size + 4;
	numberOfRecords ++;
	short int beginOffset = numberOfRecords == 1 ? 0 : *(short int *)((char *)memory + PAGE_SIZE - numberOfRecords * 4) +
							                           *(short int *)((char *)memory + PAGE_SIZE - numberOfRecords * 4 + 2);
	memcpy((char *)memory + beginOffset, data, size);
	memcpy((char *)memory + PAGE_SIZE - (numberOfRecords + 1) * 4, &beginOffset, 2);
	memcpy((char *)memory + PAGE_SIZE - (numberOfRecords + 1) * 4 + 2, &size, 2);
	memcpy((char *)memory + PAGE_SIZE - 4, &numberOfRecords, 2);
	memcpy((char *)memory + PAGE_SIZE - 2, &flag, 2);
	memcpy((char *)inputBuffer + bucket * PAGE_SIZE, memory, PAGE_SIZE);

	free(memory);
	free(data);
	return 0;
}

RC GHJoin::constructPartitions() {
	int suffix = 1;
	int partition = 1;
	string fileName = "left_join1_" + to_string(suffix);
	// Find a proper file name
	while (_rbf_manager->createFile(fileName) != 0) {
		suffix ++;
		fileName = "left_join1_" + to_string(suffix);
	}
	this->suffix = suffix;
	// Create partition files
	_rbf_manager->createFile("right_join1_" + to_string(suffix));
	while (partition < (int)numPartitions) {
		partition ++;
		fileName = "left_join" + to_string(partition) + '_' + to_string(suffix);
		_rbf_manager->createFile(fileName);
		fileName = "right_join" + to_string(partition) + '_' + to_string(suffix);
		_rbf_manager->createFile(fileName);
	}
	int bucket, size;
	short int flag = 4092;
	string name = cond.lhsAttr;
	void *record = malloc(PAGE_SIZE);
	void *attribute = malloc(PAGE_SIZE);
	void *inputBuffer = malloc(numPartitions * PAGE_SIZE);

	memset(inputBuffer, 0, numPartitions * PAGE_SIZE);
	for (unsigned i = 0; i < numPartitions; ++ i) {
		memcpy((char *)inputBuffer + (i + 1) * PAGE_SIZE - 2, &flag, 2);
	}
	// Construct the left partitions
	while (leftTable->getNextTuple(record) == 0) {
		readAttribute(record, attrLeft, name, attribute, size);
		bucket = hashFunction(attribute);
		partitionRecord(record, inputBuffer, bucket, true);
	}
	for (unsigned i = 0; i < numPartitions; ++ i) {
		void *memory = malloc(PAGE_SIZE);
		memset(memory, 0, PAGE_SIZE);
		FileHandle fileHandle;
		string fileName = "left_join" + to_string(i + 1) + '_' + to_string(suffix);
		_rbf_manager->openFile(fileName, fileHandle);
		memcpy(memory, (char *)inputBuffer + i * PAGE_SIZE, PAGE_SIZE);
		if (*(short int *)((char *)memory + PAGE_SIZE - 2) != 4092) {
			fileHandle.appendPage(memory);
		}
		_rbf_manager->closeFile(fileHandle);
		free(memory);
	}

	memset(inputBuffer, 0, numPartitions * PAGE_SIZE);
	for (unsigned i = 0; i < numPartitions; ++ i) {
		memcpy((char *)inputBuffer + (i + 1) * PAGE_SIZE - 2, &flag, 2);
	}
	// Construct the right partitions
	name = cond.rhsAttr;
	while (rightTable->getNextTuple(record) == 0) {
		readAttribute(record, attrRight, name, attribute, size);
		bucket = hashFunction(attribute);
		partitionRecord(record, inputBuffer, bucket, false);
	}
	for (unsigned i = 0; i < numPartitions; ++ i) {
		void *memory = malloc(PAGE_SIZE);
		memset(memory, 0, PAGE_SIZE);
		FileHandle fileHandle;
		string fileName = "right_join" + to_string(i + 1) + '_' + to_string(suffix);
		_rbf_manager->openFile(fileName, fileHandle);
		memcpy(memory, (char *)inputBuffer + i * PAGE_SIZE, PAGE_SIZE);
		if (*(short int *)((char *)memory + PAGE_SIZE - 2) != 4092) {
			fileHandle.appendPage(memory);
		}
		_rbf_manager->closeFile(fileHandle);
		free(memory);
	}

	free(record);
	free(attribute);
	free(inputBuffer);
	return 0;
}

RC GHJoin::buildHash() {
	intHash.clear();
	realHash.clear();
	varCharHash.clear();
	FileHandle fileHandle;
	RID rid;
	string fileName = "left_join" + to_string(index + 1) + '_' + to_string(suffix);
	_rbf_manager->openFile(fileName, fileHandle);
	RBFM_ScanIterator rbfm_ScanIterator;
	void *data = malloc(PAGE_SIZE);
	void *attribute = malloc(PAGE_SIZE);
	int size;
	vector<string> attrNames;
	for (unsigned i = 0; i < attrLeft.size(); ++ i) {
		attrNames.push_back(attrLeft[i].name);
	}
	_rbf_manager->scan(fileHandle, attrLeft, cond.lhsAttr, NO_OP, NULL, attrNames, rbfm_ScanIterator);
	while (rbfm_ScanIterator.getNextRecord(rid, data) == 0) {
		readAttribute(data, attrLeft, cond.lhsAttr, attribute, size);
		string record = "";
		if (type == TypeInt) {
			int key = *(int *)attribute;
			for (int i = 0; i < size; ++ i) {
				record += *((char *)data + i);
			}
			intHash[key].push_back(record);
		}
		else if (type == TypeReal) {
			float key = *(float *)attribute;
			for (int i = 0; i < size; ++ i) {
				record += *((char *)data + i);
			}
			realHash[key].push_back(record);
		}
		else {
			string key = "";
			for (int i = 0; i < *(int *)attribute; ++ i) {
				key += *((char *)attribute + 4 + i);
			}
			for (int i = 0; i < size; ++ i) {
				record += *((char *)data + i);
			}
			varCharHash[key].push_back(record);
		}
	}
	_rbf_manager->closeFile(fileHandle);

	free(data);
	return 0;
}

RC GHJoin:: lookUpInHash(void *record) {
	int actualByte = ceil((double)(attrLeft.size() + attrRight.size()) / 8);
	if (type != TypeVarChar) {
		void *attribute = malloc(4);
		int leftSize, rightSize = 0;
		readAttribute(record, attrRight, cond.rhsAttr, attribute, rightSize);
		if ((type == TypeInt && intHash.find(*(int *)attribute) != intHash.end()) ||
			(type == TypeReal && realHash.find(*(float *)attribute) != realHash.end())) {
			vector<string> res = type == TypeInt ? intHash.find(*(int *)attribute)->second : realHash.find(*(float *)attribute)->second;
			for (unsigned i = 0; i < res.size(); ++ i) {
				leftSize = res[i].length();
				int jointRecordSize = actualByte + leftSize - leftActualByte + rightSize - rightActualByte;
				void *jointRecord = malloc(jointRecordSize);
				void *leftRecord = malloc(PAGE_SIZE);
				memcpy(leftRecord, res[i].c_str(), leftSize);
				joinTuples(leftRecord, record, leftSize, rightSize, jointRecord);
				string s = "";
				for (int j = 0; j < jointRecordSize; ++ j) {
					char ch = *((char *)jointRecord + j);
					s += ch;
				}
				outputRes.push_back(s);
				free(jointRecord);
				free(leftRecord);
			}
		}
		free(attribute);
	}
	else {
		void *attribute = malloc(PAGE_SIZE);
		int leftSize, rightSize = 0;
		readAttribute(record, attrRight, cond.rhsAttr, attribute, rightSize);
		string key = "";
		for (int i = 0; i < *(int *)attribute; ++ i) {
			char ch = *((char *)attribute + 4 + i);
			key += ch;
		}
		if (varCharHash.find(key) != varCharHash.end()) {
			vector<string> res = varCharHash.find(key)->second;
			for (unsigned i = 0; i < res.size(); ++ i) {
				leftSize = res[i].length();
				int jointRecordSize = actualByte + leftSize - leftActualByte + rightSize - rightActualByte;
				void *jointRecord = malloc(jointRecordSize);
				void *leftRecord = malloc(PAGE_SIZE);
				memcpy(leftRecord, res[i].c_str(), leftSize);
				joinTuples(leftRecord, record, leftSize, rightSize, jointRecord);
				string s = "";
				for (int j = 0; j < jointRecordSize; ++ j) {
					char ch = *((char *)jointRecord + j);
					s += ch;
				}
				outputRes.push_back(s);
				free(jointRecord);
				free(leftRecord);
			}
		}
		free(attribute);
	}
	return 0;
}

RC GHJoin::getNextTuple(void *data) {
	if (index == 0) {
		constructPartitions();
	}
	RID rid;
	vector<string> attrNames;
	for (unsigned i = 0; i < attrRight.size(); ++ i) {
		attrNames.push_back(attrRight[i].name);
	}
	void *record = malloc(PAGE_SIZE);
	while (right_ScanIterator.getNextRecord(rid, record) != 0) {
		if (index == (int)numPartitions) {
			// Delete all the intermediate partition files
			for (unsigned i = 0; i < numPartitions; ++ i) {
				_rbf_manager->destroyFile("left_join" + to_string(i + 1) + '_' + to_string(suffix));
				_rbf_manager->destroyFile("right_join" + to_string(i + 1) + '_' + to_string(suffix));
			}
			return QE_EOF;
		}
		buildHash();
		FileHandle fileHandle;
		memset(record, 0, PAGE_SIZE);
		string fileName = "right_join" + to_string(index + 1) + '_' + to_string(suffix);
		_rbf_manager->openFile(fileName, fileHandle);
		_rbf_manager->scan(fileHandle, attrRight, cond.lhsAttr, NO_OP, NULL, attrNames, right_ScanIterator);
		_rbf_manager->closeFile(fileHandle);
		index ++;
	}
	while (outputRes.empty()) {
		lookUpInHash(record); 
		if (!outputRes.empty()) {
			break;
		}
		memset(record, 0, PAGE_SIZE);
		while (right_ScanIterator.getNextRecord(rid, record) != 0) {
			if (index == (int)numPartitions) {
				// Delete all the intermediate partition files
				for (unsigned i = 0; i < numPartitions; ++ i) {
					_rbf_manager->destroyFile("left_join" + to_string(i + 1) + '_' + to_string(suffix));
					_rbf_manager->destroyFile("right_join" + to_string(i + 1) + '_' + to_string(suffix));
				}
				return QE_EOF;
			}
			buildHash();
			FileHandle fileHandle;
			memset(record, 0, PAGE_SIZE);
			string fileName = "right_join" + to_string(index + 1) + '_' + to_string(suffix);
			_rbf_manager->openFile(fileName, fileHandle);
			_rbf_manager->scan(fileHandle, attrRight, cond.lhsAttr, NO_OP, NULL, attrNames, right_ScanIterator);
			_rbf_manager->closeFile(fileHandle);
			index ++;
		}
	}
	if (!outputRes.empty()) {
		string jointRecord = outputRes[0];
		memcpy(data, jointRecord.c_str(), jointRecord.length());
		outputRes.erase(outputRes.begin());
	}
	free(record);
	return 0;
}

void GHJoin::getAttributes(vector<Attribute> &attrs) const {
	// Combine the two attributes vectors together
	for (unsigned i = 0; i < attrLeft.size(); ++ i) {
		attrs.push_back(attrLeft[i]);
	}
	for (unsigned i = 0; i < attrRight.size(); ++ i) {
		attrs.push_back(attrRight[i]);
	}
}
