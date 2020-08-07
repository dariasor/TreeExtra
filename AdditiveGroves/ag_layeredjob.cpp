//ag_layeredjob.cpp: 
//
// (c) Xiaojie Wang

#include "ag_layeredjob.h"

// XW. Too many arguments are needed to pass to jobs
struct LayeredArg
{
	LayeredArg(
			int bagNo,
			INDdata& data,
			TrainInfo& ti,
			int validN,
			string modelFName,
			doublevv& _predsumsV
			):
		bagNo(bagNo),
		data(data),
		ti(ti),
		validN(validN),
		modelFName(modelFName),
		_predsumsV(_predsumsV)
		{}
	int bagNo;
	INDdata& data;
	TrainInfo& ti;
	int validN;
	string modelFName;
	doublevv& _predsumsV;
};

// XW
#ifndef _WIN32
TMutex StdOutMutex; // Make sure only one thread is using the standard output
TMutex DirMutex; // Probably not needed as only the first thread writes to dir
TMutex ReturnMutex; // Write to the variables computed and returned by threads
#endif
// XW. Can be used in both a single-threaded setting and a multi-threaded setting
void doLayered(LayeredArg* ptr)
{
	int bagNo = ptr->bagNo;
	INDdata& data = ptr->data;
	TrainInfo& ti = ptr->ti;
	int validN = ptr->validN;
	string modelFName = ptr->modelFName;

	// XW
	unsigned int state = time(NULL) + bagNo;
	INDsample sample(state, data);

	doublev __predsumsV(validN, 0);

#ifndef _WIN32
StdOutMutex.Lock();
#endif
	cout << "\t\tIteration " << bagNo + 1 << " out of " << ti.bagN << " (begin)" << endl;
#ifndef _WIN32
StdOutMutex.Unlock();
#endif

	CGrove grove(ti.minAlpha, ti.maxTiGN, ti.interaction);
	grove.trainLayered(sample); // XW
	for (int itemNo = 0; itemNo < validN; itemNo ++)
		__predsumsV[itemNo] = grove.predict(itemNo, VALID); // XW

	// XW. Multiple threads write to different temp files
	if (! modelFName.empty())
	{
		string _modelFName = getModelFName(modelFName, bagNo);
		// XW. Clear previous temp files as the save function appends to the files
		fstream fload(_modelFName.c_str(), ios_base::binary | ios_base::out);
		fload.close();
		grove.save(_modelFName.c_str());
	}

	// XW. Only use mutex once here and not everywhere
#ifndef _WIN32
ReturnMutex.Lock();
#endif
	// XW. Mutex is not needed because threads access different slices (memory addresses)
	ptr->_predsumsV[bagNo] = __predsumsV;
#ifndef _WIN32
ReturnMutex.Unlock();
#endif
	// XW. Add mutex here doesn't reduce training time but improves reproducibility

#ifndef _WIN32
StdOutMutex.Lock();
#endif
	cout << "\t\tIteration " << bagNo + 1 << " out of " << ti.bagN << " (end)" << endl;
#ifndef _WIN32
StdOutMutex.Unlock();
#endif

	return;
}

// XW. Wrap the doLayered function by TJob to be submitted to a thread pool
#ifndef _WIN32
class LayeredJob: public TThreadPool::TJob
{
public:
	void Run(void* ptr)
	{
		doLayered((LayeredArg*) ptr);
	}
};
#endif

//trains a Layered Groves ensemble (Additive Groves trained in layered style) 
//if modelFName is not empty, saves the model
//returns performance on validation set
double layeredGroves(
		INDdata& data, 
		TrainInfo& ti, 
		string modelFName, 
		TThreadPool& pool
		)
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

	// XW. Build bagged models, calculate sums of predictions
	doublevv _predsumsV(ti.bagN, doublev((validN, 0)));
#ifdef _WIN32
	for (int bagNo = 0; bagNo < ti.bagN; bagNo ++)
	{
		runBag(new LayeredArg(
				bagNo, 
				data, 
				ti, 
				validN, 
				modelFName, 
				_predsumsV
				));
	}
#else
	for (int bagNo = 0; bagNo < ti.bagN; bagNo ++)
	{
		LayeredArg* ptr = new LayeredArg(
				bagNo, 
				data, 
				ti, 
				validN, 
				modelFName, 
				_predsumsV
				);
		pool.Run(new LayeredJob, ptr);
	}
	pool.SyncAll();
#endif

	// XW
	for (int bagNo = 0; bagNo < ti.bagN; bagNo ++)
	{
		if (! modelFName.empty())
		{
			CGrove grove(ti.minAlpha, ti.maxTiGN, ti.interaction);
			string _modelFName = getModelFName(modelFName, bagNo);
			fstream fload(_modelFName.c_str(), ios_base::binary | ios_base::in);
			grove.load(fload);
			grove.save(modelFName.c_str());
		}

		for (int itemNo = 0; itemNo < validN; itemNo ++)
			predsumsV[itemNo] += _predsumsV[bagNo][itemNo];
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
double meanLG(
		INDdata& data, 
		TrainInfo ti, 
		int repeatN, 
		double& resStd, 
		string modelFName, 
		TThreadPool& pool
		)
{
	doublev resVals(repeatN);
	int repeatNo;
	cout << endl << "Estimating distribution of model performance" << endl;
	for(repeatNo = 0; repeatNo < repeatN; repeatNo++)
	{
		cout << "\tTraining model " << repeatNo + 1 << " out of " << repeatN << endl;		
		if(repeatNo == repeatN - 1)
		{
			//save the last model
			resVals[repeatNo] = layeredGroves(data, ti, modelFName, pool); // XW
		}
		else
			resVals[repeatNo] = layeredGroves(data, ti, string(""), pool); // XW
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

// XW
string getModelFName(string modelFName, int bagNo)
{
	string _modelFName = string("./AGTemp/") 
			+ insertSuffix(modelFName, "b." + itoa(bagNo, 10));
	return _modelFName;
}

