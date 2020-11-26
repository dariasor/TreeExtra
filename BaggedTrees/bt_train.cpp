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

#include <algorithm>
#include <errno.h>
#include <cmath>
#include <unistd.h>

#ifndef _WIN32
#include "thread_pool.h"
#endif

// XW. Programmatically decide the number of cores
#ifdef __APPLE__
#include <thread>
#endif

// XW. Used for passing arguments to a job submitted to a thread pool
struct trainArg
{
	trainArg(
			bool doOut,
			int bagNo,
			TrainInfo& ti,
			INDdata& data,
			doublev& attrCounts,
			bool doFS,
			string modelFName,
			int validN,
			doublevv& _predsumsV
			):
		doOut(doOut),
		bagNo(bagNo),
		ti(ti),
		data(data),
		attrCounts(attrCounts),
		doFS(doFS),
		modelFName(modelFName),
		validN(validN),
		_predsumsV(_predsumsV)
		{}
	bool doOut;
	int bagNo;
	TrainInfo& ti;
	INDdata& data;
	doublev& attrCounts;
	bool doFS;
	string modelFName;
	int validN;
	doublevv& _predsumsV;
};

// XW. Use mutex to access shared resources like variables within a thread
#ifndef _WIN32
TMutex StdOutMutex; // Make sure only one thread is using the standard output
TMutex ReturnMutex; // Write to the variables computed and returned by threads
#endif

// XW. Can be used in both a single-threaded setting and a multi-threaded setting
void doTrain(trainArg* ptr)
{
	bool doOut = ptr->doOut;
	int bagNo = ptr->bagNo;
	TrainInfo& ti = ptr->ti;
	INDdata& data = ptr->data;
	doublev& attrCounts = ptr->attrCounts;
	bool doFS = ptr->doFS;
	string modelFName = ptr->modelFName;
	int validN = ptr->validN;
	doublevv& _predsumsV = ptr->_predsumsV;

	doublev __predsumsV(validN, 0);

#ifndef _WIN32
StdOutMutex.Lock();
#endif
	if(doOut)
		cout << "Iteration " << bagNo + 1 << " out of " << ti.bagN << " (begin)" << endl;
#ifndef _WIN32
StdOutMutex.Unlock();
#endif

	// XW
	unsigned int state = time(NULL) + bagNo;
	INDsample sample(state, data);
	sample.newBag();

	CTree tree(ti.alpha);

	// XW
	tree.setRoot(sample);
	doublev curAttrCounts(attrCounts.size(), 0);
	tree.growBT(doFS, curAttrCounts, sample);

	string _modelFName =  getModelFName(modelFName, bagNo);
	// XW. Clear previous temp files as the save function appends to the files
	fstream fload(_modelFName.c_str(), ios_base::binary | ios_base::out);
	fload.close();
	tree.save(_modelFName.c_str());

	//generate predictions for validation set
	for(int itemNo = 0; itemNo < validN; itemNo++)
	{
		__predsumsV[itemNo] = tree.predict(itemNo, VALID);
	}

#ifndef _WIN32
ReturnMutex.Lock();
#endif
	// Unlike predsumsV, computing attrCounts is invariant to bagging order
	if(doFS)
		for(size_t attrNo = 0; attrNo < attrCounts.size(); attrNo++)
			attrCounts[attrNo] += curAttrCounts[attrNo] / sample.getBagV();

	_predsumsV[bagNo] = __predsumsV;
#ifndef _WIN32
ReturnMutex.Unlock();
#endif

#ifndef _WIN32
StdOutMutex.Lock();
#endif
	if(doOut)
		cout << "Iteration " << bagNo + 1 << " out of " << ti.bagN << " (end)" << endl;
#ifndef _WIN32
StdOutMutex.Unlock();
#endif

	return;
}

// XW. Wrap the doTrain function by TJob to be submitted to a thread pool
#ifndef _WIN32
class TrainJob: public TThreadPool::TJob
{
public:
	void Run(void* ptr)
	{
		doTrain((trainArg*) ptr);
	}
};
#endif

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
#ifndef __APPLE__
	int nCore = sysconf(_SC_NPROCESSORS_ONLN);
#else
	int nCore = std::thread::hardware_concurrency();
#endif
	// XW. Need to handle 0 which is returned when unable to detect
	if (nCore > 0) {
		if (nCore == 1)
			threadN = 1;
		else
			threadN = nCore / 2;
	}
	// std::cout << "Default number of cores is " << threadN << std::endl;
