// Additive Groves / ag_layered.cpp: implementations of functions for training layered Additive Groves models
// (c) Daria Sorokina

#include "ag_layered.h"
#include "Grove.h"
#include "functions.h"

#include <fstream>
#include <iostream>

//trains a Layered Groves ensemble (Additive Groves trained in layered style) 
//if modelFName is not empty, saves the model
//returns performance on validation set
double layeredGroves(INDdata& data, TrainInfo& ti, string modelFName)
{
	doublev validTar, validWt; //true response values on validation set
	int validN = data.getTargets(validTar, validWt, VALID); 
	doublev predsumsV(validN, 0); 	//sums of predictions for each data point

	if(!modelFName.empty())
	{//save the model's header
		fstream fmodel(modelFName.c_str(), ios_base::binary | ios_base::out);
		fmodel.write((char*) &ti.mode, sizeof(enum AG_TRAIN_MODE));
		fmodel.write((char*) &ti.maxTiGN, sizeof(int));
		fmodel.write((char*) &ti.minAlpha, sizeof(double));
		fmodel.close();		
	}

	//build bagged models, calculate sums of predictions
	for(int bagNo = 0; bagNo < ti.bagN; bagNo++)
	{
		cout << "\t\tIteration " << bagNo + 1 << " out of " << ti.bagN << endl;
		CGrove grove(ti.minAlpha, ti.maxTiGN, ti.interaction);
		INDsample sample(data);
		grove.trainLayered(sample);
		for(int itemNo = 0; itemNo < validN; itemNo++)
			predsumsV[itemNo] += grove.predict(itemNo, VALID);

		if(!modelFName.empty())
			grove.save(modelFName.c_str()); 
	}

	//calculate predictions of the whole ensemble on the validation set
	doublev predictions(validN); 
	for(int itemNo = 0; itemNo < validN; itemNo++)
		predictions[itemNo] = predsumsV[itemNo] / ti.bagN;

	if(ti.rms)
		return rmse(predictions, validTar, validWt);
	else
		return roc(predictions, validTar, validWt);	
}

//runs Layered Groves repeatN times, returns average performance and standard deviation
//saves the model from the last run
double meanLG(INDdata& data, TrainInfo ti, int repeatN, double& resStd, string modelFName)
{
	doublev resVals(repeatN);
	int repeatNo;
	cout << endl << "Estimating distribution of model performance" << endl;
	for(repeatNo = 0; repeatNo < repeatN; repeatNo++)
	{
		cout << "\tTraining model " << repeatNo + 1 << " out of " << repeatN << endl;		
		if(repeatNo == repeatN - 1)
			resVals[repeatNo] = layeredGroves(data, ti, modelFName); //save the last model
		else
			resVals[repeatNo] = layeredGroves(data, ti, string(""));
	}

	//calculate mean
	double resMean = 0;
	for(repeatNo = 0; repeatNo < repeatN; repeatNo++)
		resMean += resVals[repeatNo];
	resMean /= repeatN;

	//calculate standard deviation
	resStd = 0;
	for(repeatNo = 0; repeatNo < repeatN; repeatNo++)
		resStd += (resMean - resVals[repeatNo])*(resMean - resVals[repeatNo]);
	resStd /= repeatN;
	resStd = sqrt(resStd);

	return resMean;
}