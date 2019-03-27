//Additive Groves / ag_save.cpp: main function of executable ag_save
//
//(c) Daria Sorokina

#include "ag_functions.h"
#include "functions.h"
#include "Grove.h"
#include "LogStream.h"
#include "ErrLogStream.h"
#include "ag_definitions.h"

#include <errno.h>

//ag_save [-m _model_file_name_] [-a _alpha_value] [-n _N_value_] [-b _bagging_iterations_] | -version
int main(int argc, char* argv[])
{	 
	try{
	//0. Set log file
	LogStream telog;
	telog << "\n-----\nag_save ";
	for(int argNo = 1; argNo < argc; argNo++)
		telog << argv[argNo] << " ";
	telog << "\n\n";
	
	if((argc > 1) && !string(argv[1]).compare("-version"))
	{
		telog << "TreeExtra version " << VERSION << "\n";
		return 0;
	}
	
	//1. Read values of parameters from files
	string modelFName = "model.bin";	//name of the output file for the model
	TrainInfo ti;		 
	//parameters of the model to be saved
	int saveTiGN, saveBagN, iStub;		
	double saveAlpha, trainN, dStub;	
	string sStub;
	
	//read values of Groves parameters that produced best results
	fstream fbest;
	fbest.open("./AGTemp/best.txt", ios_base::in); 
	fbest >> dStub >> saveTiGN >> saveAlpha >> saveBagN >> trainN;
	if(fbest.fail())
		throw TEMP_ERR;
	fbest.close();

	//read values of parameters for which models are trained
	fstream fparam;	
	fparam.open("./AGTemp/params.txt", ios_base::in); 
	string modeStr, attrFName;
	fparam >> iStub >> sStub >> sStub >> ti.attrFName >> ti.minAlpha >> ti.maxTiGN 
		>> ti.bagN >> modeStr;	
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

	//2. Set parameters from command line

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
			saveAlpha = atofExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-n"))
			saveTiGN = atoiExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-b"))
			saveBagN = atoiExt(argv[argNo + 1]);
		else if(!args[argNo].compare("-m"))
		{
			modelFName = args[argNo + 1];
			if(modelFName.empty())
				throw EMPTY_MODEL_NAME_ERR;
		}
		else
			throw INPUT_ERR;
	}//end for(int argNo = 1; argNo < argc; argNo += 2)

	if((saveAlpha < ti.minAlpha) || (saveAlpha > 1))
		throw ALPHA_ERR;
	if(saveTiGN > ti.maxTiGN)
		throw TIGN_ERR;
	if(saveBagN > ti.bagN)
		throw BAGN_ERR;

	//adjust alpha, if needed
	double newAlpha = adjustAlpha(saveAlpha, trainN);
	if(saveAlpha != newAlpha)
	{
		telog << "Warning: alpha value was rounded to the closest valid value " << newAlpha << ".\n\n";
		saveAlpha = newAlpha;	
	}
	//adjust saveTiGN, if needed
	int newTiGN = adjustTiGN(saveTiGN);
	if(saveTiGN != newTiGN)
	{
		telog << "Warning: N value was rounded to the closest smaller valid value " << newTiGN << ".\n\n";
		saveTiGN = newTiGN;	
	}

	int alphaN = getAlphaN(ti.minAlpha, trainN);
	int tigNN = getTiGNN(ti.maxTiGN);
	int saveAlphaNo = getAlphaN(saveAlpha, trainN) - 1;
	int saveTiGNNo = getTiGNN(saveTiGN) - 1;
	boolv dir; //path on the parameter grid

	telog << "Alpha = " << saveAlpha << "\nN = " << saveTiGN << "\n" 
		<< saveBagN << " bagging iterations" << "\n\n";

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
	string treesFName = string("./AGTemp/ag.a.") 
						+ alphaToStr(saveAlpha)
						+ ".n." 
						+ itoa(saveTiGN, 10) 
						+ ".tmp";
	//read saveBagN groves and save them to the output file
	fstream ftrees(treesFName.c_str(), ios_base::binary | ios_base::in);
	for(int groveNo = 0; groveNo < saveBagN; groveNo++)
	{
		CGrove grove(saveAlpha, saveTiGN);
		grove.load(ftrees);
		grove.save(modelFName.c_str()); 
	}
	ftrees.close();

	}catch(TE_ERROR err){
		te_errMsg((TE_ERROR)err);
		return 1;
	}catch(AG_ERROR err){
		ErrLogStream errlog;
		switch(err)
		{
			case INPUT_ERR:
				errlog << "Usage: ag_save [-m _output_file_name_] [-a _alpha_value_] [-n _N_value_] " 
					<< "[-b _bagging_iterations_] | -version\n";
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
