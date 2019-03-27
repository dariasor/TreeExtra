//Visualization / vis_iplot.cpp: main function of executable vis_iplot
//
//(c) Daria Sorokina

#include "Grove.h"
#include "TrainInfo.h"
#include "LogStream.h"
#include "ErrLogStream.h"
#include "functions.h"
#include "vis_definitions.h"
#include "ag_functions.h"

#include <errno.h>



//vis_iplot -v _validation_set_ -r _attr_file_ -f1 _feature1_ -f2 _feature2_ [-q1 _#quantile_values1_] 
//[-q2 _#quantile_values2_] [-m _model_file_name_] [-o _output_file_suffix_] [-x _fixed_values_file_] | -version
int main(int argc, char* argv[])
{	 
	try{
	//0. Set log file
	LogStream telog;
	telog << "\n-----\nvis_iplot ";
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
	string suffix;					//suffix for the output files
	string fixedFName;				//name of the input file for fixed attributes and their values
	int quantN1 = 10;	//number of quantile point values to plot for feature 1
	int quantN2 = 10;	//number of quantile point values to plot for feature 2

	TrainInfo ti;
	string attrName1, attrName2; //interacting attribute names

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
	bool hasVal = false; 
	bool hasAttr = false;
	bool hasF1 = false;
	bool hasF2 = false;
	
	for(int argNo = 1; argNo < argc; argNo += 2)
	{
		if(!args[argNo].compare("-m"))
			modelFName = args[argNo + 1];
		else if(!args[argNo].compare("-o"))
			suffix = args[argNo + 1];
		else if(!args[argNo].compare("-x"))
			fixedFName = args[argNo + 1];
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
		else if(!args[argNo].compare("-f1"))
		{
			attrName1 = args[argNo + 1];
			hasF1 = true;
		}
		else if(!args[argNo].compare("-f2"))
		{
			attrName2 = args[argNo + 1];
			hasF2 = true;
		}
		else if(!args[argNo].compare("-q1"))
			quantN1 = atoi(argv[argNo + 1]);
		else if(!args[argNo].compare("-q2"))
			quantN2 = atoi(argv[argNo + 1]);
		else
			throw VIS_INPUT_ERR;
	}

	if(!(hasVal && hasAttr && hasF1 && hasF2))
		throw VIS_INPUT_ERR;

//2. Load data
	INDdata data(ti.trainFName.c_str(), ti.validFName.c_str(), ti.testFName.c_str(), 
				 ti.attrFName.c_str());
	CGrove::setData(data);
	CTreeNode::setData(data);

//3. Calculate and output data for the interaction plot 
	int attrId1 = data.getAttrId(attrName1);
	int attrId2 = data.getAttrId(attrName2);
	if(!data.isActive(attrId1) || !data.isActive(attrId2))
		throw ATTR_NAME_ERR;

	outIPlots(data, iipairv(1, iipair(attrId1, attrId2)), quantN1, quantN2, modelFName, 
			  suffix, fixedFName);

	string in_suffix;
	if(suffix.size())
		in_suffix = "." + suffix;
	string outFName = attrName1 + "." + attrName2 + in_suffix + ".iplot.txt";

	string denFName = insertSuffix(outFName, "dens");

	telog << "Joint effect values are saved into file " << outFName << ".\n";
	telog << "Density table is saved into file " << denFName << ".\n";

	}catch(TE_ERROR err){
		te_errMsg((TE_ERROR)err);
		return 1;
	}catch(VIS_ERROR err){
		ErrLogStream errlog;
		switch(err) 
		{
			case VIS_INPUT_ERR:
				errlog << "Usage: -v _validation_set_ -r _attr_file_ -f1 _feature1_ -f2 _feature2_ "
					<< "[-q1 _#quantile_values1_] [-q2 _#quantile_values2_] [-m _model_file_name_] "
					<< "[-o _output_file_suffix_] [-x _fixed_values_file_] | -version\n";
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
