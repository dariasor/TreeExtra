//INDdata.cpp: implementation of INDdata class
//
// (c) Daria Sorokina

#include "INDdata.h"
#include "functions.h"
#include "LogStream.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>


//Downloads the data (train, validation, test sets) into memory following specifications 
//in the attr file.
//filenames may be empty strings, if correspondent data is not provided
INDdata::INDdata(const char* trainFName, const char* validFName, const char* testFName,
	const char* attrFName, bool doOut)
{
	LogStream telog;

	//read attr file, collect info about boolean attributes and attrN
	telog << "Reading the attribute file: \"" << attrFName << "\"\n";
	fstream fattr;
	fattr.open(attrFName, ios_base::in);
	if(!fattr) 
		throw OPEN_ATTR_ERR;

	char buf[LINE_LEN];	//buffer for reading from input files
	getLineExt(fattr, buf);

	//read list of attributes, collect information about them
	int attrId, colNo; // counters
	bool foundClass = false;	//response found flag
	weightColNo = -1;
	for(attrId = 0, colNo = 0; fattr.gcount(); attrId++, colNo++)
	{
		string attrStr(buf);	//a line of an attr file (corresponds to 1 attribute)
		
		//check for response attribute
		if(attrStr.find("(class)") != string::npos)	
		{
			if(foundClass)
				throw MULT_CLASS_ERR;

			tarColNo = colNo;
			attrId--;
			foundClass = true;

			string::size_type nameLen = attrStr.find(":");
			tarName = attrStr.substr(0, nameLen);

			getLineExt(fattr, buf);
			continue;
		}
		if(attrStr.find("(weight)") != string::npos)	
		{
			weightColNo = colNo;
			attrId--;

			getLineExt(fattr, buf);
			continue;
		}

		//parse attr name
		string::size_type nameLen = attrStr.find(":");
		if((attrStr.find("contexts") != -1) || (nameLen == -1)) 
			break; //end of listed attributes
		string attrName = attrStr.substr(0, nameLen);
		if(attrName.find_first_of("\\/*?\"<>|:") != string::npos)
			throw ATTR_NAME_DEF_ERR;
		if(isIn(attrNames, attrName))
		{
			clog << "\n" << attrName;
			clog.flush();
			throw DUPLICATE_ATTRIBUTES_ERR;
		}
		attrNames.push_back(trimSpace(attrName));
		aIdToColNo.push_back(colNo);

		//parse attr type
		string::size_type endType = attrStr.find(".");
		string typeStr = attrStr.substr(nameLen + 1, endType - nameLen - 1);
		typeStr = trimSpace(typeStr);
		if(typeStr.compare("nom") == 0)
		{
			nomAttrs.insert(attrId);
		} else {
			if(typeStr.compare("0,1") == 0)
				boolAttrs.insert(attrId);
			else if(attrStr.find("cont") == string::npos) 
				throw ATTR_TYPE_ERR;
		}

		getLineExt(fattr, buf);
	}
	attrN = attrId;
	colN = colNo;
	if(!foundClass)
		throw NO_CLASS_ERR;
	
	//read contexts part (if any), add unused attributes into ignoreattrs
	while(fattr.gcount())
	{
		string attrStr(buf);
		if(attrStr.find(" never") != string::npos)
		{//extract name of the attribute, find its number, insert it into ignoreattrs
			int nameLen = (int)attrStr.find(" ");
			string attrName = attrStr.substr(0, nameLen);
			attrName = trimSpace(attrName);
			int neverAttrId = getAttrId(attrName);
			if (neverAttrId == -1)
				telog << "\nWARNING: trying to exclude \"" << attrName << "\" - this is not a valid feature\n\n";
			else
				ignoreAttrs.insert(neverAttrId);
		}
		getLineExt(fattr, buf);
	}
	fattr.close();
	
	int activeAttrN = attrN - (int)ignoreAttrs.size();
	telog << attrN << " attributes\n" << activeAttrN << " active attributes\n\n";
	
	//check for active nominals (not allowed)
	bipair isSub = isSubset(nomAttrs, ignoreAttrs);
	if(!isSub.first)
	{
		cerr << getAttrName(isSub.second);
		throw NOM_ACTIVE_ERR;
	}

	//fill rawIgnore
	rawIgnore.resize(colN, false);
	for(intset::iterator aIt = ignoreAttrs.begin(); aIt != ignoreAttrs.end(); aIt++)
		rawIgnore[aIdToColNo[*aIt]] = true;

	//Check that all files exist. Done here to avoid crashing on the absence of the test file after spending time on reading train
	if(string(trainFName).compare("") != 0)
	{
		fstream fin;
		fin.open(trainFName, ios_base::in);
		if(fin.fail()) 
			throw OPEN_TRAIN_ERR;
		fin.close();
	}
	if(string(validFName).compare("") != 0)
	{
		fstream fvalid;
		fvalid.open(validFName, ios_base::in); 
		if(fvalid.fail())
			throw OPEN_VALID_ERR;
		fvalid.close();
	}
	if(string(testFName).compare("") != 0)
	{//Read test set
		fstream ftest;
		ftest.open(testFName, ios_base::in); 
		if(ftest.fail()) 
			throw OPEN_TEST_ERR;
		ftest.close();
	}

	//Read data
	if(string(trainFName).compare("") != 0)
	{//Read train set
		telog << "Reading the train set: \"" << trainFName << "\"\n";
		fstream fin;
		fin.open(trainFName, ios_base::in);
		if(fin.fail()) 
			throw OPEN_TRAIN_ERR;
		 
		hasMV = false;
		hasActiveMV = false;
		intv mvCounts(activeAttrN);
		intv activeAttrs;
		getActiveAttrs(activeAttrs);

		getLineExt(fin, buf);
		int caseNo;
		for(caseNo = 0; fin.gcount(); caseNo++)
		{//read one line of data file, save class value in targets, attribute values in data
			if(doOut && ((caseNo + 1)% 100000 == 0))
				cout << "\tRead " << caseNo + 1 << " lines..." << endl;
			
			floatv item;	//single data point
			try {
				readData(buf, fin.gcount(), item, colN);
			
				if(isnan(item[tarColNo]))
					throw MV_CLASS_TRAIN_ERR;
				trainTar.push_back(item[tarColNo]);
			
				if(weightColNo != -1)
					trainWt.push_back(item[weightColNo]);

				item.erase(item.begin() + max(tarColNo, weightColNo));
				if(weightColNo != -1)
					item.erase(item.begin() + min(tarColNo, weightColNo));

				//check if boolean attributes have valid values
				for (intset::iterator boolIt = boolAttrs.begin(); boolIt != boolAttrs.end(); boolIt++)
				{
					int attrId = *boolIt;
					if ((item[attrId] != 0) && (item[attrId] != 1) && !isnan(item[attrId]))
					{
						cerr << "\nAttribute " << getAttrName(attrId);
						throw ATTR_NOT_BOOL_ERR;
					}
				}

				if(hasMV)
					for(int activeNo = 0; activeNo < activeAttrN; activeNo++)
						if(isnan(item[activeAttrs[activeNo]]))
							mvCounts[activeNo]++;
			}
			catch (TE_ERROR err) {
				cerr << "\nLine " << caseNo + 1;
				throw err;
			}
			train.push_back(item);
			getLineExt(fin, buf);
		}
		trainN = caseNo;
		if(trainN == 0)
			throw TRAIN_EMPTY_ERR;

		if(weightColNo == -1)
			trainWt.resize(trainN, 1);

		double trainStD = getTarStD(TRAIN);
		telog << trainN << " points in the train set, std. dev. of " << tarName << " values = " << trainStD 
			<< "\n\n"; 
		fin.close();

		//output missing values counts
		for(int activeNo = 0; activeNo < activeAttrN; activeNo++)
			if(mvCounts[activeNo] > 0)
				hasActiveMV = true;
		if(hasActiveMV)
		{
			fstream fmisval;
			fmisval.open("missing_values.txt", ios_base::out);
			fmisval << "Attribute\tnumber of missing values in the training data\n";
			for(int activeNo = 0; activeNo < activeAttrN; activeNo++)
				if(mvCounts[activeNo] > 0)
					fmisval << getAttrName(activeAttrs[activeNo]) << "\t" << mvCounts[activeNo] << "\n";
			fmisval.close();
			telog << "Warning: active attributes have missing values. More information in missing_values.txt.\n\n";
		}

		//initialize bootstrap (bag of data)
		bootstrap.resize(trainN); 
		newBag();	
	}
	else //no train set
		trainN = 0;

	if(string(validFName).compare("") != 0)
	{//Read validation set
		telog << "Reading the validation set: \"" << validFName << "\"\n";
		fstream fvalid;
		fvalid.open(validFName, ios_base::in); 
		if(fvalid.fail())
			throw OPEN_VALID_ERR;

		getLineExt(fvalid, buf);
		int caseNo;
		for(caseNo=0; fvalid.gcount(); caseNo++)
		{//read one line of data file, save response value in validtar, attributes values in valid
			if (doOut && ((caseNo + 1) % 100000 == 0))
				cout << "\tRead " << caseNo + 1 << " lines..." << endl;
			
			floatv item;	//single data point
			try {
				readData(buf, fvalid.gcount(), item, colN);
			} catch (TE_ERROR err) {
				cerr << "\nLine " << caseNo + 1;
				throw err;
			}
			if(isnan(item[tarColNo]))
				throw MV_CLASS_VALID_ERR;
			validTar.push_back(item[tarColNo]);
			
			if(weightColNo != -1)
				validWt.push_back(item[weightColNo]);
			item.erase(item.begin() + max(tarColNo, weightColNo));
			if(weightColNo != -1)
				item.erase(item.begin() + min(tarColNo, weightColNo));

			valid.push_back(item);
			getLineExt(fvalid, buf);
		}
		validN = caseNo;
		if(validN == 0)
			throw VALID_EMPTY_ERR;
		if(weightColNo == -1)
			validWt.resize(validN, 1);

		double validStD = getTarStD(VALID);
		telog << validN << " points in the validation set, std. dev. of " << tarName << " values = " 
			<< validStD << "\n\n"; 
		fvalid.close();
	}
	else	//no validation set
		validN = 0;

	if(string(testFName).compare("") != 0)
	{//Read test set
		telog << "Reading the test set: \"" << testFName << "\"\n";
		fstream ftest;
		ftest.open(testFName, ios_base::in); 
		if(ftest.fail()) 
			throw OPEN_TEST_ERR;

		getLineExt(ftest, buf);
		int caseNo;
		for(caseNo=0; ftest.gcount(); caseNo++)
		{//read one line of data file, save response value in testtar, attributes in test
			if (doOut && ((caseNo + 1) % 100000 == 0))
				cout << "\tRead " << caseNo + 1 << " lines...\n";

			floatv item;	//single data point
			try {
				readData(buf, ftest.gcount(), item, colN);
			} catch (TE_ERROR err) {
				cerr << "\nLine " << caseNo + 1;
				throw err;
			}

			testTar.push_back(item[tarColNo]);
			if(weightColNo != -1)
				testWt.push_back(item[weightColNo]);
			item.erase(item.begin() + max(tarColNo, weightColNo));
			if(weightColNo != -1)
				item.erase(item.begin() + min(tarColNo, weightColNo));

			test.push_back(item);
			getLineExt(ftest, buf);
		}
		testN = caseNo;

		if(weightColNo == -1)
			testWt.resize(testN, 1);
		double testStD = getTarStD(TEST);
		telog << testN << " points in the test set, std. dev. of " << tarName << " values = " << testStD 
			<< "\n\n";
		ftest.close();
	}
	else	//no test set
		testN = 0;
}

