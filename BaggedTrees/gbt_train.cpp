//Gradient boosting optimizing RMS. gbt_train.cpp: main function of executable gbt_train
//(c) Daria Sorokina

//gbt_train -t _train_set_ -v _validation_set_ -r _attr_file_ 
//[-a _alpha_value_] [-n _boosting_iterations_] [-i _init_random_] [-c rms|roc]
// [-sh _shrinkage_ ] [-sub _subsampling_]

#include "Tree.h"
#include "functions.h"
#include "TrainInfo.h"
#include "LogStream.h"
#include "ErrLogStream.h"
#include "bt_definitions.h"

#ifndef _WIN32
#include "thread_pool.h"
#endif

#include <algorithm>
#include <errno.h>

int main(int argc, char* argv[])
{	
	try{

//1. Analyze input parameters
	//convert input parameters to string from char*
	stringv args(argc); 
	for(int argNo = 0; argNo < argc; argNo++)
		args[argNo] = string(argv[argNo]);
	
	//check that the number of arguments is even (flags + value pairs)
	if(argc % 2 == 0)
		throw INPUT_ERR;

#ifndef _WIN32
	int threadN = 6;	//number of threads
#endif

	TrainInfo ti; //model training parameters

	//parse and save input parameters
	//indicators of presence of required flags in the input
	bool hasTrain = false;
	bool hasVal = false; 
	bool hasAttr = false; 

	int treeN = 100;
	double shrinkage = 0.01;
	double subsample = -1;

	for(int argNo = 1; argNo < argc; argNo += 2)
	{
		if(!args[argNo].compare("-t"))
		{
			ti.trainFName = args[argNo + 1];
			hasTrain = true;
		}
		else if(!args[argNo].compare("-v"))
		{
			ti.validFName = args[argNo + 1];
			hasVal = true;
		}
		else if(!args[argNo].compare("-r"))
		{
			ti.attrFName = args[argNo + 1];
			hasAttr = true;
		}
		else if(!args[argNo].compare("-a"))
			ti.alpha = atofExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-n"))
			treeN = atoiExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-i"))
			ti.seed = atoiExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-sh"))
			shrinkage = atofExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-sub"))
			subsample = atofExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-c"))
		{
			if(!args[argNo + 1].compare("roc"))
				ti.rms = false;
			else if(!args[argNo + 1].compare("rms"))
				ti.rms = true;
			else
				throw INPUT_ERR;
		}
		else if(!args[argNo].compare("-h"))
#ifndef _WIN32 
			threadN = atoiExt(argv[argNo + 1]);
#else
			throw WIN_ERR;
#endif
		else
			throw INPUT_ERR;
	}//end for(int argNo = 1; argNo < argc; argNo += 2) //parse and save input parameters

	if(!(hasTrain && hasVal && hasAttr))
		throw INPUT_ERR;

	if((ti.alpha < 0) || (ti.alpha > 1))
		throw ALPHA_ERR;

//1.a) Set log file
	LogStream clog;
	LogStream::init(true);
	clog << "\n-----\ngbt_train ";
	for(int argNo = 1; argNo < argc; argNo++)
		clog << argv[argNo] << " ";
	clog << "\n\n";

//1.b) Initialize random number generator. 
	srand(ti.seed);

//2. Load data
	INDdata data(ti.trainFName.c_str(), ti.validFName.c_str(), ti.testFName.c_str(), 
				 ti.attrFName.c_str());
	CTree::setData(data);
	CTreeNode::setData(data);

//2.a) Start thread pool
#ifndef _WIN32
	TThreadPool pool(threadN);
	CTree::setPool(pool);
#endif

//------------------

	fstream frmscurve("boosting_rms.txt", ios_base::out); //bagging curve (rms)
	frmscurve.close();
	fstream froccurve;
	if(!ti.rms)
	{
		froccurve.open("boosting_roc.txt", ios_base::out); //bagging curve (roc) 
		froccurve.close();
	}

	doublev validTar;
	int validN = data.getTargets(validTar, VALID);

	doublev trainTar;
	int trainN = data.getTargets(trainTar, TRAIN);

	int sampleN;
	if(subsample == -1)
		sampleN = trainN;
	else
		sampleN = (int) (trainN * subsample);
	
	doublev validPreds(validN, 0);
	doublev trainPreds(trainN, 0);
	
	for(int treeNo = 0; treeNo < treeN; treeNo++)
	{
		if(subsample == -1)
			data.newBag();
		else
			data.newSample(sampleN);

		CTree tree(ti.alpha);
		tree.setRoot();
		tree.resetRoot(trainPreds);
		idpairv stub;
		tree.grow(false, stub);

		//update predictions
		for(int itemNo = 0; itemNo < trainN; itemNo++)
			trainPreds[itemNo] += shrinkage * tree.predict(itemNo, TRAIN);
		for(int itemNo = 0; itemNo < validN; itemNo++)
			validPreds[itemNo] += shrinkage * tree.predict(itemNo, VALID);

		//output
		frmscurve.open("boosting_rms.txt", ios_base::out | ios_base::app); 
		frmscurve << rmse(validPreds, validTar) << endl;
		frmscurve.close();
		
		if(!ti.rms)
		{
			froccurve.open("boosting_roc.txt", ios_base::out | ios_base::app); 
			froccurve << roc(validPreds, validTar) << endl;
			froccurve.close();
		}

		if(treeNo % 100 == 0)
			cout << "\titeration " << treeNo + 1 << " out of " << treeN << endl;
	}

//------------------

		}catch(TE_ERROR err){
		te_errMsg((TE_ERROR)err);
		return 1;
	}catch(BT_ERROR err){
		ErrLogStream errlog;
		switch(err) 
		{
			case INPUT_ERR:
				errlog << "Usage: gbt_train -t _train_set_ -v _validation_set_ -r _attr_file_" 
					<< "[-a _alpha_value_] [-n _boosting_iterations_] [-i _init_random_] [-c rms|roc]"
					<< " [-sh _shrinkage_ ] [-sub _subsampling_]\n";
				break;
			case ALPHA_ERR:
				errlog << "Error: alpha value is out of [0;1] range.\n";
				break;
			case WIN_ERR:
				errlog << "Input error: TreeExtra currently does not support multithreading for Windows.\n"; 
				break;
			default:
				throw err;
		}
		return 1;
	}catch(exception &e){
		ErrLogStream errlog;
		string errstr(e.what());
		errlog << "Error: " << errstr << "\n";
		return 1;
	}catch(...){
		string errstr = strerror(errno);
		ErrLogStream errlog;
		errlog << "Error: " << errstr << "\n";
		return 1;
	}
	return 0;
}