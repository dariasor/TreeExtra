//Visualization / vis_effect.cpp: main function of executable vis_effect
//
//(c) Daria Sorokina

#include "Grove.h"
#include "TrainInfo.h"
#include "LogStream.h"
#include "ErrLogStream.h"
#include "functions.h"
#include "ag_definitions.h"
#include "ag_functions.h"

#include <errno.h>

//vis_effect -v _validation_set_ -r _attr_file_ -f _feature_ [-m _model_file_name_] [-o _output_file_name_] 
	//[-q _#quantile_values_] | -version
int main(int argc, char* argv[])
{
	try{
	//0. Set log file
	LogStream clog;
	clog << "\n-----\nvis_effect ";
	for(int argNo = 1; argNo < argc; argNo++)
		clog << argv[argNo] << " ";
	clog << "\n\n";
	
	if((argc > 1) && !string(argv[1]).compare("-version"))
	{
		clog << "TreeExtra version " << VERSION << "\n";
		return 0;
	}

	//1. Set default values of parameters
	string modelFName = "model.bin";	//name of the input file for the model
	string outFName;					//name of the output file for predictions
	int quantN = 10;					//number of quantile point values to plot

	TrainInfo ti;
	string attrName; // partial dependence attribute name 

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
	bool hasVal = false; 
	bool hasAttr = false;
	bool hasFeature = false;
	
	for(int argNo = 1; argNo < argc; argNo += 2)
	{
		if(!args[argNo].compare("-m"))
			modelFName = args[argNo + 1];
		else if(!args[argNo].compare("-o"))
			outFName = args[argNo + 1];
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
		else if(!args[argNo].compare("-f"))
		{
			attrName = args[argNo + 1];
			hasFeature = true;
		}
		else if(!args[argNo].compare("-q"))
			quantN = atoi(argv[argNo + 1]);
		else
			throw INPUT_ERR;
	}

	if(!(hasVal && hasAttr && hasFeature))
		throw INPUT_ERR;

//2. Load data
	INDdata data(ti.trainFName.c_str(), ti.validFName.c_str(), ti.testFName.c_str(), 
				 ti.attrFName.c_str());
	CGrove::setData(data);
	CTreeNode::setData(data);
	
	int attrId = data.getAttrId(attrName);
	if(!data.isActive(attrId))
		throw ATTR_NAME_ERR;

//3. Calculate and output data for feature effect plot
	outEffects(data, intv(1,attrId), quantN, modelFName, outFName);		

	if(outFName.size() == 0)
		outFName = attrName + ".effect.txt";
	clog << "Partial dependence function values are saved into file " << outFName << ".\n";

	}catch(TE_ERROR err){
		te_errMsg((TE_ERROR)err);
		return 1;
	}catch(AG_ERROR err){
		ErrLogStream errlog;
		switch(err) 
		{
			case INPUT_ERR:
				errlog << "Usage: -v _validation_set_ -r _attr_file_ -f _feature_ [-m _model_file_name_] "
					<< "[-o _output_file_name_] [-q _#quantile_values_] | -version\n";
				break;
			case ATTR_NAME_ERR:
				errlog << "Error: attribute name misspelled or the attribute is not active.\n";
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