//Gets a line of text, returns a vector with data points 
//Missing values should be encoded as '?', they get converted to NANs
//hasMV (class member) value becomes true if there are missing values (otherwise not changed)
void INDdata::readData(char* buf, streamsize buflen, floatv& retv, int retvlen)
{
	//remove spaces (there can be spaces in nominal values, space should not be a delimiter
	//and also should be ignored when it is next to a number)

	string line;
	line.reserve((size_t)buflen);
	for(int chNo = 0; chNo < buflen; chNo ++)
		if(buf[chNo] != ' ')
			line.push_back(buf[chNo]);

	retv.resize(retvlen, 0);
	stringstream itemstr(line.c_str());
	string singleItem;
	for(int colNo = 0; colNo < retvlen; colNo++)
	{
		itemstr >> singleItem;
		if(itemstr.fail())
			throw ATTR_DATA_MISMATCH_L_ERR;
		singleItem = string(singleItem.c_str()); //to trim '\0'
		if(singleItem.compare("?"))
		{//should be a number, convert it
			stringstream sistr(singleItem);
			sistr >> retv[colNo];
			if(sistr.fail() && !rawIgnore[colNo])
			{
				cerr << "\nColumn " << colNo + 1 << ", " << colToName(colNo);
				throw NON_NUMERIC_VALUE_ERR;
			}
		}
		else //missing value
		{
			retv[colNo] = QNAN;
			hasMV = true;
		}
	}

	itemstr >> singleItem;
	if(!itemstr.fail())
	  throw ATTR_DATA_MISMATCH_G_ERR;
}

