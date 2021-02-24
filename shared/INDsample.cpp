//INDsample.cpp: Sample a bag of data from INDdata 
//
// (c) Xiaojie Wang

#include "INDsample.h"
#include "LogStream.h"
#include "functions.h"

#include <algorithm> // Solve error: 'sort' was not declared in this scope
#include <cmath> // Solve error: use of undeclared identifier 'isnan'

INDsample::INDsample(INDdata& data): data(data)
{
	bootstrap.resize(data.getTrainN());
}

//Puts bootstrapped ids (indices) of train set data points into bootstrap vector
void INDsample::newBag(void)
{
	//Get data references from INDdata
	int trainN = data.getTrainN();
	doublev& trainWt = data.getTrainWt();

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
void INDsample::newSample(int sampleN)
{
	//Get data references from INDdata
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
void INDsample::sortItems()
{
	//Get data references from INDdata
	floatvv& train = data.getTrain();

	//get a list of defined attributes
	intv attrs;
	data.getActiveAttrs(attrs);
	int actAttrN = (int)attrs.size();

	int sampleN = (int)bootstrap.size();

	//reserve space for sortedItems
	sortedItems.clear();
	sortedItems.resize(actAttrN);
	for(int attrNo = 0; attrNo < actAttrN; attrNo++)
		if(!data.boolAttr(attrs[attrNo]))
			sortedItems[attrNo].reserve(sampleN);

	//fill sortedItems 
	for(int attrNo = 0; attrNo < actAttrN; attrNo++)
		if(!data.boolAttr(attrs[attrNo]))
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
	//Get data references from INDdata
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
	//Get data references from INDdata
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
	//Get data references from INDdata
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

//Calculates and outputs correlation scores between active attributes based on the training set. Weights are ignored.
void INDsample::correlations(string trainFName)
{
	//Get data references from INDdata
	floatvv& train = data.getTrain();

	LogStream telog;

	//get a list of defined attributes
	intv attrs;
	data.getActiveAttrs(attrs);
	size_t activeN = attrs.size();

	size_t itemN = data.getTrainN();

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
				corrv.push_back(ssdtriple(sspair(
						data.getAttrName(attrs[attrNo1]), 
						data.getAttrName(attrs[attrNo2])
						), 1 - corr));
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
		fcorr << "\t" << data.getAttrName(attrs[attrNo]);
	fcorr << endl;

	for(size_t attrNo1 = 0; attrNo1 < activeN; attrNo1++)
	{
		fcorr << data.getAttrName(attrs[attrNo1]);
		for(size_t attrNo2 = 0; attrNo2 < activeN; attrNo2++)
			fcorr << "\t" << correlations[attrNo1][attrNo2];
		fcorr << endl;
	}
	fcorr.close();

	telog << "Correlation scores are saved into the file " << outFName << ".\n";
}
