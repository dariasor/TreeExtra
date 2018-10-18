//Additive Groves / ag_predict.cpp: main function of executable ag_predict
//
//(c) Daria Sorokina

#include "Grove.h"
#include "TrainInfo.h"
#include "LogStream.h"
#include "ErrLogStream.h"
#include "functions.h"
#include "ag_definitions.h"

#include <errno.h>

//ag_predict -p _test_set_ -r _attr_file_ [-m _model_file_name_] [-o _output_file_name_] [-c rms|roc] | -version
int main(int argc, char* argv[])
{	 
	try{
	//0. Set log file
	LogStream telog;
	telog << "\n-----\nag_predict ";
	for(int argNo = 1; argNo < argc; argNo++)
		telog << argv[argNo] << " ";
	telog << "\n\n";
	
	if((argc > 1) && !string(argv[1]).compare("-version"))
	{
		telog << "TreeExtra version " << VERSION << "\n";
		return 0;
	}

	//1. Set default values of parameters
	string modelFName = "model.bin";	//name of the input file for the model
	string predFName = "preds.txt";		//name of the output file for predictions

	TrainInfo ti;

	//2. Set parameters from command line
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
		else
			throw INPUT_ERR;
	}

	if(!(hasTest && hasAttr))
		throw INPUT_ERR;

//2. Load data
	INDdata data(ti.trainFName.c_str(), ti.validFName.c_str(), ti.testFName.c_str(), 
				 ti.attrFName.c_str());
	CGrove::setData(data);
	CTreeNode::setData(data);

//3. Open model file, read its header
	fstream fmodel(modelFName.c_str(), ios_base::binary | ios_base::in);
	fmodel.read((char*) &ti.mode, sizeof(enum AG_TRAIN_MODE));
	if(ti.mode == FAST)
	{//skip information about fast training - it is not used in this command
		int dirN = 0;
		fmodel.read((char*) &dirN, sizeof(int));
		bool dirStub = false;
		for(int dirNo = 0; dirNo < dirN; dirNo++)
			fmodel.read((char*) &dirStub, sizeof(bool));
	}
	fmodel.read((char*) &ti.maxTiGN, sizeof(int));
	fmodel.read((char*) &ti.minAlpha, sizeof(double));
	if(fmodel.fail() || (ti.maxTiGN < 1))
		throw MODEL_ERR;

//4. Load models, get predictions	
	doublev testTar, testWt;
	int testN = data.getTargets(testTar, testWt, TEST);
	doublev preds(testN, 0);

	ti.bagN = 0;
	cout << "Calculating predictions " << endl;
	while(fmodel.peek() != char_traits<char>::eof())
	{//load next Grove in the ensemble 
		ti.bagN++;
		cout << "Iteration " << ti.bagN << endl;
		CGrove grove(ti.minAlpha, ti.maxTiGN);
		grove.load(fmodel);

		//get predictions, add them to predictions of previous models
		for(int itemNo = 0; itemNo < testN; itemNo++)
			preds[itemNo] += grove.predict(itemNo, TEST);
	}

	//get bagged predictions of the ensemble
	for(int itemNo = 0; itemNo < testN; itemNo++)
		preds[itemNo] /= ti.bagN;
	
//5. Output predictions into the output file and performance value (if available) to std output
	fstream fpreds;
	fpreds.open(predFName.c_str(), ios_base::out); 
	for(int itemNo = 0; itemNo < testN; itemNo++)
		fpreds << preds[itemNo] << endl;
	fpreds.close();

	if(data.hasTrueTest())
	{
		double performance;
		if(ti.rms)
		{
			performance = rmse(preds, testTar, testWt);
			telog << "\nRMSE: " << performance << "\n";
		}
		else
		{
			performance = roc(preds, testTar, testWt);
			telog << "\nROC: " << performance << "\n";
		}
	}
	
	}catch(TE_ERROR err){
		te_errMsg((TE_ERROR)err);
		return 1;
	}catch(AG_ERROR err){
		ErrLogStream errlog;
		switch(err) 
		{
			case INPUT_ERR:
				errlog << "Usage: ag_predict -p _test_set_ -r _attr_file_name_ "
					<< "[-m _model_file_name_] [-o _output_file_name_] [-c rms|roc] | -version\n";
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
}