#endif

	TrainInfo ti; //model training parameters
	string modelFName = "model.bin";	//name of the output file for the model
	string predFName = "preds.txt";		//name of the output file for predictions
	int topAttrN = -1;		//how many top attributes to output and keep in the cut data 
							//(0 = do not do feature selection)
							//(-1 = output all available features)
	int splitAttrN = -1;	// XW. How many split attributes to leave in output attribute file
	bool doOut = true;		//whether to output log information to stdout

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
		else if (!args[argNo].compare("-s")) {
			splitAttrN = atoiExt(argv[argNo + 1]);
		} else {
			throw INPUT_ERR;
		}
	}//end for(int argNo = 1; argNo < argc; argNo += 2) //parse and save input parameters

	if(!(hasTrain && hasVal && hasAttr))
		throw INPUT_ERR;

	if((ti.alpha < 0) || (ti.alpha > 1))
		throw ALPHA_ERR;

//1.a) delete all temp files from the previous run and create a directory BTTemp
#ifdef WIN32	//in windows
	WIN32_FIND_DATA fn;			//structure that will contain the name of file
	HANDLE hFind;				//current file
	hFind = FindFirstFile("./BTTemp/*.*", &fn);	//"."	
	FindNextFile(hFind, &fn);						//".." 
	//delete all files in the directory
	while(FindNextFile(hFind, &fn) != 0) 
	{
		string fullName = "BTTemp/" + (string)fn.cFileName;
		DeleteFile(fullName.c_str());
	} 
	CreateDirectory("BTTemp", NULL);
#else 
	system("rm -rf ./BTTemp/");
	system("mkdir ./BTTemp/");
#endif

//1.b) Set log file
	LogStream telog;
	LogStream::init(doOut);
	telog << "\n-----\nbt_train ";
	for(int argNo = 1; argNo < argc; argNo++)
		telog << argv[argNo] << " ";
	telog << "\n\n";

//1.c) Initialize random number generator. 
	srand(ti.seed);

//2. Load data
	INDdata data(ti.trainFName.c_str(), ti.validFName.c_str(), ti.testFName.c_str(), 
				 ti.attrFName.c_str(), doOut);
	CTree::setData(data);
	CTreeNode::setData(data);

	/*// XW. Disable parallel node split cause parallel bagging consumes all CPUs
//2.a) Start thread pool
#ifndef _WIN32
	TThreadPool pool(threadN);
	CTree::setPool(pool);
#endif
	*///

//3. Train models
	doublev validTar, validWt;
	int validN = data.getTargets(validTar, validWt, VALID);

	//adjust minAlpha, if needed
	double newAlpha = adjustAlpha(ti.alpha, data.getTrainN());
	if(ti.alpha != newAlpha)
	{
		if(newAlpha == 0)
			telog << "Warning: due to small train set size value of alpha was changed to 0"; 
		else 
			telog << "Warning: alpha value was rounded to the closest valid value " << newAlpha;
		telog << ".\n\n";
		ti.alpha = newAlpha;	
	}
	telog << "Alpha = " << ti.alpha << "\n" 
		<< ti.bagN << " bagging iterations\n";

	doublev rmsV(ti.bagN, 0); 				//bagging curve of rms values for validation set
	doublev rocV;							 
	if(!ti.rms)
		rocV.resize(ti.bagN, 0);			//bagging curve of roc values for validation set
	doublev predsumsV(validN, 0); 			//sums of predictions for each data point

	int attrN = data.getAttrN();
	if(topAttrN == -1)
		topAttrN = attrN;
	// XW. Default number of split attributes to leave is 3
	if (splitAttrN == -1)
		splitAttrN = 3;
	
	doublev attrCounts(attrN, 0); //counts of attribute importance
	idpairv attrCountsP(attrN, idpair(0, 0)); //another structure for counts of attribute importance, will need it later for sorting
	bool doFS = (topAttrN != 0);	//whether feature selection is requested
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

// XW. 3.a) Make bags, build trees, collect predictions in parallel if not Windows
	doublevv _predsumsV(ti.bagN, doublev(validN, 0));
#ifdef _WIN32
	for (int bagNo = 0; bagNo < ti.bagN; bagNo ++)
	{
		doTrain(new trainArg(
				doOut,
				bagNo,
				ti,
				data,
				attrCounts,
				doFS,
				modelFName,
				validN,
				_predsumsV
				));
	}
