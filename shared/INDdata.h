// INDdata.h: interface for the INDdata class

#pragma once
#include "ItemInfo.h"

class INDdata
{
public:
	//loads data into memory
	INDdata(const char* trainFName, const char* valFName, const char* testFName, 
			const char* attrFName, bool doOut = true);
	
//get functions for private members  
	int getAttrN(){return attrN;}	
	int getTrainN(){return trainN;}
	int getTarColNo(){return tarColNo;}
	int getTargets(doublev& targets, doublev& weights, DATA_SET dset);
	bool getHasWeights(){return weightColNo != -1;}
	bool getHasActiveMV(){return hasActiveMV;}
	bool useCoef(){return hasActiveMV || (weightColNo != -1);}
	int getColNo(int attrId){return aIdToColNo[attrId];}
	//Return data references to avoid copying large sized data
	doublev& getTrainTar() { return trainTar; }
	doublev& getTrainWt() { return trainWt; }
	floatvv& getTrain() { return train; }

//untrivial get functions

	//gets attrID by its name
	int getAttrId(string attrName); 

	//return name of the column (attribute or target)
	string colToName(int column);

	//gets a list of active attributes
	void getActiveAttrs(intv& attrs);

	//gets a value of a given attribute for a given case in a given data set
	double getValue(int itemNo, int attrId, DATA_SET dset);

	//returns the name of the attribute by its number
	string getAttrName(int attrId);

	//returns counts and quantile values
	int getQuantiles(int attrId, int& quantN, dipairv& valCounts);

	//returns std of response
	double getTarStD(DATA_SET ds);

//"question" functions

	//checks if attribute is boolean
	bool boolAttr(int attrId);

	//checks if all target values in test set are valid
	bool hasTrueTest();

	//checks if the attr number is valid and active
	bool isActive(int attrId);

//action functions

	//deactivates the attribute
	void ignoreAttr(int attrId); 

	//actuvates the attribute
	void useAttr(int attrId);

	//inserts a new data point into the data set
	int addTestItem(idpairv& values); 

	//outputs a version of attribute file where only a predefined set of features is active
	void outAttr(string attrFName);

	//gets all values of two specific attribute in the validation data
	void getValues(int attr1No, int attr2No, ddpairv& values);

	//gets all values of a specific attribute in the validation set
	void getValues(int attrId, doublev& values);

	intset& getSplitAttrs() { return splitAttrs; }

private:
	//gets a line of text, returns a vector with data points
	void readData(char* buf, streamsize buflen, floatv& retv, int retvlen); 

private:
	int attrN;			//number of attributes
	int colN;			//number of columns in the data file
	
	intv aIdToColNo;	//attribute ids to column numbers
	intset boolAttrs;	//boolean attributes
	intset nomAttrs;	//nominal attributes
	//boolv rawNom;		//boolean vector with the original number of columns, marks columns with nominal attributes
	intset ignoreAttrs;	//attributes that should be ignored
	intset splitAttrs;	//Attributes that are allowed to split nodes of FirTree (usually query-level features)

	boolv rawIgnore;	//boolean vector with the original number of columns, marks columns with the attributes that should not be used
	stringv attrNames;	//names of attributes
	string tarName;		//name of the response attribute
	int tarColNo;		//response column number
	int weightColNo;	//weights column number

	int trainN;			//number of data points in the train set
	floatvv train;		//train set data w/o response
	doublev trainTar;	//train set response
	doublev trainWt;		//train set weights

	int validN;			//number of data points in the validation set
	floatvv valid;		//validation set data w/o response
	doublev validTar;	//validation set response
	doublev validWt;		//validation set weights

	int testN;			//number of data points in the test set
	floatvv test;		//test set data w/o response
	doublev testTar;	//test set response
	doublev testWt;		//test set weights

	bool hasMV;			//data has missing values
	bool hasActiveMV;	//data has missing values in active attributes

};
