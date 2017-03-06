//Additive Groves / ag_addbag.cpp: main function of executable ag_addbag
//
//(c) Daria Sorokina

#include "ag_functions.h"
#include "functions.h"
#include "Grove.h"
#include "LogStream.h"
#include "ErrLogStream.h"
#include "ag_definitions.h"

#ifndef _WIN32
#include "thread_pool.h"
#endif

#include <errno.h>

//ag_addbag [-m _model_file_name_] [-b _bagging_iterations_] [-i _init_random_] | -version
int main(int argc, char* argv[])
{	 
	try{
//0. Set log file
	LogStream clog;
	clog << "\n-----\nag_addbag ";
	for(int argNo = 1; argNo < argc; argNo++)
		clog << argv[argNo] << " ";
	clog << "\n\n";

	if((argc > 1) && !string(argv[1]).compare("-version"))
	{
		clog << "TreeExtra version " << VERSION << "\n";
		return 0;
	}
//1a. Set select parameters from AGTemp/params.txt

	TrainInfo ti;		//current and previous sets of input parameters
			
	fstream fparam;	
	fparam.open("./AGTemp/params.txt", ios_base::in); 
	string modeStr, metric;
	double stubD = 0; 
	int stubI = 0;
	fparam >> stubI >> ti.trainFName >> ti.validFName >> ti.attrFName >> stubD >> stubI >> stubI 
		>> modeStr >> metric;	

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

	fparam.close();

//1b. Set default values of parameters
	string modelFName = "model.bin";	//name of the input file for the model
	ti.seed = -1;	//random seed default value will be set later	
#ifndef _WIN32
	int threadN = 6;	//number of threads
#endif

//1c. Set input parameters from command line (they override default settings) 
	
	//check that the number of arguments is even (flags + value pairs)
	if(argc % 2 == 0)
		throw INPUT_ERR;
	//convert input parameters to string from char*
	stringv args(argc); 
	for(int argNo = 0; argNo < argc; argNo++)
		args[argNo] = string(argv[argNo]);
	
	//parse and save input parameters
	bool hasBagN = false; //indicator of parameter presence in the command line
	for(int argNo = 1; argNo < argc; argNo += 2)
	{
		if(!args[argNo].compare("-m"))
			modelFName = args[argNo + 1];
		else if(!args[argNo].compare("-b"))
		{
			ti.bagN = atoiExt(argv[argNo + 1]);
			hasBagN = true;
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
	}//end for(int argNo = 1; argNo < argc; argNo += 2)

	if(ti.seed == -1)
		ti.seed = ti.bagN;

//2.a) Initialize random number generator. 
	srand(ti.seed);

//2.b) Load data
	INDdata data(ti.trainFName.c_str(), ti.validFName.c_str(), ti.testFName.c_str(), 
				 ti.attrFName.c_str());
	CGrove::setData(data);
	CTreeNode::setData(data);

//2.c) Start thread pool
#ifndef _WIN32
	TThreadPool pool(threadN);
	CGrove::setPool(pool);
#endif

//3. Read model file
	
	fstream fmodel(modelFName.c_str(), ios_base::binary | ios_base::in);
	fmodel.read((char*) &ti.mode, sizeof(enum AG_TRAIN_MODE));
	
	//read fast training path
	int dirN = 0;
	boolv dir;
	if(ti.mode == FAST)
	{
		fmodel.read((char*) &dirN, sizeof(int));
		dir.resize(dirN);
		for(int dirNo = 0; dirNo < dirN; dirNo++)
		{
			bool d;	//can't read directly into dir[dirNo] because vector<bool> is not a usual stl type
			fmodel.read((char*) &d, sizeof(bool));
			dir[dirNo] = d;
		}
	}
	
	//read Groves parameters
	fmodel.read((char*) &ti.maxTiGN, sizeof(int));
	fmodel.read((char*) &ti.minAlpha, sizeof(double));
	if(fmodel.fail())
		throw MODEL_ERR;

	//Read single groves already in the model, count their number, calculate beginning of bagging curve
	doublev validTar;
	int validN = data.getTargets(validTar, VALID);
	doublev rmsV, rocV; 	//bagging curves of rms and roc (if applicable) performance for the validation set
	doublev predsumsV(validN, 0); 	//sums of predictions for each data point
	int prevBagN = 0;

	while(fmodel.peek() != char_traits<char>::eof())
	{//load next Grove in the ensemble 
		prevBagN++;
		CGrove grove(ti.minAlpha, ti.maxTiGN);
		grove.load(fmodel);

		doublev predictions(validN);
		for(int itemNo = 0; itemNo < validN; itemNo++)
		{
			predsumsV[itemNo] += grove.predict(itemNo, VALID);
			predictions[itemNo] = predsumsV[itemNo] / prevBagN;
		}
		rmsV.push_back(rmse(predictions, validTar));
		if(!ti.rms)
			rocV.push_back(roc(predictions, validTar));
	}
	fmodel.close();

	if(!hasBagN)	//set default value for the number of bagging iterations
		ti.bagN = prevBagN + 40;
	if(ti.bagN < prevBagN)
		throw BAGN_ERR;

	rmsV.resize(ti.bagN, 0);
	if(!ti.rms)
		rocV.resize(ti.bagN, 0);

	clog << "Alpha = " << ti.minAlpha << "\nN = " << ti.maxTiGN << "\n";
	if(ti.mode == FAST)
		clog << "fast mode\n\n";
	else if(ti.mode == SLOW)
		clog << "slow mode\n\n";
	else //if(ti.mode == LAYERED)
		clog << "layered mode\n\n";
	clog << "Previous model:\n\t" << prevBagN << " bagging iterations\n";
	if(ti.rms)
		clog << "\tRMSE on validation set = " << rmsV[prevBagN - 1] << "\n\n";
	else
		clog << "\tROC on validation set = " << rocV[prevBagN - 1] << "\n\n";

	//4. Train new models
	double trainV = data.getTrainV();
	int itemN = data.getTrainN();
	int alphaN = getAlphaN(ti.minAlpha, trainV);
	int tigNN = getTiGNN(ti.maxTiGN);

	for(int bagNo = prevBagN; bagNo < ti.bagN; bagNo++)
	{
		cout << "Iteration " << bagNo + 1 << " out of " << ti.bagN << endl;

		CGrove finGrove(ti.minAlpha, ti.maxTiGN);
		if(ti.mode == FAST)	//fast training, train only specified path on the grid
		{
			//predictions of single trees in a grove on the train set data points
			doublevv sinpreds(ti.maxTiGN, doublev(itemN, 0));	
			//outer array: grove (multiple trees)
			//inner array: predictions by a tree in a grove

			//predictions of a whole grove on the train set data points 
			doublev jointpreds(itemN, 0);

			int alphaNo = 0;
			int tigNNo = 0;	
			CGrove smalltree(0.5, 1);
			smalltree.converge(sinpreds, jointpreds);
			//note: sinpreds, jointpreds are both in-out arguments and are passed
			//between different groves during training
		
			for(int dirNo = 0; dirNo < dirN - 1; dirNo++) //follow all but last steps on the path
			{
				data.newBag();

				if(dir[dirNo])  //Up
					tigNNo++;
				else			//Right
					alphaNo++;
				
				double alpha;
				if(alphaNo < alphaN - 1)
					alpha = alphaVal(alphaNo);
				else //this is a special case because minAlpha can be zero
					alpha = ti.minAlpha;
				
				int tigN = tigVal(tigNNo);

				CGrove grove(alpha, tigN);
				grove.converge(sinpreds, jointpreds);
			}
			//the last step on the path, automatically place the final model into finGrove
			finGrove.converge(sinpreds, jointpreds);
		}
		else if(ti.mode == SLOW)	//slow training, train the whole grid
		{
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

				//generate a column with the same alpha 
				for(int tigNNo = 0; tigNNo < tigNN; tigNNo++) 
				{
					data.newBag();

					int tigN = tigVal(tigNNo);	//number of trees in the current grove
					CGrove leftGrove(alpha, tigN); //(alpha, tigN) grove grown from the left neighbor
					CGrove bottomGrove(alpha, tigN); //(alpha, tigN) grove grown from the bottom neighbor

					//note: grove from left is automatically ready for further use,
					//	but when grove from below is needed instead, it requires extra effort 
					// (update winGrove, sinpreds, jointpreds)
					if(tigNNo == 0)		//bottom row, build from left neighbor
						leftGrove.converge(sinpreds[tigNNo], jointpreds[tigNNo]);
					else if(alphaNo == 0)
					{//build from lower neighbour 
						sinpreds[tigNNo] = sinpreds[tigNNo - 1];
						jointpreds[tigNNo] = jointpreds[tigNNo - 1];
						bottomGrove.converge(sinpreds[tigNNo], jointpreds[tigNNo]);
					}
					else
					{//build both groves, compare performances on oob data
						doublevv sinpreds2 = sinpreds[tigNNo - 1];
						doublev jointpreds2 = jointpreds[tigNNo - 1];

						ddpair rmse_l = leftGrove.converge(sinpreds[tigNNo], jointpreds[tigNNo]);
						ddpair rmse_b = bottomGrove.converge(sinpreds2, jointpreds2);

						if((rmse_b < rmse_l) || ((rmse_b == rmse_l) && (rand()%2 == 0)))
						{//bottom grove is the winning one
							sinpreds[tigNNo] = sinpreds2;
							jointpreds[tigNNo] = jointpreds2;
						}
					}
				}//end for(int tigNNo = 0; tigNNo < tigNN; tigNNo++) 
			}//end for(int alphaNo = 0; alphaNo < alphaN; alphaNo++)
			finGrove.converge(sinpreds[tigNN - 1], jointpreds[tigNN - 1]);
		}//end else (slow training)
		else
			finGrove.trainLayered();

		//calculate next element of the bagging curve
		doublev predictions(validN);
		for(int itemNo = 0; itemNo < validN; itemNo++)
		{
			predsumsV[itemNo] += finGrove.predict(itemNo, VALID);
			predictions[itemNo] = predsumsV[itemNo] / (bagNo + 1);
		}
		rmsV[bagNo] = rmse(predictions, validTar);
		if(!ti.rms)
			rocV[bagNo] = roc(predictions, validTar);

		//append the new grove to the model file
		finGrove.save(modelFName.c_str());

	}//end for(int bagNo = prevBagN; bagNo < ti.bagN; bagNo++)

	//5. Output
	clog << "New model:\n\t" << ti.bagN << " bagging iterations\n";
	if(ti.rms)
		clog << "\tRMSE on validation set = " << rmsV[ti.bagN - 1] << "\n\n";
	else
		clog << "\tROC on validation set = " << rocV[ti.bagN - 1] << "\n\n";

	//output rms bagging curve in the best (alpha, TiGN) point
	fstream frmscurve;	//output text file 
	frmscurve.open("bagging_rms.txt", ios_base::out); 
	for(int bagNo = 0; bagNo < ti.bagN; bagNo++)
		frmscurve << rmsV[bagNo] << endl;
	frmscurve.close();

	//same for roc curve, if applicable
	if(!ti.rms)
	{
		fstream froccurve;	//output text file 
		froccurve.open("bagging_roc.txt", ios_base::out); 
		for(int bagNo = 0; bagNo < ti.bagN; bagNo++)
			froccurve << rocV[bagNo] << endl;
		froccurve.close();
	}

	//analyze whether more bagging should be recommended based on the curve	
	if(moreBag(rmsV))
	{
		int recBagN = ti.bagN + 40;
		clog << "\nRecommendation: further bagging might produce a better model.\n"
			<< "Suggested action: addbag -b " << (int)(ti.bagN + 40) << " -m " << modelFName << "\n";
	}

	}catch(TE_ERROR err){
		te_errMsg((TE_ERROR)err);
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
				errlog << "Usage: ag_addbag [-m _model_file_name_] [-b _bagging_iterations_] "
					<< "[-i _init_random_] | -version\n";
				break;
			case BAGN_ERR:
				errlog << "Input error: the number of bagging iterations is less than "
					<< "the model has already.\n";
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
}

