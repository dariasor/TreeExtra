//Additive Groves / ag_expand.cpp: main function of executable ag_expand
//
//(c) Daria Sorokina

#include "Grove.h"
#include "ag_functions.h"
#include "functions.h"
#include "LogStream.h"
#include "ErrLogStream.h"
#include "ag_definitions.h"

#ifndef _WIN32
#include "thread_pool.h"
#endif

#include <errno.h>
#include <algorithm>

//ag_expand [-a _alpha_value_] [-n _N_value_] [-b _bagging_iterations_] [-i _init_random_] [-e on/off] | -version

int main(int argc, char* argv[])
{	
	try{
//0. Set log file
	LogStream clog;
	clog << "\n-----\nag_expand ";
	for(int argNo = 1; argNo < argc; argNo++)
		clog << argv[argNo] << " ";
	clog << "\n\n";

	if((argc > 1) && !string(argv[1]).compare("-version"))
	{
		clog << "TreeExtra version " << VERSION << "\n";
		return 0;
	}

//1. Set parameters from AGTemp/params.txt

#ifndef _WIN32
	int threadN = 6;	//number of threads
#endif

	TrainInfo ti, prev;		//current and previous sets of input parameters
	double prevBest;		//best value of performance achieved on the previous run
			
	fstream fparam;	
	fparam.open("./AGTemp/params.txt", ios_base::in); 
	string modeStr, metric;
	fparam >> ti.seed >> ti.trainFName >> ti.validFName >> ti.attrFName >> ti.minAlpha >> ti.maxTiGN 
		>> ti.bagN >> modeStr >> metric;	

	//modeStr should be "fast" or "slow" or "layered"	
	if(modeStr.compare("fast") == 0)
		ti.mode = FAST;
	else if(modeStr.compare("slow") == 0)
		ti.mode = SLOW;
	else if(modeStr.compare("layered") == 0)
		ti.mode = LAYERED;
	else
		throw TEMP_ERR;

	//metric should be "roc" or "rms"
	if(metric.compare("rms") == 0)
		ti.rms = true;
	else if(metric.compare("roc") == 0)
		ti.rms = false;
	else
		throw TEMP_ERR;

	if(fparam.fail())
		throw TEMP_ERR;
	fparam.close();

	//read best value of performance on previous run
	fstream fbest;
	fbest.open("./AGTemp/best.txt", ios_base::in); 
	fbest >> prevBest;
	if(fbest.fail())
		throw TEMP_ERR;
	fbest.close();

	ti.seed += 10000;	//random seed default value is previous value + 10 000 (hope nobody will merge from more than 10000 cpus...)
	prev.minAlpha = ti.minAlpha;
	prev.maxTiGN = ti.maxTiGN;
	prev.bagN = ti.bagN;

//2. Set input parameters from command line (they override settings from AGTemp/params.txt)

	bool errorsOn = true; //hidden input parameter for expand, allows to turn off some safety

	//check that the number of arguments is even (flags + value pairs)
	if(argc % 2 == 0)
		throw INPUT_ERR;
	//convert input parameters to string from char*
	stringv args(argc); 
	for(int argNo = 0; argNo < argc; argNo++)
		args[argNo] = string(argv[argNo]);
	
	//parse and save input parameters
	for(int argNo = 1; argNo < argc; argNo += 2)
	{
		if(!args[argNo].compare("-a"))
			ti.minAlpha = atofExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-n"))
			ti.maxTiGN = atoiExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-b"))
			ti.bagN = atoiExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-i"))
			ti.seed = atoiExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-h"))
#ifndef _WIN32 
			threadN = atoiExt(argv[argNo + 1]);
#else
			throw WIN_ERR;
#endif
		else if(!args[argNo].compare("-e"))
		{
			if(!args[argNo + 1].compare("on"))
				errorsOn = true;
			else if(!args[argNo + 1].compare("off"))
				errorsOn = false;
			else
				throw INPUT_ERR;
		}
		else
			throw INPUT_ERR;
	}//end for(int argNo = 1; argNo < argc; argNo += 2)

	//check a,n,b values
	if(ti.minAlpha < 0)
		throw ALPHA_ERR;
	if(errorsOn)
	{
		if(ti.minAlpha > prev.minAlpha)
			throw ALPHA_ERR;
		if(ti.maxTiGN < prev.maxTiGN)
			throw TIGN_ERR;
	}
	if(ti.bagN < prev.bagN)
		throw BAGN_ERR;

//2.a) Initialize random number generator. 
	srand(ti.seed);

//3. Load data
	INDdata data(ti.trainFName.c_str(), ti.validFName.c_str(), ti.testFName.c_str(), 
				 ti.attrFName.c_str());
	CGrove::setData(data);
	CTreeNode::setData(data);

//3.a) Start thread pool
#ifndef _WIN32
	TThreadPool pool(threadN);
	CGrove::setPool(pool);
#endif

//4. Initialize other variables and produce initial output
	doublev validTar;
	int validN = data.getTargets(validTar, VALID);
	double trainV = data.getTrainV();
	int itemN = data.getTrainN();

	//adjust minAlpha, if needed
	double newAlpha = adjustAlpha(ti.minAlpha, trainV);
	if(ti.minAlpha != newAlpha)
	{
		if(newAlpha == 0)
			clog << "Warning: due to small train set size value of alpha was changed to 0"; 
		else 
			clog << "Warning: alpha value was rounded to the closest valid value " << newAlpha;
		clog << ".\n\n";
		ti.minAlpha = newAlpha;	
	}
	//adjust maxTiGN, if needed
	int newTiGN = adjustTiGN(ti.maxTiGN);
	if(ti.maxTiGN != newTiGN)
	{
		clog << "Warning: N value was rounded to the closest smaller valid value - " << newTiGN << ".\n\n";
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
	clog << "Model already trained:\n\tAlpha = " << prev.minAlpha << "\n\tN = " << prev.maxTiGN 
		<< "\n\t" << prev.bagN << " bagging iterations\n"; 
	if(ti.rms)
		clog << "Previous model's RMSE on validation set = " << prevBest << "\n\n";
	else
		clog << "Previous model's ROC on validation set = " << prevBest << "\n\n";

	int alphaN = getAlphaN(ti.minAlpha, trainV); //number of different alpha values
	int prevAlphaN = getAlphaN(prev.minAlpha, trainV); //number of alpha values already trained
	int tigNN = getTiGNN(ti.maxTiGN);	//number of different tigN values
	int prevTiGNN = getTiGNN(prev.maxTiGN);		//number of different tigN values already trained	

	//surfaces of performance values for validation set. 
	//Always calculate rms (for convergence analysis), if needed, calculate roc
	doublevvv rmsV(max(tigNN, prevTiGNN), doublevv(max(alphaN, prevAlphaN), doublev(ti.bagN, 0))); 
	doublevvv rocV;
	if(!ti.rms)
		rocV.resize(max(tigNN, prevTiGNN), doublevv(max(alphaN, prevAlphaN), doublev(ti.bagN, 0))); 
	//outer array: column (by TiGN)
	//middle array: row (by alpha)
	//inner array: bagging iterations. Performance is kept for all iterations to create bagging curves

	//sums of predictions for each data point (raw material to calculate performance)
	doublevvv predsumsV(max(tigNN, prevTiGNN), doublevv(max(alphaN, prevAlphaN), doublev(validN, 0)));
	//outer array: column (by TiGN)
	//middle array: row	(by alpha)
	//inner array: data points in the validation set
	
	//load part of predsumsV that is already filled
	fstream fsums;
	fsums.open("./AGTemp/predsums.bin", ios_base::binary | ios_base::in);
	for(int tigNNo = 0; tigNNo < prevTiGNN; tigNNo++)
		for(int alphaNo = 0; alphaNo < prevAlphaN; alphaNo++)
			fsums >> predsumsV[tigNNo][alphaNo];
	if(fsums.fail())
		throw TEMP_ERR;
	fsums.close();

	//load rms matrices for every bagging iteration for models already trained
	fstream fbagrms;	
	fbagrms.open("./AGTemp/bagrms.txt", ios_base::in); 
	for(int bagNo = 0; bagNo < prev.bagN; bagNo++)
		for(int tigNNo = 0; tigNNo < prevTiGNN; tigNNo++)
			for(int alphaNo = 0; alphaNo < prevAlphaN; alphaNo++)
				fbagrms >> rmsV[tigNNo][alphaNo][bagNo];
	if(fbagrms.fail())
		throw TEMP_ERR;
	fbagrms.close();

	//same for roc, if needed
	if(!ti.rms)
	{
		fstream fbagroc;	
		fbagroc.open("./AGTemp/bagroc.txt", ios_base::in); 
		for(int bagNo = 0; bagNo < prev.bagN; bagNo++)
			for(int tigNNo = 0; tigNNo < prevTiGNN; tigNNo++)
				for(int alphaNo = 0; alphaNo < prevAlphaN; alphaNo++)
					fbagroc >> rocV[tigNNo][alphaNo][bagNo];
		if(fbagroc.fail())
			throw TEMP_ERR;
		fbagroc.close();
	}

	//direction of initialization (1 - up, 0 - right), used in fast mode only
	doublevv dir(max(tigNN, prevTiGNN), doublev(max(alphaN, prevAlphaN), 0)); 
	//outer array: column (by TiGN)
	//middle array: row	(by alpha)
	if(ti.mode == FAST)
	{//read part of the directions table from file
		fstream fdir;	
		fdir.open("./AGTemp/dir.txt", ios_base::in); 
		for(int tigNNo = 0; tigNNo < prevTiGNN; tigNNo++)
			for(int alphaNo = 0; alphaNo < prevAlphaN; alphaNo++)
				fdir >> dir[tigNNo][alphaNo];
		if(fdir.fail())
			throw TEMP_ERR;
		fdir.close();
	}

	//direction of initialization (1 - up, 0 - right), collects statistics in the slow mode
	doublevv dirStat(max(tigNN, prevTiGNN), doublev(max(alphaN, prevAlphaN), 0));
	fstream fdirStat;	
	fdirStat.open("./AGTemp/dirstat.txt", ios_base::in); 
	for(int alphaNo = 0; alphaNo < prevAlphaN; alphaNo++)
		for(int tigNNo = 0; tigNNo < prevTiGNN; tigNNo++)
		{
			fdirStat >> dirStat[tigNNo][alphaNo];
			dirStat[tigNNo][alphaNo] *= prev.bagN;
		}
	if(fdirStat.fail())
		throw TEMP_ERR;
	fdirStat.close();


	//temp files containing earlier groves on the edge of grid
	vector<fstream*> ftemps(prevTiGNN + prevAlphaN - 1);

//5. Train and save models
	int bagNo = 0; //number of the first bagging iteration where model needs to be expanded
	if((ti.maxTiGN == prev.maxTiGN) && (ti.minAlpha == prev.minAlpha))
		bagNo = prev.bagN;
	for(; bagNo < ti.bagN; bagNo++)
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

				//temp file that keeps models corresponding to alpha and tigN
				string prefix = string("./AGTemp/ag.a.") 
									+ alphaToStr(alpha)
									+ ".n." 
									+ itoa(tigN, 10);
				string tempFName = prefix + ".tmp";

				if(bagNo < prev.bagN)
				{
					//on old edge, retrieve old models and regenerate sinpreds and jointpreds
					if((tigNNo == prevTiGNN - 1) && (alphaNo < prevAlphaN) ||
					   (alphaNo == prevAlphaN - 1) && (tigNNo < prevTiGNN))
					{
						fstream*& pftemp = ftemps[tigNNo + prevAlphaN - 1 - alphaNo];
						if(bagNo == 0)
							pftemp = new fstream(tempFName.c_str(), ios_base::binary | ios_base::in);
						if(pftemp->fail())
							throw TEMP_ERR;		
						
						CGrove oldGrove(alpha, tigN);
						oldGrove.load(*pftemp);

						oldGrove.batchPredict(sinpreds[tigNNo], jointpreds[tigNNo]);

						if(bagNo == prev.bagN - 1)
						{
							pftemp->close();
							delete pftemp;
						}
					}
					//skip the rest of the iteration when the model is already built
					if((tigNNo < prevTiGNN) && (alphaNo < prevAlphaN))
						continue;
				}
				
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
				{//build both groves, compare performances on oob data
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
	}// end for(; bagNo < ti.bagN; bagNo++)
	
//4. Output
	if(ti.rms)
	{
		double validStD = data.getTarStD(VALID);
		trainOut(ti, dir, rmsV, rmsV, predsumsV, trainV, dirStat, validStD);
	}
	else
		trainOut(ti, dir, rmsV, rocV, predsumsV, trainV, dirStat);

	}catch(TE_ERROR err){
		ErrLogStream errlog;
		switch(err)
		{
			case TREE_LOAD_ERR:
				errlog << "Error: temporary files from previous runs of train/expand "
					<< "are corrupted.\n";
				break;
			case MODEL_ATTR_MISMATCH_ERR:
				errlog << "Error: either temporary files from previous runs of train/expand "
					<< "or attribute file are corrupted.\n";
				break;
			default:
				te_errMsg((TE_ERROR)err);
		}
		return 1;

	}catch(AG_ERROR err){
		ErrLogStream errlog;
		switch(err)
		{
			case TEMP_ERR:
				errlog << "Error: temporary files from previous runs of train/expand "
					<< "are missing or corrupted.\n";
				break;
			case INPUT_ERR:
				errlog << "Usage: ag_expand [-a _alpha_value_] [-n _N_value_] [-b _bagging_iterations_]"
					<< " [-i _init_random_] | -version\n";
				break;
			case ALPHA_ERR:
				errlog << "Input error: alpha value is out of [0; previous value] range.\n";
				break;
			case TIGN_ERR:
				errlog << "Input error: N value is less than in previous runs of train/expand.\n";
				break;
			case BAGN_ERR:
				errlog << "Input error: number of bagging iterations is less than "
					<< "in previous runs of train/expand.\n";
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
