//Additive Groves / ag_interactions.cpp: main function of executable ag_interactions
//
//(c) Daria Sorokina

//ag_nway -t _train_set_ -v _validation_set_ -r _attr_file_ -a _alpha_value_ -n _N_value_ 
//		-b _bagging_iterations_ -ave _mean_performance_ -std _std_of_performance_ -w _interaction_file_
//		[-c rms|roc] [-i _seed_] [-m _model_file_name_]  | -version

#include "ag_definitions.h"
#include "functions.h"
#include "ag_functions.h"
#include "Grove.h"
#include "LogStream.h"
#include "ErrLogStream.h"
#include <errno.h>

#ifndef _WIN32
#include "thread_pool.h"
#endif

int main(int argc, char* argv[])
{	
	try{
//0. Set log file
	LogStream telog;
	telog << "\n-----\nag_nway ";
	for(int argNo = 1; argNo < argc; argNo++)
		telog << argv[argNo] << " ";
	telog << "\n\n";
	
	if((argc > 1) && !string(argv[1]).compare("-version"))
	{
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

	TrainInfo ti;
	double meanPerf = -1;
	double stdPerf = -1;
	string nwayFName; //file with feature names that are involved in the interaction to test
	string modelFName = "restricted_model.bin";	//name of the output file for the model
	ti.mode = LAYERED;

#ifndef _WIN32
	int threadN = 6;	//number of threads
#endif

	//parse and save input parameters
	//indicators of presence of required flags in the input
	bool hasTrain = false;
	bool hasVal = false; 
	bool hasAttr = false; 
	bool hasAlpha = false;
	bool hasTiGN = false;
	bool hasBagN = false;
	bool hasMean = false;
	bool hasStD = false;
	bool hasNWay = false;

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
		{
			ti.minAlpha = atofExt(argv[argNo + 1]);
			hasAlpha = true;
		}
		else if(!args[argNo].compare("-n"))
		{
			ti.maxTiGN = atoiExt(argv[argNo + 1]);
			hasTiGN = true;
		}
		else if(!args[argNo].compare("-b"))
		{
			ti.bagN = atoiExt(argv[argNo + 1]);
			hasBagN = true;
		}
		else if(!args[argNo].compare("-ave"))
		{
			meanPerf = atofExt(argv[argNo + 1]);
			hasMean = true;
		}
		else if(!args[argNo].compare("-std"))
		{
			stdPerf = atofExt(argv[argNo + 1]);
			hasStD = true;
		}
		else if(!args[argNo].compare("-w"))
		{
			nwayFName = args[argNo + 1];
			hasNWay = true;
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
		else if(!args[argNo].compare("-i"))
			ti.seed = atoiExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-m"))
		{
			modelFName = args[argNo + 1];
			if(modelFName.empty())
				throw EMPTY_MODEL_NAME_ERR;
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

	if(!(hasTrain && hasVal && hasAttr && hasAlpha && hasTiGN && hasBagN && hasMean && hasStD && hasNWay))
		throw INPUT_ERR;

	if(ti.trainFName.compare(ti.validFName) == 0)
		throw TRAIN_EQ_VALID_ERR;

	if((ti.minAlpha < 0) || (ti.minAlpha > 1))
		throw ALPHA_ERR;

	if(ti.maxTiGN < 1)
		throw TIGN_ERR;

//1.b) Initialize random number generator. 
	srand(ti.seed);

//2. Load data
	INDdata data(ti.trainFName.c_str(), ti.validFName.c_str(), ti.testFName.c_str(), ti.attrFName.c_str());
	CGrove::setData(data);
	CTreeNode::setData(data);

	doublev validTar, validWt;
	int validN = data.getTargets(validTar, validWt, VALID);
	int itemN = data.getTrainN();

	//adjust minAlpha, if needed
	double newAlpha = adjustAlpha(ti.minAlpha, itemN);
	if(ti.minAlpha != newAlpha)
	{
		if(newAlpha == 0)
			telog << "Warning: due to small train set size value of alpha was changed to 0"; 
		else 
			telog << "Warning: alpha value was rounded to the closest valid value " << newAlpha;
		telog << ".\n\n";
		ti.minAlpha = newAlpha;	
	}
	//adjust maxTiGN, if needed
	int newTiGN = adjustTiGN(ti.maxTiGN);
	if(ti.maxTiGN != newTiGN)
	{
		telog << "Warning: N value was rounded to the closest smaller valid value " << newTiGN << ".\n\n";
		ti.maxTiGN = newTiGN;	
	}
	telog << "Alpha = " << ti.minAlpha << "\nN = " << ti.maxTiGN << "\n" 
		<< ti.bagN << " bagging iterations\n";

//2.a) Start thread pool
#ifndef _WIN32
	TThreadPool pool(threadN);
	CGrove::setPool(pool);
#endif

//3. Main part - run interaction detection

	//read interaction features
	fstream fnway;
	fnway.open(nwayFName.c_str(), ios_base::in);
	if(!fnway) 
		throw OPEN_NWAY_ERR;
	string nwayAttr;
	fnway >> nwayAttr;
	while(!fnway.fail())
	{
		int nwayAttrNo = data.getAttrId(nwayAttr);
		if(!data.isActive(nwayAttrNo))
			throw ATTR_NAME_ERR;
		ti.interaction.push_back(nwayAttrNo);
		fnway >> nwayAttr;
	}

	//test interaction
	telog << "\nRestricting interaction between "; 
	for(int nwayNo = 0; nwayNo < (int)ti.interaction.size() - 1; nwayNo++)
		telog << data.getAttrName(ti.interaction[nwayNo]) << ", "; 
	telog << data.getAttrName(ti.interaction[ti.interaction.size() - 1]) << "\n";
		
	double rPerf = layeredGroves(data, ti, modelFName);
	double score = (meanPerf - rPerf) / stdPerf;
	if(ti.rms)
		score *= -1; 
	telog << "\tPerformance: " << rPerf << ". " << score << " standard deviations from the mean. ";
	if(score > 3)
		telog << "Interaction is present.\n";
	else
		telog << "Interaction is absent.\n";

	}catch(TE_ERROR err){
		te_errMsg((TE_ERROR)err);
		return 1;
	}catch(AG_ERROR err){
		ErrLogStream errlog;
		switch(err)
		{
			case INPUT_ERR:
				errlog << "Usage: ag_nway -t _train_set_ -v _validation_set_ -r _attr_file_ "
					<< "-a _alpha_value_ -n _N_value_ -b _bagging_iterations_ -ave _mean_performance_ "
					<< "-std _std_of_performance_ -w _interaction_file_ [-i _init_random_] [-c rms|roc] "
					<< "[-m _model_file_name_] | -version\n";
				break;
			case ALPHA_ERR:
				errlog << "Input error: alpha value is out of [0;1] range.\n";
				break;
			case TIGN_ERR:
				errlog << "Input error: N value is less than 1.\n"; 
				break;
			case OPEN_NWAY_ERR:
				errlog << "Error: failed to open interaction file. This file should list features involved"
					<< " in the interaction that we are testing.\n";
				break;
			case ATTR_NAME_ERR:
				errlog << "Error: attribute name misspelled or the attribute is not active.\n";
				break;
			case TRAIN_EQ_VALID_ERR:
				errlog << "Error: train set should be different from the validation set.\n"; 
				break;
			case WIN_ERR:
				errlog << "Input error: TreeExtra does not support multithreading for Windows.\n"; 
				break;
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
