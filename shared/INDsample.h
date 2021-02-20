//INDsample.h: Consist of a state for the rand_r function and a bag of data
//
// (c) Xiaojie Wang

#pragma once
#include "INDdata.h"

class INDsample
{
public:
	INDsample(INDdata& data);

//get functions for private members
	double getBagV() { return bagV; }
	int getBagDataN() { return bootstrap.size(); }
	int getOutOfBag(intv& oobData, doublev& oobTar, doublev& oobWt);
	
//untrivial get functions
	//gets current bag of training data 
	void getCurBag(ItemInfov& itemSet);
	int getCurBag(intv& bagData, doublev& bagTar, doublev& bagWt);

	//gets sorted indexes of current training data
	void getSortedData(fipairvv& sorted);

//action functions
	//replaces bootstrap in the bag
	void newBag(void);

	//subsampling without replacement
	void newSample(int sampleN);

	//calculates and outputs correlation scores between active attributes based on the training set
	void correlations(string trainFName);

private:
	//create versions of bootstrap data sorted by active continuous attributes 
	void sortItems();

private:
	unsigned int state;	// Avoid data sampling of a thread affecting that of other threads
						// The same state yields the same sequence of sampled bags
	INDdata& data;		// Data access reference

	// The following five variables come from the INDdata class
	intv bootstrap;		//indexes of data points currently in the bag, can be repeating
	int oobN;			//number of out-of-bag data points
	intv oobData;		//indexes of out-of-bag data points

	double bagV;		//sum of weights

	fipairvv sortedItems;	//several copies of sorted data points in the bag
							//separate vector for sorting by each attribute
							//each data point represented as (id, attrvalue) pair
};