//Puts bootstrapped ids (indices) of train set data points into bootstrap vector
void INDdata::newBag(void)
{
	boolv oobInx(trainN, true); //true if item is not in current bag
	bagV = 0;

	for(int itemNo = 0; itemNo < trainN; itemNo++)
	{//put a new item into bag
		double randCoef = rand_coef();
		int nextItem = (int) ((trainN - 1) * randCoef);
		bootstrap[itemNo] = nextItem;
		oobInx[nextItem] = false;
		bagV += trainWt[itemNo];
	}

	//calculate number of oob cases
	oobN = 0;
	for(int itemNo = 0; itemNo < trainN; itemNo++)
		if(oobInx[itemNo])
			oobN++;
		
	if(oobN == 0)
		newBag(); //we need out of bag data, so try again

	//fill out of bag data
	oobData.resize(oobN);
	int oobNo = 0;
	for(int itemNo = 0; itemNo < trainN; itemNo++)
		if(oobInx[itemNo])
		{
			oobData[oobNo] = itemNo;
			oobNo++;
		}

	//create versions of data sorted by values of attributes
	sortItems();
}

//subsampling without replacement
void INDdata::newSample(int sampleN)
{
	intv inxv(trainN);
	for(int i = 0; i < trainN; i++)
		inxv[i] = i;

	bootstrap.clear();
	bootstrap.resize(sampleN);
	bagV = 0;

	for(int i = 0; i < sampleN; i++)
	{
		double randCoef = rand_coef();
		int nextItem = (int) ((trainN - 1 - i) * randCoef);
		bootstrap[i] = inxv[nextItem];
		bagV += trainWt[i];
		inxv[nextItem] = inxv[trainN - 1 - i];
	}

	//create versions of data sorted by values of attributes
	sortItems();
}

