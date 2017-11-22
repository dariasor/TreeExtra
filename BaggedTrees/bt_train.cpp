//Bagged Trees / bt_train.cpp: main function of executable bt_train
//
//(c) Daria Sorokina

//bt_train -t _train_set_ -v _validation_set_ -r _attr_file_ 
//[-a _alpha_value_] [-b _bagging_iterations_] [-i _init_random_] [-m_model_file_name_]
//[-k _attributes_to_leave_] [-l log|nolog] [-c rms|roc] | -version

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
#include <cmath>

int main(int argc, char* argv[])
{	
	try{
//0. -version mode	
	if((argc > 1) && !string(argv[1]).compare("-version"))
	{
		LogStream clog;
		clog << "\n-----\nbt_train ";
		for(int argNo = 1; argNo < argc; argNo++)
			clog << argv[argNo] << " ";
		clog << "\n\n";

		clog << "TreeExtra version " << VERSION << "\n";
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
	string modelFName = "model.bin";	//name of the output file for the model
	string predFName = "preds.txt";		//name of the output file for predictions
	int topAttrN = 0;  //how many top attributes to output and keep in the cut data 
							//(0 = do not do feature selection)
							//(-1 = output all available features)
	bool doOut = true; //whether to output log information to stdout

	//parse and save input parameters
	//indicators of presence of required flags in the input
	bool hasTrain = false;
	bool hasVal = false; 
	bool hasAttr = false; 

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
		else if(!args[argNo].compare("-b"))
			ti.bagN = atoiExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-i"))
			ti.seed = atoiExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-k"))
			topAttrN = atoiExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-m"))
		{
			modelFName = args[argNo + 1];
			if(modelFName.empty())
				throw EMPTY_MODEL_NAME_ERR;
		}
		else if(!args[argNo].compare("-o"))
			predFName = args[argNo + 1];
		else if(!args[argNo].compare("-l"))
		{
			if(!args[argNo + 1].compare("log"))
				doOut = true;
			else if(!args[argNo + 1].compare("nolog"))
				doOut = false;
			else
				throw INPUT_ERR;
		}
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
	LogStream::init(doOut);
	clog << "\n-----\nbt_train ";
	for(int argNo = 1; argNo < argc; argNo++)
		clog << argv[argNo] << " ";
	clog << "\n\n";

//1.b) Initialize random number generator. 
	srand(ti.seed);

//2. Load data
	INDdata data(ti.trainFName.c_str(), ti.validFName.c_str(), ti.testFName.c_str(), 
				 ti.attrFName.c_str(), doOut);
	CTree::setData(data);
	CTreeNode::setData(data);

//2.a) Start thread pool
#ifndef _WIN32
	TThreadPool pool(threadN);
	CTree::setPool(pool);
#endif

//3. Train models
	doublev validTar;
	int validN = data.getTargets(validTar, VALID);
	int itemN = data.getTrainN();

	//adjust minAlpha, if needed
	double newAlpha = adjustAlpha(ti.alpha, itemN);
	if(ti.alpha != newAlpha)
	{
		if(newAlpha == 0)
			clog << "Warning: due to small train set size value of alpha was changed to 0"; 
		else 
			clog << "Warning: alpha value was rounded to the closest valid value " << newAlpha;
		clog << ".\n\n";
		ti.alpha = newAlpha;	
	}
	clog << "Alpha = " << ti.alpha << "\n" 
		<< ti.bagN << " bagging iterations\n";

	doublev rmsV(ti.bagN, 0); 				//bagging curve of rms values for validation set
	doublev rocV;							 
	if(!ti.rms)
		rocV.resize(ti.bagN, 0);			//bagging curve of roc values for validation set
	doublev predsumsV(validN, 0); 			//sums of predictions for each data point

	int attrN = data.getAttrN();
	if(topAttrN == -1)
		topAttrN = attrN;
	idpairv attrCounts;	//counts of attribute importance
	bool doFS = (topAttrN != 0);	//whether feature selection is requested
	if(doFS)
	{//initialize attrCounts
		attrCounts.resize(attrN);
		for(int attrNo = 0; attrNo < attrN; attrNo++)
		{
			attrCounts[attrNo].first = attrNo;	//number of attribute	
			attrCounts[attrNo].second = 0;		//counts
		}
	}
	fstream fmodel(modelFName.c_str(), ios_base::binary | ios_base::out);
	//header for compatibility with Additive Groves model
	AG_TRAIN_MODE modeStub = SLOW;
	fmodel.write((char*) &modeStub, sizeof(enum AG_TRAIN_MODE));
	int tigNStub = 1;
	fmodel.write((char*) &tigNStub, sizeof(int));
	fmodel.write((char*) &ti.alpha, sizeof(double));
	fmodel.close();
	
	fstream fbagrms("bagging_rms.txt", ios_base::out); //bagging curve (rms)
	fbagrms.close();
	fstream fbagroc;
	if(!ti.rms)
	{
		fbagroc.open("bagging_roc.txt", ios_base::out); //bagging curve (roc) 
		fbagroc.close();
	}

	//make bags, build trees, collect predictions
	doublev predictions(validN);
	for(int bagNo = 0; bagNo < ti.bagN; bagNo++)
	{
		if(doOut)
			cout << "Iteration " << bagNo + 1 << " out of " << ti.bagN << endl;

		data.newBag();
		CTree tree(ti.alpha);
		tree.setRoot();
		tree.grow(doFS, attrCounts);
		tree.save(modelFName.c_str());

		//generate predictions for validation set
		for(int itemNo = 0; itemNo < validN; itemNo++)
		{
			predsumsV[itemNo] += tree.predict(itemNo, VALID);
			predictions[itemNo] = predsumsV[itemNo] / (bagNo + 1);
		}
		rmsV[bagNo] = rmse(predictions, validTar);
		if(!ti.rms)
			rocV[bagNo] = roc(predictions, validTar);

		//output an element of bagging curve 
		fbagrms.open("bagging_rms.txt", ios_base::out | ios_base::app); 
		fbagrms << rmsV[bagNo] << endl;
		fbagrms.close();

		//same for roc, if needed
		if(!ti.rms)
		{
			fbagroc.open("bagging_roc.txt", ios_base::out | ios_base::app); 
			fbagroc << rocV[bagNo] << endl;
			fbagroc.close();
		}
	}

	if(doFS)	//sort attributes by counts
		sort(attrCounts.begin(), attrCounts.end(), idGreater);
	
//4. Output
		
	//output results 
	if(ti.rms)
		clog << "RMSE on validation set = " << rmsV[ti.bagN - 1] << "\n";
	else
		clog << "ROC on validation set = " << rocV[ti.bagN - 1] << "\n";

	//output predictions into the output file
	fstream fpreds;
	fpreds.open(predFName.c_str(), ios_base::out);
	for(int itemNo = 0; itemNo < validN; itemNo++)
		fpreds << predictions[itemNo] << endl;
	fpreds.close();

	//analyze whether more bagging should be recommended based on the curve in the best point
	if(moreBag(rmsV))
	{
		int recBagN = roundInt(round(ti.bagN * 1.5));
		clog << "\nRecommendation: a greater number of bagging iterations might produce a better model.\n"
			<< "Suggested action: bt_train -b " << recBagN << "\n";
	}
	else
		clog << "\nThe bagging curve shows good convergence. \n"; 
	clog << "\n";

	//standard output in case of turned off log output: final performance on validation set only
	if(!doOut)
		if(ti.rms)
			cout << rmsV[ti.bagN - 1] << endl;
		else
			cout << rocV[ti.bagN - 1] << endl;

	//output feature selection results
	if(doFS)
	{
		if(topAttrN > attrN)
			topAttrN = attrN;

		fstream ffeatures("feature_scores.txt", ios_base::out);	
		ffeatures << "Top " << topAttrN << " features\n";
		for(int attrNo = 0; attrNo < topAttrN; attrNo++)
			ffeatures << data.getAttrName(attrCounts[attrNo].first) << "\t" 
				<< attrCounts[attrNo].second / ti.bagN / itemN << "\n";
		ffeatures << "\n\nColumn numbers (beginning with 1)\n";
		for(int attrNo = 0; attrNo < topAttrN; attrNo++)
			ffeatures << data.getColNo(attrCounts[attrNo].first) + 1 << " ";
		ffeatures << "\nLabel column number: " << data.getTarColNo() + 1;
		ffeatures.close();

		//output new attribute file
		for(int attrNo = topAttrN; attrNo < attrN; attrNo++)
			data.ignoreAttr(attrCounts[attrNo].first);
		data.outAttr(ti.attrFName);
	}

	}catch(TE_ERROR err){
		te_errMsg((TE_ERROR)err);
		return 1;
	}catch(BT_ERROR err){
		ErrLogStream errlog;
		switch(err) 
		{
			case INPUT_ERR:
				errlog << "Usage: bt_train -t _train_set_ -v _validation_set_ -r _attr_file_ "
					<< "[-a _alpha_value_] [-b _bagging_iterations_] [-i _init_random_] " 
					<< "[-m _model_file_name_] [-k _attributes_to_leave_] [-c rms|roc] "
					<< "[-l log|nolog] | -version\n";
				break;
			case ALPHA_ERR:
				errlog << "Error: alpha value is out of [0;1] range.\n";
				break;
			case WIN_ERR:
				errlog << "Input error: TreeExtra does not support multithreading for Windows.\n"; 
				break;
			default:
				throw err;
		}
		return 1;
	}catch(exception &e){
		ErrLogStream errlog;
		string errstr(e.what());
		exception_errMsg(errstr);
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
