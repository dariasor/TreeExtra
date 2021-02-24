//Visualization / vis_correlations.cpp: main function of the executable vis_correlations
//
//(c) Daria Sorokina

#include "LogStream.h"
#include "ErrLogStream.h"
#include "functions.h"
#include "vis_definitions.h"
#include "INDdata.h"

#include <errno.h>

//vis_correlations -t _training_set_ -r _attr_file_  | -version
int main(int argc, char* argv[])
{
	try{
	//0. Set log file
	LogStream telog;
	telog << "\n-----\nvis_correlations ";
	for(int argNo = 1; argNo < argc; argNo++)
		telog << argv[argNo] << " ";
	telog << "\n\n";
	
	if((argc > 1) && !string(argv[1]).compare("-version"))
	{
		telog << "TreeExtra version " << VERSION << "\n";
		return 0;
	}

	//1. Set default values of parameters
	string trainFName;
	string attrFName;

	//2. Set parameters from command line
	//check that the number of arguments is even (flags + value pairs)
	if(argc % 2 == 0)
		throw VIS_INPUT_ERR;
	//convert input parameters to string from char*
	stringv args(argc); 
	for(int argNo = 0; argNo < argc; argNo++)
		args[argNo] = string(argv[argNo]);

	//parse and save input parameters
	//indicators of presence of required flags in the input
	bool hasTrain = false; 
	bool hasAttr = false;
	
	for(int argNo = 1; argNo < argc; argNo += 2)
	{
		if(!args[argNo].compare("-t"))
		{
			trainFName = args[argNo + 1];
			hasTrain = true;
		}
		else if(!args[argNo].compare("-r"))
		{
			attrFName = args[argNo + 1];
			hasAttr = true;
		}
		else
			throw VIS_INPUT_ERR;
	}

	if(!(hasTrain && hasAttr))
		throw VIS_INPUT_ERR;

//2. Load data
	INDdata data(trainFName.c_str(), "", "", attrFName.c_str());

//3. Calculate and output correlations
	data.correlations(trainFName);		

	}catch(TE_ERROR err){
		te_errMsg((TE_ERROR)err);
		return 1;
	}catch(VIS_ERROR err){
		ErrLogStream errlog;
		switch(err) 
		{
			case VIS_INPUT_ERR:
				errlog << "Usage: vis_correlations -t _training_set_ -r _attr_file_  | -version\n ";
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