//In order to decrease the training time, we keep indexes of current training data sorted by values of 
//each attribute. This function does the initial sorting and 
//initializes sortedItems - a vector of vectors corresponding to active continuous attributes. 
//At the end each vector should contain pairs (data point id, attribute value), and be
//sorted by attribute values. Only data points where the attribute of interest is defined are included
void INDdata::sortItems()
{
	//get a list of defined attributes
	intv attrs;
	getActiveAttrs(attrs);
	int actAttrN = (int)attrs.size();

	int sampleN = (int)bootstrap.size();

	//reserve space for sortedItems
	sortedItems.clear();
	sortedItems.resize(actAttrN);
	for(int attrNo = 0; attrNo < actAttrN; attrNo++)
		if(!boolAttr(attrs[attrNo]))
			sortedItems[attrNo].reserve(sampleN);

	//fill sortedItems 
	for(int attrNo = 0; attrNo < actAttrN; attrNo++)
		if(!boolAttr(attrs[attrNo]))
		{
			for(int itemNo = 0; itemNo < sampleN; itemNo++)
			{
				float value = train[bootstrap[itemNo]][attrs[attrNo]];
				if(!isnan(value))
					sortedItems[attrNo].push_back(fipair(value, itemNo));
			}
			sort(sortedItems[attrNo].begin(), sortedItems[attrNo].end());
		}
}

