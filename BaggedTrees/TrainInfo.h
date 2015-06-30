// Bagged Trees / TrainInfo.h: implementation of the TrainInfo structure
// This structure contains all parameters relevant to training Bagged Trees
// (c) Daria Sorokina

#pragma once

struct TrainInfo
{
public:
	int bagN;			//number of bagging iterations
	int seed;			//random number initializer
	double alpha;		//min proportion of train set in the leaf (controls size of tree)
	bool rms;			//rms/roc performance metric

	//file names
	string trainFName;	//train set
	string validFName;	//validation set
	string testFName;	//test set
	string attrFName;	//attributes description 	

	TrainInfo(): bagN(60), seed(1), alpha(0), rms(true) {};
};
