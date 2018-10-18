//Bagged Trees / bt_predict.cpp: main function of executable bt_predict
//
//(c) Daria Sorokina

#include <errno.h>

#include "LogStream.h"
#include "ErrLogStream.h"
#include "Tree.h"
#include "bt_definitions.h"
#include "TrainInfo.h"
#include "functions.h"


//bt_predict -p _test_set_ -r _attr_file_ [-m _model_file_name_] [-o _output_file_name_] [-c rms|roc]
//[-l log|nolog] | -version
int main(int argc, char* argv[])
{	 
	try{
//0. -version mode	
	if((argc > 1) && !string(argv[1]).compare("-version"))
	{
		LogStream telog;
		telog << "\n-----\nbt_predict ";
		for(int argNo = 1; argNo < argc; argNo++)
			telog << argv[argNo] << " ";
		telog << "\n\n";

		telog << "TreeExtra version " << VERSION << "\n";
			return 0;
	}

//1. Analyze input parameters
	string modelFName = "model.bin";	//name of the input file for the model
	string predFName = "preds.txt";		//name of the output file for predictions
	bool doOut = true; //whether to output log information to stdout
	
	TrainInfo ti;

	//check that the number of arguments is even (flags + value pairs)
	if(argc % 2 == 0)
		throw INPUT_ERR;
	//convert input parameters to string from char*
	stringv args(argc); 
	for(int argNo = 0; argNo < argc; argNo++)
		args[argNo] = string(argv[argNo]);

	//parse and save input parameters
	//indicators of presence of required flags in the input
	bool hasTest = false; 
	bool hasAttr = false;
	
	for(int argNo = 1; argNo < argc; argNo += 2)
	{
		if(!args[argNo].compare("-m"))
			modelFName = args[argNo + 1];
		else if(!args[argNo].compare("-o"))
			predFName = args[argNo + 1];
		else if(!args[argNo].compare("-p"))
		{
			ti.testFName = args[argNo + 1];
			hasTest = true;
		}
		else if(!args[argNo].compare("-r"))
		{
			ti.attrFName = args[argNo + 1];
			hasAttr = true;
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
		else if(!args[argNo].compare("-l"))
		{
			if(!args[argNo + 1].compare("log"))
				doOut = true;
			else if(!args[argNo + 1].compare("nolog"))
				doOut = false;
			else
				throw INPUT_ERR;
		}
		else
			throw INPUT_ERR;
	}

	if(!(hasTest && hasAttr))
		throw INPUT_ERR;

//1a. Set log file
	LogStream telog;
	LogStream::doOut = doOut;
	telog << "\n-----\nbt_predict ";
	for(int argNo = 1; argNo < argc; argNo++)
		telog << argv[argNo] << " ";
	telog << "\n\n";

//2. Load data
	INDdata data(ti.trainFName.c_str(), ti.validFName.c_str(), ti.testFName.c_str(), 
				 ti.attrFName.c_str(), doOut);
	CTree::setData(data);
	CTreeNode::setData(data);

//3. Open model file
	fstream fmodel(modelFName.c_str(), ios_base::binary | ios_base::in);
	//read AG header
	AG_TRAIN_MODE mode;
	int tigN;
	double alpha;
	fmodel.read((char*) &mode, sizeof(enum AG_TRAIN_MODE));
	fmodel.read((char*) &tigN, sizeof(int));
	fmodel.read((char*) &alpha, sizeof(double));
	
	if(fmodel.fail() || (mode != SLOW) || (tigN != 1))
		throw MODEL_ERR;

//4. Load models, get predictions	
	doublev testTar, testWt;
	int testN = data.getTargets(testTar, testWt, TEST);
	doublev preds(testN, 0);

	ti.bagN = 0;
	while(fmodel.peek() != char_traits<char>::eof())
	{//load next Grove in the ensemble 
		ti.bagN++;
		if(doOut)
			cout << "Iteration " << ti.bagN << endl;
		CTree tree;
		tree.load(fmodel);

		//get predictions, add them to predictions of previous models
		for(int itemNo = 0; itemNo < testN; itemNo++)
			preds[itemNo] += tree.predict(itemNo, TEST);
	}

	//get bagged predictions of the ensemble
	for(int itemNo = 0; itemNo < testN; itemNo++)
		preds[itemNo] /= ti.bagN;
	
//5. Output predictions into the output file and performance on test set (if available) to std output
	fstream fpreds;
	fpreds.open(predFName.c_str(), ios_base::out); 
	for(int itemNo = 0; itemNo < testN; itemNo++)
		fpreds << preds[itemNo] << endl;
	fpreds.close();

	if(data.hasTrueTest())
	{
		double performance = -1;
		if(ti.rms)
		{
			performance = rmse(preds, testTar, testWt);
			telog << "RMSE: " << performance << "\n";
		}
		else
		{
			performance = roc(preds, testTar, testWt);
			telog << "ROC: " << performance << "\n";
		}
		if(!doOut)
			cout << performance << endl;
	}

	
	}catch(TE_ERROR err){
		te_errMsg((TE_ERROR)err);
		return 1;
	}catch(BT_ERROR err){
		ErrLogStream errlog;
		switch(err) 
		{
			case INPUT_ERR:
				errlog << "Usage: bt_predict -p _test_set_ -r _attr_file_name_ "
					<< "[-m _model_file_name_] [-o _output_file_name_] [-c rms|roc] [-l log|nolog] | -version\n";
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