//Returns ids of all active attributes (attributes that are allowed to use in the model)
void INDdata::getActiveAttrs(intv& attrs)
{
	attrs.clear();
	attrs.reserve(attrN);
	for(int attrId = 0; attrId < attrN; attrId++)
		if(ignoreAttrs.find(attrId) == ignoreAttrs.end())
			attrs.push_back(attrId);
}

//Returns true if the attribute is boolean
bool INDdata::boolAttr(int attrId)
{
	return boolAttrs.find(attrId) != boolAttrs.end();
}

//Gets attrID by its name
//If the name is invalid, returns -1
int INDdata::getAttrId(string attrName)
{
	for(int attrId = 0; attrId < (int)attrNames.size(); attrId++ )
		if(attrNames[attrId].compare(attrName) == 0)
			return attrId;
	return -1;
}

//Gets out of bag data info (oobData, oobTar, oobN)
int INDdata::getOutOfBag(intv& oobData_out, doublev& oobTar, doublev& oobWt)
{
	oobData_out = oobData;
	oobTar.resize(oobN);
	oobWt.resize(oobN);
	
	for(int oobNo = 0; oobNo < oobN; oobNo++)
	{
		oobTar[oobNo] = trainTar[oobData[oobNo]];
		oobWt[oobNo] = trainWt[oobData[oobNo]];
	}

	return oobN;
}

//Gets validaton data info (validTar, validN)
int INDdata::getTargets(doublev& targets, doublev& weights, DATA_SET dset)
{
	if(dset == TRAIN)
	{
		targets = trainTar;
		weights = trainWt;
		return trainN;
	}
	else if(dset == VALID)
	{
		targets = validTar;
		weights = validWt;
		return validN;
	}
	else //(dset == TEST)
	{
		targets = testTar;
		weights = testWt;
		return testN;
	}
}

//Fills itemSet with ids and responses of data points in the current bag
void INDdata::getCurBag(ItemInfov& itemSet)
{ 
	int sampleN = (int)bootstrap.size();
	itemSet.resize(sampleN);

	for(int i = 0; i < sampleN; i++)
	{
		itemSet[i].key = bootstrap[i];
		itemSet[i].response = trainTar[bootstrap[i]];
		itemSet[i].coef = trainWt[bootstrap[i]];
	}
}

//Returns ids and responses of data points in the current bag
int INDdata::getCurBag(intv& bagData, doublev& bagTar, doublev& bagWt)
{ 
	int sampleN = (int)bootstrap.size();
	bagData = bootstrap;
	bagTar.resize(sampleN);
	bagWt.resize(sampleN);

	for(int i = 0; i < sampleN; i++)
	{
		bagTar[i] = trainTar[bootstrap[i]];
		bagWt[i] = trainWt[bootstrap[i]];
	}

	return sampleN;
}

//Creates a copy of sortedItems
void INDdata::getSortedData(fipairvv& sorted)
{
	sorted = sortedItems;
}

//gets the value of a given attribute (attrId) for a given case (itemNo) in a given data set (dset)
//returns whether the value in question is defined
double INDdata::getValue(int itemNo, int attrId, DATA_SET dset)
{
	if(attrId >= attrN)
		throw ATTR_ID_ERR;
	if(dset == TRAIN)
		return train[itemNo][attrId];
	else if(dset == TEST)
		return test[itemNo][attrId];
	else //if(dset == VALID)
		return valid[itemNo][attrId];
}

