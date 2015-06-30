//Additive Groves / ag_savemerge.cpp: main function of executable ag_savemerge
//
//(c) Daria Sorokina

#include "ag_functions.h"
#include "functions.h"
#include "Grove.h"
#include "LogStream.h"
#include "ErrLogStream.h"
#include "ag_definitions.h"

#include <errno.h>
#include <sys/types.h>  
#include <sys/stat.h>   

//ag_savemerge  -a _alpha_value -n _N_value_ [-m _model_file_name_]
//-d _directory1_ _directory2_ [_directory3_] [_directory4_] ...
int main(int argc, char* argv[])
{	 
	try{
	//0. Set log file
	LogStream clog;
	clog << "\n-----\nag_savemerge ";
	for(int argNo = 1; argNo < argc; argNo++)
		clog << argv[argNo] << " ";
	clog << "\n\n";
	
	//1. Read values of parameters from files
	string modelFName = "model.bin";	//name of the output file for the model
	TrainInfo ti;		 
	//parameters of the model to be saved
	int saveTiGN, itemN, iStub;		
	double saveAlpha, dStub;	
	string sStub;
	

	//2. Set parameters from command line
	
	int firstDirNo = 0;
	//convert input parameters to string from char*
	stringv args(argc); 
	for(int argNo = 0; argNo < argc; argNo++)
		args[argNo] = string(argv[argNo]);
	
	//parse and save input parameters
	//indicators of presence of required flags in the input
	bool hasN = false; 
	bool hasA = false; 
	for(int argNo = 1; argNo < argc; argNo += 2)
	{
		if(!args[argNo].compare("-a"))
		{
			saveAlpha = atofExt(argv[argNo + 1]);
			hasA = true;
		}
		else if(!args[argNo].compare("-n"))
		{
			saveTiGN = atoiExt(argv[argNo + 1]);
			hasN = true;
		}
		else if(!args[argNo].compare("-m"))
		{
			modelFName = args[argNo + 1];
			if(modelFName.empty())
				throw EMPTY_MODEL_NAME_ERR;
		}
		else if(!args[argNo].compare("-d"))
		{
			firstDirNo = argNo + 1;
			break;
		}
		else
			throw INPUT_ERR;
	}//end for(int argNo = 1; argNo < argc; argNo += 2)

	if(!(hasA && hasN))
		throw INPUT_ERR;
	//check that there are at least two directories 
	if(argc < (firstDirNo + 2))
		throw INPUT_ERR;

	int folderN = argc - firstDirNo;
	stringv folders(folderN); 
	for(int argNo = firstDirNo; argNo < argc; argNo++)
	{
		folders[argNo - firstDirNo] = string(argv[argNo]);
		struct stat status;
		if((stat(argv[argNo], &status) != 0) || !(status.st_mode & S_IFDIR))
			throw DIR_ERR;
	}

	//read values of Groves parameters that produced best results
	fstream fbest;
	string bestPathName = folders[0] + "/AGTemp/best.txt";
	fbest.open(bestPathName.c_str(), ios_base::in); 
	fbest >> dStub >> iStub >> dStub >> iStub >> itemN;
	if(fbest.fail())
		throw TEMP_ERR;
	fbest.close();

	//read values of parameters for which models are trained
	fstream fparam;
	string paramPathName = folders[0] + "/AGTemp/params.txt";
	fparam.open(paramPathName.c_str(), ios_base::in); 
	string modeStr, attrFName;
	fparam >> iStub >> sStub >> sStub >> ti.attrFName >> ti.minAlpha >> ti.maxTiGN 
		>> iStub >> modeStr;	
	//modeStr should be "fast" or "slow" or "layered"	
	if(modeStr.compare("fast") == 0)
		ti.mode = FAST;
	else if(modeStr.compare("slow") == 0)
		ti.mode = SLOW;
	else if(modeStr.compare("layered") == 0)
		ti.mode = LAYERED;
	else
		throw TEMP_ERR;
	if(fparam.fail())
		throw TEMP_ERR;
	fparam.close();

	if((saveAlpha < ti.minAlpha) || (saveAlpha > 1))
		throw ALPHA_ERR;
	if(saveTiGN > ti.maxTiGN)
		throw TIGN_ERR;


	//read number of bags in each folder
	intv bagNs(folderN, 0);
	for(int folderNo = 0; folderNo < folderN; folderNo++)
	{
		TrainInfo extraTI;	//set of model parameters in the additional directory
		
		string fparamPathName = folders[folderNo] + "/AGTemp/params.txt";
		fparam.open(fparamPathName.c_str(), ios_base::in); 

		fparam >> iStub >> sStub >> sStub >> sStub >> dStub >> iStub >> bagNs[folderNo];

		if(fparam.fail())
		{
			clog << fparamPathName << '\n';
			throw TEMP_ERR;
		}
		fparam.close();
	}

	//adjust alpha, if needed
	double newAlpha = adjustAlpha(saveAlpha, itemN);
	if(saveAlpha != newAlpha)
	{
		clog << "Warning: alpha value was rounded to the closest valid value " << newAlpha << ".\n\n";
		saveAlpha = newAlpha;	
	}
	//adjust saveTiGN, if needed
	double newTiGN = adjustTiGN(saveTiGN);
	if(saveTiGN != newTiGN)
	{
		clog << "Warning: N value was rounded to the closest smaller valid value " << newTiGN << ".\n\n";
		saveTiGN = newTiGN;	
	}

	int alphaN = getAlphaN(ti.minAlpha, itemN);
	int tigNN = getTiGNN(ti.maxTiGN);
	int saveAlphaNo = getAlphaN(saveAlpha, itemN) - 1;
	int saveTiGNNo = getTiGNN(saveTiGN) - 1;
	boolv dir; //path on the parameter grid

	clog << "Alpha = " << saveAlpha << "\nN = " << saveTiGN /*<< "\n" 
		<< saveBagN << " bagging iterations"*/ << "\n\n";

	//3. Load info about attributes
	INDdata data("", "", "", ti.attrFName.c_str());
	CGrove::setData(data);
	CTreeNode::setData(data);

	//4. For fast models, figure out the directions path.
	if(ti.mode == FAST)
	{//read the directions table from file
		doublevv dirMx(tigNN, doublev(alphaN, 0)); 
		//outer array: column (by TiGN)
		//middle array: row	(by alpha)
		fstream fdir;	
		fdir.open("./AGTemp/dir.txt", ios_base::in); 
		for(int tigNNo = 0; tigNNo < tigNN; tigNNo++)
			for(int alphaNo = 0; alphaNo < alphaN; alphaNo++)
				fdir >> dirMx[tigNNo][alphaNo];
		if(fdir.fail())
			throw TEMP_ERR;
		fdir.close();

		//set dir - path from (0,0) to (bestAlphaNo, bestTigNNo)
		int tigNNo = saveTiGNNo; 
		int alphaNo = saveAlphaNo;
		for(int dirNo = 0; dirNo < saveTiGNNo + saveAlphaNo; dirNo++)
			if(dirMx[tigNNo][alphaNo] == 1) //UP
			{
				dir.insert(dir.begin(),true);
				tigNNo--;
			}
			else
			{
				dir.insert(dir.begin(),false);
				alphaNo--;
			}
	}

	//5. Save the model
	fstream fmodel(modelFName.c_str(), ios_base::binary | ios_base::out);
	//save ti.mode, dir (if ti.mode==FAST) and saveTiGN
	fmodel.write((char*) &ti.mode, sizeof(enum AG_TRAIN_MODE));
	if(ti.mode == FAST)
	{
		int dirN = (int)dir.size();
		fmodel.write((char*) &dirN, sizeof(int));
		for(int dirNo = 0; dirNo < saveTiGNNo + saveAlphaNo; dirNo++)
		{
			bool d = dir[dirNo]; 
			fmodel.write((char*) &d, sizeof(bool));
		}	//can't write directly from dir[dirNo] because vector<bool> is not a usual stl type
	}
	fmodel.write((char*) &saveTiGN, sizeof(int));
	fmodel.write((char*) &saveAlpha, sizeof(double));
	fmodel.close();
	
	//generate the title of the file with the trees
	const int buflen = 1024;
	char buf[buflen]; 
	string treesFName = string("/AGTemp/ag.a.") 
						+ alphaToStr(saveAlpha)
						+ ".n." 
						+ itoa(saveTiGN, 10) 
						+ ".tmp";

	//read groves from all folders and save them to the output file
	for(int folderNo = 0; folderNo < folderN; folderNo++)
	{
		string fullTreesFName = folders[folderNo] + treesFName;
		fstream ftrees(fullTreesFName.c_str(), ios_base::binary | ios_base::in);
		for(int groveNo = 0; groveNo < bagNs[folderNo]; groveNo++)
		{
			CGrove grove(saveAlpha, saveTiGN);
			try{
			grove.load(ftrees);
			}catch(TE_ERROR err){
				clog << fullTreesFName << '\n';
				throw err;
			}
			grove.save(modelFName.c_str()); 
		}
		ftrees.close();
	}
	}catch(TE_ERROR err){
		te_errMsg((TE_ERROR)err);
		return 1;
	}catch(AG_ERROR err){
		ErrLogStream errlog;
		switch(err)
		{
			case INPUT_ERR:
				errlog << "Usage: ag_savemerge -a _alpha_value -n _N_value_ [-m _model_file_name_] " 
					<< "-d _directory1_ _directory2_ [_directory3_] [_directory4_] ... \n";
				break;
			case TEMP_ERR:
				errlog << "Error: temporary files from previous runs of train/expand "
					<< "are missing or corrupted.\n";
				break;
			case ALPHA_ERR:
				errlog << "Error: alpha value is out of [0;1] range " 
					<< "or less than in the last run of train/expand.\n";
				break;
			case TIGN_ERR:
				errlog << "Input error: N value is greater than in the last run of train/expand.\n";
				break;
			case BAGN_ERR:
				errlog << "Input error: number of bagging iterations is greater than "
					<< "in the last run of train/expand.\n";
				break;
			case DIR_ERR:
				errlog << "Error: one of input directories does not exist.\n";
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