#else
	TThreadPool pool(threadN);
	for (int bagNo = 0; bagNo < ti.bagN; bagNo ++)
	{
		trainArg* ptr = new trainArg(
				doOut,
				bagNo,
				ti,
				data,
				attrCounts,
				doFS,
				modelFName,
				validN,
				_predsumsV
				);
		pool.Run(new TrainJob, ptr);
	}
	pool.SyncAll();
#endif
	telog << "Info: All of the threads have been synchronized\n"; // XW

// XW. 3.b) Aggregate results
	doublev predictions(validN);
	for(int bagNo = 0; bagNo < ti.bagN; bagNo++)
	{
		CTree tree(ti.alpha);
		string _modelFName = getModelFName(modelFName, bagNo);
		fstream fload(_modelFName.c_str(), ios_base::binary | ios_base::in);
		tree.load(fload);
		tree.save(modelFName.c_str());

		//generate predictions for validation set
		for(int itemNo = 0; itemNo < validN; itemNo++)
		{
			predsumsV[itemNo] += _predsumsV[bagNo][itemNo];
			predictions[itemNo] = predsumsV[itemNo] / (bagNo + 1);
		}
		rmsV[bagNo] = rmse(predictions, validTar, validWt);
		if(!ti.rms)
			rocV[bagNo] = roc(predictions, validTar, validWt);

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

//4. Output
		
	//output results 
	if(ti.rms)
		telog << "RMSE on validation set = " << rmsV[ti.bagN - 1] << "\n";
	else
		telog << "ROC on validation set = " << rocV[ti.bagN - 1] << "\n";

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
		telog << "\nRecommendation: a greater number of bagging iterations might produce a better model.\n"
			<< "Suggested action: bt_train -b " << recBagN << "\n";
	}
	else
		telog << "\nThe bagging curve shows good convergence. \n"; 
	telog << "\n";

	//standard output in case of turned off log output: final performance on validation set only
	if(!doOut)
		if(ti.rms)
			cout << rmsV[ti.bagN - 1] << endl;
		else
			cout << rocV[ti.bagN - 1] << endl;

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
		// ffeatures << "Top " << topAttrN << " features\n";
		ffeatures << "All " << attrN << " features in descending order of scores\n"; // XW
		for (int attrNo = 0; attrNo < attrN; attrNo ++) // XW
			ffeatures << data.getAttrName(attrCountsP[attrNo].first) << "\t" 
				<< attrCountsP[attrNo].second / ti.bagN << "\n";
		ffeatures << "\n\nColumn numbers (beginning with 1)\n";
		for (int attrNo = 0; attrNo < attrN; attrNo ++) // XW
			ffeatures << data.getColNo(attrCountsP[attrNo].first) + 1 << " ";
		ffeatures << "\nLabel column number: " << data.getTarColNo() + 1;
		ffeatures.close();

		// XW
		intset splitAttrs = data.getSplitAttrs();
		telog << "Aim to leave " << splitAttrN << " out of " << splitAttrs.size() << " attributes marked by split\n";
		int splitN = 0;
		for (int attrNo = 0; attrNo < topAttrN; attrNo ++) {
			if (splitAttrs.find(attrCountsP[attrNo].first) != splitAttrs.end()) {
				splitN ++;
				telog << "\t" << data.getAttrName(attrCountsP[attrNo].first) << " (split)\n";
			}
		}
		telog << "Find " << splitN << " attributes marked by split and within top " << topAttrN << "\n";
		splitN = splitAttrN - splitN;
		telog << "Keep " << splitN << " attributes marked by split but not within top " << topAttrN << "\n";

		//output new attribute file
		for (int attrNo = topAttrN; attrNo < attrN; attrNo ++) {
			// XW. Do not ignore if an attribute is marked by split and there are less than TK split attributes
			if ((splitAttrs.find(attrCountsP[attrNo].first) != splitAttrs.end()) && (splitN > 0)) {
				splitN --;
				telog << "\t" << data.getAttrName(attrCountsP[attrNo].first) << " (split)\n";
			} else {
				data.ignoreAttr(attrCountsP[attrNo].first);
			}
		}
		telog << "\n";
		data.outAttr(ti.attrFName);
	}

	if(data.getHasActiveMV())
		telog << "Warning: the data has missing values in active attributes, correlations can not be calculated.\n\n";
	else
	{
		// XW
		int bagNo = ti.bagN - 1;
		unsigned int state = bagNo; // TODO. time(NULL) + bagNo;
		INDsample sample(state, data);
		sample.newBag();
		sample.correlations(ti.trainFName);
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
					<< "[-l log|nolog] [-s _split_attributes_to_leave_] | -version\n"; // XW
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