//checks if target values are present for test data 
bool INDdata::hasTrueTest()
{
	for(int itemNo = 0; itemNo < testN; itemNo++)
		if(isnan(testTar[itemNo]))
			return false;
	return true;
}

//returns the name of an attribute by its number
string INDdata::getAttrName(int attrId)
{
	return attrNames[attrId];
}

//returns std of response values
double INDdata::getTarStD(DATA_SET ds)
{
	doublev* ptargets = NULL;
	doublev* pweights = NULL;
	if(ds == TRAIN)
	{
		ptargets = &trainTar;	
		pweights = &trainWt;
	}
	if(ds == TEST)
	{
		ptargets = &testTar;
		pweights = &testWt;
	}
	if(ds == VALID)
	{	
		ptargets = &validTar;
		pweights = &validWt;
	}
	doublev& targets = *ptargets;
	doublev& weights = *pweights;

	int itemN = (int)targets.size();
	if(itemN == 0)
		return 0;
	
	double volume = 0;
	double mean = 0;

	for(int itemNo = 0; itemNo < itemN; itemNo++)
	{
		mean += targets[itemNo] * weights[itemNo];		
		volume += weights[itemNo];
	}
	mean /= volume;														

	double var = 0;
	for(int itemNo = 0; itemNo < itemN; itemNo++)
		var += pow(targets[itemNo] - mean, 2) * weights[itemNo];	
	var /= volume;														

	return sqrt(var);
}

//checks if attrId is valid and active
bool INDdata::isActive(int attrId)
{	
	if(attrId < 0)
		return false;
	if(attrId >= attrN)
		return false;
	if(ignoreAttrs.find(attrId) != ignoreAttrs.end())
		return false;
	return true;
}

//deactivate the attribute
void INDdata::ignoreAttr(int attrId) 
{
	ignoreAttrs.insert(attrId);
}

//activate the attribute
void INDdata::useAttr(int attrId)
{
	ignoreAttrs.erase(attrId);
}

//return all values of the attribute from the validation set (sorted, with duplicates, missing values at the end)
void INDdata::getValues(int attrId, doublev& values)
{
	values.clear();
	for(int itemNo = 0; itemNo < validN; itemNo++)
		values.push_back(valid[itemNo][attrId]);

	sort(values.begin(), values.end(), lessNaN);
}

//gets all pairs of values of two specific attributes in the validation data
void INDdata::getValues(int attr1Id, int attr2Id, ddpairv& values)
{
	values.clear();
	for(int itemNo = 0; itemNo < validN; itemNo++)
		values.push_back(ddpair(valid[itemNo][attr1Id], valid[itemNo][attr2Id]));

	sort(values.begin(), values.end(), lessNaNP);
}

//inserts a new data point into the test set, returns its id (number)
//in: values is a vector of (attrId, attrVal) pairs
int INDdata::addTestItem(idpairv& values) 
{
	test.resize(testN + 1, floatv(attrN, QNAN));
	testTar.resize(testN + 1, QNAN);
	testN++;

	for(int valNo = 0; valNo < (int)values.size(); valNo++)
		test[testN - 1][values[valNo].first] = (float)values[valNo].second;

	return testN - 1;
}

