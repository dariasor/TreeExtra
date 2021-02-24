//Gradient boosting optimizing RMS. gbt_train.cpp: main function of executable gbt_train

//(c) Daria Sorokina

//gbt_train -t _train_set_ -v _validation_set_ -r _attr_file_ 
//[-a _alpha_value_] [-n _boosting_iterations_] [-i _init_random_] [-c rms|roc]
// [-sh _shrinkage_ ] [-sub _subsampling_] | -version

#include "Tree.h"
#include "bt_functions.h"
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
//0. -version mode	
	if((argc > 1) && !string(argv[1]).compare("-version"))
	{
		LogStream telog;
		telog << "\n-----\nbt_train ";
		for(int argNo = 1; argNo < argc; argNo++)
			telog << argv[argNo] << " ";
		telog << "\n\n";

		telog << "TreeExtra version " << VERSION << "\n";
			return 0;
	}

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
	int topAttrN = 0;  //how many top attributes to output and keep in the cut data 
						//(0 = do not do feature selection)
						//(-1 = output all available features)
	bool doOut = true; //whether to output log information to stdout

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
		else if(!args[argNo].compare("-k"))
			topAttrN = atoiExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-l"))
		{
			if(!args[argNo + 1].compare("log"))
				doOut = true;
			else if(!args[argNo + 1].compare("nolog"))
				doOut = false;
			else
				throw INPUT_ERR;
		}
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
	LogStream telog;
	LogStream::init(doOut);
	telog << "\n-----\ngbt_train ";
	for(int argNo = 1; argNo < argc; argNo++)
		telog << argv[argNo] << " ";
	telog << "\n\n";

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
	doublev validTar, validWt;
	int validN = data.getTargets(validTar, validWt, VALID);

	doublev rmsV(treeN, 0); 				//learning curve of rms values for the validation set
	doublev rocV;							 
	if(!ti.rms)
		rocV.resize(treeN, 0);				//learning curve of roc values for the validation set
	doublev predsumsV(validN, 0); 			//sums of predictions for each data point

	int attrN = data.getAttrN();
	if(topAttrN == -1)
		topAttrN = attrN;
	doublev attrCounts(attrN, 0); //counts of attribute importance
	idpairv attrCountsP; //another structure for counts of attribute importance, will need it later for sorting
	bool doFS = (topAttrN != 0);	//whether feature selection is requested

	fstream frmscurve("boosting_rms.txt", ios_base::out); //bagging curve (rms)
	frmscurve.close();
	fstream froccurve;
	if(!ti.rms)
	{
		froccurve.open("boosting_roc.txt", ios_base::out); //bagging curve (roc) 
		froccurve.close();
	}

	doublev trainTar, trainWt;
	int trainN = data.getTargets(trainTar, trainWt, TRAIN);

	int sampleN;
	if(subsample == -1)
		sampleN = trainN;
	else
		sampleN = (int) (trainN * subsample);
	
	doublev validPreds(validN, 0);
	doublev trainPreds(trainN, 0);
	
	for(int treeNo = 0; treeNo < treeN; treeNo++)
	{
		if(doOut && (treeNo % 10 == 0))
			cout << "\titeration " << treeNo + 1 << " out of " << treeN << endl;

		// XW
		INDsample sample(data);
		if(subsample == -1)
			sample.newBag();
		else
			sample.newSample(sampleN);

		CTree tree(ti.alpha);
		tree.setRoot(sample); // XW
		tree.resetRoot(trainPreds);
		tree.growGBT(doFS, attrCounts, sample); // XW

		//update predictions
		for(int itemNo = 0; itemNo < trainN; itemNo++)
			trainPreds[itemNo] += shrinkage * tree.predict(itemNo, TRAIN);
		for(int itemNo = 0; itemNo < validN; itemNo++)
			validPreds[itemNo] += shrinkage * tree.predict(itemNo, VALID);

		rmsV[treeNo] = rmse(validPreds, validTar, validWt);
		if(!ti.rms)
			rocV[treeNo] = roc(validPreds, validTar, validWt);

		//output
		frmscurve.open("boosting_rms.txt", ios_base::out | ios_base::app); 
		frmscurve << rmsV[treeNo] << endl;
		frmscurve.close();
		
		if(!ti.rms)
		{
			froccurve.open("boosting_roc.txt", ios_base::out | ios_base::app); 
			froccurve << rocV[treeNo] << endl;
			froccurve.close();
		}

	}
	//4. Output

	//output results 
	if(ti.rms)
		telog << "RMSE on validation set = " << rmsV[treeN - 1] << "\n";
	else
		telog << "ROC on validation set = " << rocV[treeN - 1] << "\n";

	//standard output in case of turned off log output: final performance on validation set only
	if(!doOut)
		if(ti.rms)
			cout << rmsV[treeN - 1] << endl;
		else
			cout << rocV[treeN - 1] << endl;
	
	//output feature selection results
	if(doFS)
	{
		for(int attrNo = 0; attrNo < attrN; attrNo++)
		{
			attrCountsP[attrNo].first = attrNo;	//number of attribute	
			attrCountsP[attrNo].second = attrCounts[attrNo];		//counts
		}
		sort(attrCountsP.begin(), attrCountsP.end(), idGreater);
		if(topAttrN > attrN)
			topAttrN = attrN;

		fstream ffeatures("feature_scores.txt", ios_base::out);
		ffeatures << "Top " << topAttrN << " features\n";
		for(int attrNo = 0; attrNo < topAttrN; attrNo++)
			ffeatures << data.getAttrName(attrCountsP[attrNo].first) << "\t"
			<< attrCountsP[attrNo].second / treeN << "\n";
		ffeatures << "\n\nColumn numbers (beginning with 1)\n";
		for(int attrNo = 0; attrNo < topAttrN; attrNo++)
			ffeatures << data.getColNo(attrCountsP[attrNo].first) + 1 << " ";
		ffeatures << "\nLabel column number: " << data.getTarColNo() + 1;
		ffeatures.close();

		//output new attribute file
		for(int attrNo = topAttrN; attrNo < attrN; attrNo++)
			data.ignoreAttr(attrCountsP[attrNo].first);
		data.outAttr(ti.attrFName);
	}

	//output predictions
	fstream fpreds;
	fpreds.open("preds.txt", ios_base::out);
	for(int itemNo = 0; itemNo < validN; itemNo++)
		fpreds << validPreds[itemNo] << endl;
	fpreds.close();

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
					<< " [-sh _shrinkage_ ] [-sub _subsampling_] | -version\n";
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
