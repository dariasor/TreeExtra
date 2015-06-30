// Additive Groves / TrainInfo.h: implementation of the TrainInfo structure
// This structure contains all parameters relevant to training an Additive Groves model

// (c) Daria Sorokina

#pragma once
#include "ag_definitions.h"

struct TrainInfo
{
public:
	double minAlpha;	//min proportion of train set in the leaf (controls size of tree)
	int maxTiGN;		//number of trees in a grove
	int bagN;			//number of bagging iterations
	AG_TRAIN_MODE mode;	//mode of training Groves (fast/slow/layered)
	bool rms;			//which performance metric is used (rms/roc)
	int seed;			//random number initializer

	//file names
	string trainFName;	//train set
	string validFName;	//validation set
	string testFName;	//test set
	string attrFName;	//attributes description 	

	intv interaction;	//a higher-order interaction between all these attributes 
							//should not be allowed in the model (model is restricted on interaction)

	TrainInfo(): minAlpha(0.01), maxTiGN(8), bagN(60), mode(FAST), rms(true), seed(1){};
};
