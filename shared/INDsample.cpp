//INDsample.cpp: Sample a bag of data from INDdata and use a state for sampling
//
// (c) Xiaojie Wang

#include "INDsample.h"
#include "LogStream.h"
#include "functions.h"

#include <algorithm> // Solve error: 'sort' was not declared in this scope
#include <cmath> // Solve error: use of undeclared identifier 'isnan'

INDsample::INDsample(unsigned int state, INDdata& data):
	state(state), data(data)
{
	bootstrap.resize(data.getTrainN());
}

//Puts bootstrapped ids (indices) of train set data points into bootstrap vector
void INDsample::newBag(void)
{
	// XW. Get data references from INDdata
	int trainN = data.getTrainN();
	doublev& trainWt = data.getTrainWt();

	boolv oobInx(trainN, true); //true if item is not in current bag
	bagV = 0;

	for(int itemNo = 0; itemNo < trainN; itemNo++)
	{//put a new item into bag
		double randCoef = rand_coef(state); // XW. Use rand_r for thread safety
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
void INDsample::newSample(int sampleN)
{
	// XW. Get data references from INDdata
	int trainN = data.getTrainN();
	doublev& trainWt = data.getTrainWt();

	intv inxv(trainN);
	for(int i = 0; i < trainN; i++)
		inxv[i] = i;

	bootstrap.clear();
	bootstrap.resize(sampleN);
	bagV = 0;

	for(int i = 0; i < sampleN; i++)
	{
		double randCoef = rand_coef(state); // XW. Use rand_r for thread safety
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
void INDsample::sortItems()
{
	// XW. Get data references from INDdata
	floatvv& train = data.getTrain();

	//get a list of defined attributes
	intv attrs;
	data.getActiveAttrs(attrs); // XW
	int actAttrN = (int)attrs.size();

	int sampleN = (int)bootstrap.size();

	//reserve space for sortedItems
	sortedItems.clear();
	sortedItems.resize(actAttrN);
	for(int attrNo = 0; attrNo < actAttrN; attrNo++)
		if(!data.boolAttr(attrs[attrNo])) // XW
			sortedItems[attrNo].reserve(sampleN);

	//fill sortedItems 
	for(int attrNo = 0; attrNo < actAttrN; attrNo++)
		if(!data.boolAttr(attrs[attrNo])) // XW
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

//Gets out of bag data info (oobData, oobTar, oobN)
int INDsample::getOutOfBag(intv& oobData_out, doublev& oobTar, doublev& oobWt)
{
	// XW. Get data references from INDdata
	doublev& trainTar = data.getTrainTar();
	doublev& trainWt = data.getTrainWt();

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

//Fills itemSet with ids and responses of data points in the current bag
void INDsample::getCurBag(ItemInfov& itemSet)
{
	// XW. Get data references from INDdata
	doublev& trainTar = data.getTrainTar();
	doublev& trainWt = data.getTrainWt();

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
int INDsample::getCurBag(intv& bagData, doublev& bagTar, doublev& bagWt)
{
	// XW. Get data references from INDdata
	doublev& trainTar = data.getTrainTar();
	doublev& trainWt = data.getTrainWt();

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
void INDsample::getSortedData(fipairvv& sorted)
{
	sorted = sortedItems;
}
