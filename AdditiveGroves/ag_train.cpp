//Additive Groves / ag_train.cpp: main function of executable ag_train
//
//(c) Daria Sorokina

#include "ag_definitions.h"
#include "functions.h"
#include "ag_functions.h"
#include "Grove.h"
#include "LogStream.h"
#include "ErrLogStream.h"

#ifndef _WIN32
#include "thread_pool.h"
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include <errno.h>



//ag_train -t _train_set_ -v _validation_set_ -r _attr_file_ [-a _alpha_value_] [-n _N_value_] 
//		[-b _bagging_iterations_] [-s slow|fast|layered] [-c rms|roc] [-i seed] | -version
int main(int argc, char* argv[])
{	
	try{
//0. Set log file
	LogStream clog;
	LogStream::init(true);
	clog << "\n-----\nag_train ";
	for(int argNo = 1; argNo < argc; argNo++)
		clog << argv[argNo] << " ";
	clog << "\n\n";
	
	if((argc > 1) && !string(argv[1]).compare("-version"))
	{
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

	TrainInfo ti;
#ifndef _WIN32
	int threadN = 6;	//number of threads
#endif

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
			ti.minAlpha = atofExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-n"))
			ti.maxTiGN = atoiExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-b"))
			ti.bagN = atoiExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-s"))
		{
			if(!args[argNo + 1].compare("slow"))
				ti.mode = SLOW;
			else if(!args[argNo + 1].compare("fast"))
				ti.mode = FAST;
			else if(!args[argNo + 1].compare("layered"))
				ti.mode = LAYERED;
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
		else if(!args[argNo].compare("-i"))
			ti.seed = atoiExt(argv[argNo + 1]);
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

	if((ti.minAlpha < 0) || (ti.minAlpha > 1))
		throw ALPHA_ERR;

	if(ti.maxTiGN < 1)
		throw TIGN_ERR;

//1.a) delete all temp files from the previous run and create a directory AGTemp
#ifdef WIN32	//in windows
	WIN32_FIND_DATA fn;			//structure that will contain the name of file
	HANDLE hFind;				//current file
	hFind = FindFirstFile("./AGTemp/*.*", &fn);	//"."	
	FindNextFile(hFind, &fn);						//".." 
	//delete all files in the directory
	while(FindNextFile(hFind, &fn) != 0) 
	{
		string fullName = "AGTemp/" + (string)fn.cFileName;
		DeleteFile(fullName.c_str());
	} 
	CreateDirectory("AGTemp", NULL);

#else 
	system("rm -rf ./AGTemp/");
	system("mkdir ./AGTemp/");
#endif


//1.b) Initialize random number generator. 
	srand(ti.seed);

//2. Load data
	INDdata data(ti.trainFName.c_str(), ti.validFName.c_str(), ti.testFName.c_str(), ti.attrFName.c_str());
	CGrove::setData(data);
	CTreeNode::setData(data);

//2.a) Start thread pool
#ifndef _WIN32
	TThreadPool pool(threadN);
	CGrove::setPool(pool);
#endif
	
//3. Train models
	doublev validTar;
	int validN = data.getTargets(validTar, VALID);
	int itemN = data.getTrainN();
	double trainV = data.getTrainV();

	//adjust minAlpha, if needed
	double newAlpha = adjustAlpha(ti.minAlpha, trainV);
	if(!eqDouble(ti.minAlpha, newAlpha))
	{
		if(newAlpha == 0)
		  clog << "Warning: due to the small train set size (" << trainV << ") the value of alpha was changed to 0"; 
		else 
			clog << "Warning: alpha value was rounded to the closest valid value - " << newAlpha;
		clog << ".\n\n";
		ti.minAlpha = newAlpha;	
	}
	//adjust maxTiGN, if needed
	int newTiGN = adjustTiGN(ti.maxTiGN);
	if(ti.maxTiGN != newTiGN)
	{
		clog << "Warning: N value was rounded to the closest smaller valid value " << newTiGN << ".\n\n";
		ti.maxTiGN = newTiGN;	
	}
	clog << "Alpha = " << ti.minAlpha << "\nN = " << ti.maxTiGN << "\n" 
		<< ti.bagN << " bagging iterations\n";
	if(ti.mode == FAST)
		clog << "fast mode\n\n";
	else if(ti.mode == SLOW)
		clog << "slow mode\n\n";
	else //if(ti.mode == LAYERED)
		clog << "layered mode\n\n";

	int alphaN = getAlphaN(ti.minAlpha, trainV); //number of different alpha values
	int tigNN = getTiGNN(ti.maxTiGN); //number of different tigN values

	//surfaces of performance values for validation set. 
	//Always calculate rms (for convergence analysis), if needed, calculate roc
	doublevvv rmsV(tigNN, doublevv(alphaN, doublev(ti.bagN, 0))); 
	doublevvv rocV;
	if(!ti.rms)
		rocV.resize(tigNN, doublevv(alphaN, doublev(ti.bagN, 0))); 
	//outer array: column (by TiGN)
	//middle array: row (by alpha)
	//inner array: bagging iterations. Performance is kept for all iterations to create bagging curves

	//sums of predictions for each data point (raw material to calculate performance values)
	doublevvv predsumsV(tigNN, doublevv(alphaN, doublev(validN, 0)));
	//outer array: column (by TiGN)
	//middle array: row	(by alpha)
	//inner array: data points in the validation set

	//direction of initialization (1 - up, 0 - right), used in fast mode only
	doublevv dir(tigNN, doublev(alphaN, 0)); 
	//outer array: column (by TiGN)
	//middle array: row	(by alpha)

	//direction of initialization (1 - up, 0 - right), collects statistics in the slow mode
	doublevv dirStat(tigNN, doublev(alphaN, 0));

	//make bags, build trees, collect predictions
	for(int bagNo = 0; bagNo < ti.bagN; bagNo++)
	{
		cout << "Iteration " << bagNo + 1 << " out of " << ti.bagN << endl;
		//predictions of single trees in groves on the train set data points
		doublevvv sinpreds(tigNN, doublevv(ti.maxTiGN, doublev(itemN, 0)));	
		//outer array: running column in surface matrix
		//middle array: grove (multiple trees)
		//inner array: predictions by a tree in a grove

		//predictions of groves on the train set data points 
		doublevv jointpreds(tigNN, doublev(itemN, 0));
		//outer array: running column in surface matrix
		//inner array: predictions by the grove
		//generate a grid of models
		for(int alphaNo = 0; alphaNo < alphaN; alphaNo++)
		{
			double alpha;
			if(alphaNo < alphaN - 1)
				alpha = alphaVal(alphaNo);
			else	//this is a special case because minAlpha can be zero
				alpha = ti.minAlpha;

			cout << "\tBuilding models with alpha = " << alpha << endl;

			//generate a column with the same alpha 
			for(int tigNNo = 0; tigNNo < tigNN; tigNNo++) 
			{
				data.newBag();

				int tigN = tigVal(tigNNo);	//number of trees in the current grove
				CGrove leftGrove(alpha, tigN); //(alpha, tigN) grove grown from the left neighbor
				CGrove bottomGrove(alpha, tigN); //(alpha, tigN) grove grown from the bottom neighbor
				CGrove* winGrove = &leftGrove; //better of the two groves

				//note: grove from left is automatically ready for further use,
				//	but when grove from below is needed instead, it requires extra effort 
				// (update winGrove, sinpreds, jointpreds)
				if((tigNNo == 0)  //bottom row
					|| (ti.mode == LAYERED)	//layered training style				
					|| ((ti.mode == FAST) && (bagNo > 0) && (dir[tigNNo][alphaNo] == 0))) //fixed direction
					//build from left neighbor
					leftGrove.converge(sinpreds[tigNNo], jointpreds[tigNNo]);
				else if((alphaNo == 0) || (dir[tigNNo][alphaNo] == 1))	//direction fixed upwards 
				{//build from lower neighbour 
					sinpreds[tigNNo] = sinpreds[tigNNo - 1];
					jointpreds[tigNNo] = jointpreds[tigNNo - 1];
					bottomGrove.converge(sinpreds[tigNNo], jointpreds[tigNNo]);
					winGrove = &bottomGrove;
					if((ti.mode == FAST) && (bagNo == 0))	
						dir[tigNNo][alphaNo] = 1;	//set direction upwards
					dirStat[tigNNo][alphaNo] += 1;
				}
				else
				{//build both groves, compare performances on train and oob data
					doublevv sinpreds2 = sinpreds[tigNNo - 1];
					doublev jointpreds2 = jointpreds[tigNNo - 1];

					ddpair rmse_l = leftGrove.converge(sinpreds[tigNNo], jointpreds[tigNNo]);
					ddpair rmse_b = bottomGrove.converge(sinpreds2, jointpreds2);

					if((rmse_b < rmse_l) || ((rmse_b == rmse_l) && (rand()%2 == 0)))
					{//bottom grove is the winning one
						winGrove = &bottomGrove;
						sinpreds[tigNNo] = sinpreds2;
						jointpreds[tigNNo] = jointpreds2;
						if((ti.mode == FAST) && (bagNo == 0))	
							dir[tigNNo][alphaNo] = 1;	//set direction upwards
						dirStat[tigNNo][alphaNo] += 1;
					}
				}
				//add the winning grove to a model file with alpha and tigN values in the name
				string prefix = string("./AGTemp/ag.a.") 
									+ alphaToStr(alpha)
									+ ".n." 
									+ itoa(tigN, 10);
				string tempFName = prefix + ".tmp";
				winGrove->save(tempFName.c_str());

				//generate predictions for validation set
				doublev predictions(validN);
				for(int itemNo = 0; itemNo < validN; itemNo++)
				{
					predsumsV[tigNNo][alphaNo][itemNo] += winGrove->predict(itemNo, VALID);
					predictions[itemNo] = predsumsV[tigNNo][alphaNo][itemNo] / (bagNo + 1);
				}
				if(bagNo == ti.bagN - 1)
				{
					string predsFName = prefix + ".preds.txt";
					fstream fpreds(predsFName.c_str(), ios_base::out);
					for(int itemNo = 0; itemNo < validN; itemNo++)
						fpreds << predictions[itemNo] << endl;
					fpreds.close();
				}
				rmsV[tigNNo][alphaNo][bagNo] = rmse(predictions, validTar);
				if(!ti.rms)
					rocV[tigNNo][alphaNo][bagNo] = roc(predictions, validTar);
			}//end for(int tigNNo = 0; tigNNo < tigNN; tigNNo++) 
		}//end for(int alphaNo = 0; alphaNo < alphaN; alphaNo++)
	}// end for(int bagNo = 0; bagNo < ti.bagN; bagNo++)

//4. Output
	if(ti.rms)
	{
		double validStD = data.getTarStD(VALID);
		trainOut(ti, dir, rmsV, rmsV, predsumsV, trainV, dirStat, validStD);
	}
	else
		trainOut(ti, dir, rmsV, rocV, predsumsV, trainV, dirStat);

	}catch(TE_ERROR err){
		te_errMsg((TE_ERROR)err);
		return 1;
	}catch(AG_ERROR err){
		ErrLogStream errlog;
		switch(err)
		{
			case INPUT_ERR:
				errlog << "Usage: ag_train -t _train_set_ -v _validation_set_ -r _attr_file_ "
					<< "[-a _alpha_value_] [-n _N_value_] [-b _bagging_iterations_] [-s slow|fast|layered] " 
					<< "[-i _init_random_] [-c rms|roc] | -version \n";
				break;
			case ALPHA_ERR:
				errlog << "Input error: alpha value is out of [0;1] range.\n";
				break;
			case TIGN_ERR:
				errlog << "Input error: N value is less than 1.\n"; 
				break;
			case WIN_ERR:
				errlog << "Input error: TreeExtra currently does not support multithreading for Windows.\n"; 
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