//outputs a new attribute file when some of the old attributes are not active 
void INDdata::outAttr(string attrFName)
{
	fstream fin, fout;
	fin.open(attrFName.c_str(), ios_base::in); 
	
	//modify the original attribute name. 
	//a) remove directory information
	string::size_type lastSlash1 = attrFName.rfind('\\');
	string::size_type lastSlash2 = attrFName.rfind('/');
	
	string::size_type lastSlash = (lastSlash1 == string::npos) ? 
		((lastSlash2 == string::npos) ? -1 : lastSlash2) : 
		((lastSlash2 == string::npos) ? lastSlash1 : max(lastSlash1, lastSlash2));
		attrFName = attrFName.substr(lastSlash + 1);
	
	//b) insert .fs into the attribute file name before the .extension
	string newAttrFName = insertSuffix(attrFName, "fs");

	fout.open(newAttrFName.c_str(),ios_base::out); 
	
	//copy old attr file (up to contexts line, if present) into the new one
	char buf[LINE_LEN];			//buffer for reading from the input file
	std::streamsize bufLen = getLineExt(fin, buf);
	while(bufLen > 1)
	{
		if((string(buf)).find("contexts") != string::npos)
			break;
			

		fout.write(buf, bufLen - 1);
		fout << "\n";

		bufLen = getLineExt(fin, buf);
	}
	fin.close();

	//add info about eliminated attributes
	fout << "contexts:\n";

	for(intset::iterator attrIt = ignoreAttrs.begin(); attrIt != ignoreAttrs.end(); attrIt++)
		fout << attrNames[*attrIt] << " never.\n";

	fout.close();
}

//returns quantN internal quantile values and their counts in valCounts
//returns the length of valCounts as return value
int INDdata::getQuantiles(int attrId, int& quantN, dipairv& valCounts)
{
	LogStream telog;

	doublev attrVals; //all values of given attribute in the validation set; sorted, w duplicates
	getValues(attrId, attrVals); 
	int valsN = (int)attrVals.size();
	bool singleVal = false;
	if(equalsNaN(attrVals[0], attrVals[valsN - 1]))
	{
		telog << "Warning: feature " << attrNames[attrId] << " takes on a single value on the whole validation set\n";
		singleVal = true;
	}
	
	//figure out the appropriate number of quantiles: if the requested number is not enough (all quantile 
	//values are the same), double it until success.
	bool setQN = false;
	double oldQuantN = quantN;
	while(!setQN)
	{
		double firstQuant = attrVals[ (valsN - 1) / (quantN + 1) ];
		double lastQuant = attrVals[(size_t) ((valsN - 1) * ((double)quantN / (quantN + 1))) ];
		if(!equalsNaN(firstQuant, lastQuant) || singleVal)
			setQN = true;
		else
			quantN *= 2;
	}
	if(oldQuantN != quantN)
		telog << "Warning: " << oldQuantN << " quantile values was not enough to see the effect of " 
			<< attrNames[attrId] << ". The number of quantile values for this feature was changed to " 
			<< quantN << ".\n\n";

	//split values on quantN + 1 equal parts and take out quantN quantile points (ignoring the end points)
	doublev quantVals(quantN); 	//quantile values - values from attrVals on equal distances 
	for(int quantNo = 0; quantNo < quantN; quantNo++)
		quantVals[quantNo] = attrVals[(size_t) ((valsN - 1) * ((double)(quantNo + 1) / (quantN + 1))) ];

	//extract unique values and there counts
	valCounts.push_back(dipair(quantVals[0], 1));
	int uValsN = 1;
	for(int quantNo = 1; quantNo < quantN; quantNo++)
		if(!equalsNaN(quantVals[quantNo], valCounts[uValsN - 1].first))
		{
			valCounts.push_back(dipair(quantVals[quantNo], 1));
			uValsN++;
		}
		else
			valCounts[uValsN - 1].second++;

	return uValsN;
}

//Calculates and outputs correlation scores between active attributes based on the training set. Weights are ignored.
void INDdata::correlations(string trainFName)
{
	LogStream telog;

	//get a list of defined attributes
	intv attrs;
	getActiveAttrs(attrs);
	size_t activeN = attrs.size();

	size_t itemN = getTrainN();

	//reserve space for sortedItems
	sortedItems.clear();
	sortedItems.resize(activeN);
	for(size_t attrNo = 0; attrNo < activeN; attrNo++)
		sortedItems[attrNo].reserve(itemN);

	//fill sortedItems 
	for(size_t attrNo = 0; attrNo < activeN; attrNo++)
	{
		for(size_t itemNo = 0; itemNo < itemN; itemNo++)
		{
			float value = train[bootstrap[itemNo]][attrs[attrNo]];
			if(isnan(value))
				throw CORR_MV_ERR;

			sortedItems[attrNo].push_back(fipair(value, itemNo));
		}
		sort(sortedItems[attrNo].begin(), sortedItems[attrNo].end());
		
		//replace actual values with ranks. Ties get average rank.
		int lastDone = -1;
		float curVal = sortedItems[attrNo][0].first;
		for(size_t itemNo = 1; itemNo <= itemN; itemNo++)
			if((itemNo == itemN) || (sortedItems[attrNo][itemNo].first != curVal))
			{
				if(itemNo != itemN)
					curVal = sortedItems[attrNo][itemNo].first;
				float rank = (float)((lastDone + itemNo) / 2.0 + 1);
				for(size_t fillNo = lastDone + 1; fillNo < itemNo; fillNo++)
					sortedItems[attrNo][fillNo].first = rank;
				lastDone = itemNo - 1;
			}

		//re-sort by itemId
		sort(sortedItems[attrNo].begin(), sortedItems[attrNo].end(), ltSecond);
	}

	//calculate Spearman's rank correlation values
	double coef = 6.0 / (itemN * ((double)itemN * itemN - 1.0));

	doublev stub(activeN, 0);
	doublevv correlations(activeN, stub);

	ssdtriplev corrv;
	corrv.reserve(activeN * (activeN - 1));

	for(size_t attrNo1 = 0; attrNo1 < activeN; attrNo1++)
		for(size_t attrNo2 = 0; attrNo2 < activeN; attrNo2++)
			if(attrNo1 > attrNo2)
				correlations[attrNo1][attrNo2] = correlations[attrNo2][attrNo1];
			else if(attrNo1 == attrNo2)
				correlations[attrNo1][attrNo2] = QNAN;
			else
			{
				double corr = 0;
				for(size_t itemNo = 0; itemNo < itemN; itemNo++)
				{
					double d = sortedItems[attrNo1][itemNo].first - sortedItems[attrNo2][itemNo].first;
					corr += coef * d * d;
				}
				correlations[attrNo1][attrNo2] = 1 - corr;
				corrv.push_back(ssdtriple(sspair(getAttrName(attrs[attrNo1]), getAttrName(attrs[attrNo2])), 1 - corr));
			}

	//open output file
	string outFName = /*beforeLastDot(trainFName) + "." + */"correlations.txt";
	fstream fcorr(outFName.c_str(), ios_base::out);

	//output in the sorted list of triples format
	sort(corrv.begin(), corrv.end(), gtAbsThird);
	for(size_t pairNo = 0; pairNo < corrv.size(); pairNo++)
		fcorr << corrv[pairNo].first.first << "\t" << corrv[pairNo].first.second << "\t" << corrv[pairNo].second << endl;


	// output in the table format
	fcorr << "\n" << QNAN;
	for(size_t attrNo = 0; attrNo < activeN; attrNo++)
		fcorr << "\t" << getAttrName(attrs[attrNo]);
	fcorr << endl;

	for(size_t attrNo1 = 0; attrNo1 < activeN; attrNo1++)
	{
		fcorr << getAttrName(attrs[attrNo1]); 
		for(size_t attrNo2 = 0; attrNo2 < activeN; attrNo2++)
			fcorr << "\t" << correlations[attrNo1][attrNo2];
		fcorr << endl;
	}
	fcorr.close();

	telog << "Correlation scores are saved into the file " << outFName << ".\n";
}

//return name of the column (attribute or target)
string INDdata::colToName(int column)
{
	if(column == tarColNo)
		return tarName;

	for(int attrNo = 0; attrNo < attrN; attrNo++)
		if(aIdToColNo[attrNo] == column)
			return attrNames[attrNo];

	return "";
}
