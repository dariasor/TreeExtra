//rnd_tv_dta executable: converts csv or dta files with or without header to randomly split files
//if all flag is set, everything goes into train file, in which case its name is just stem
//automatically identifies the empty line after the header. 
//for more information see --help.
//
// Some subsampling options at this point work only for files_int == 2 (train/validation split)
//
//(c) Daria Sorokina, Yichen Zhou

#pragma warning(disable : 4996)
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <algorithm>
#include <functional> 
#include <cctype>
#include <locale>


#include <string>
#include <map>
#include <vector>

using namespace std;

const int lineLen = 500000; //max size that I so far needed
const int shLineLen = 16;
enum DATASET
{
	NONE = 0,
	TRAIN = 1,
	VALID = 2,
	TEST = 3
};

typedef map<string, enum DATASET> sDmap;
typedef vector<string> stringv;

void outStr(string& str, fstream& fout, char delch, bool outDta);
int atoiExt(char* str);
double atofExt(char* str);
double atofExt(string str);
string trim(string s);
string cutCol(int inColNo, string str, char delch);

int main(int argc, char* argv[])
{
	string usage("Usage: rnd_tv_dta --input file_str --stem stem_str [--rand rand_int] [--in delim_str] [--out outtype_str] [--header] [--files-n files_int] [--group group_int] [--group-method group_method] \
[--train train_flt] [--valid valid_flt] [--target target_int] [--train-zero train_zero_flt] [--valid-zero valid_zero_flt]\n\n\
--input file_str\n\t\
file_str is the input file name, required parameter\n\n\
--stem stem_str\n\t\
stem_str is the prefix of the output files names, required parameter\n\n\
--rand rand_int\n\t\
rand_int is the random seed, default = 1\n\n\
--in delim_str\n\t\
delim_str identifies the delimiter in the input file, \"dta\" for tab and \"csv\" for comma. Default = \"dta\".\n\n\
--out outtype_str\n\t\
outtype_str identifies the format of the output file. \"dta\" for TreeExtra format(no header, missing values encoded as \"?\") and \"tsv\" for gbm format(header present, missing values encoded as omissions\
 between delimiters). Default = \"dta\".\n\n\
--header\n\t\
Indicates that the input file has header, by default it does not\n\n\
--files-n files_int\n\t\
files_int indicates the number of the output files : 1, 2 or 3. 2 means split into training and validation, 3 means training, validation and test. Default = 2.\n\n\
--group group_int\n\t\
group_int is the number of the column containing group values. 0 means no such column exists. Data points with identical group values end up in the same output file. Default = 0.\n\n\
--train train_flt\n\t\
train_flt is the proportion of the data that should go into the training set. Default = switch(files_int) {case 1: 1.0; case 2: 0.5; case 3: 0.333}.\n\n\
--valid valid_flt\n\t\
valid_flt is the proportion of the data that should go into the validation set. Default = switch(files_int) {case 2: 0.5; case 3: 0.333}.\n\n\
--group-method group_method\n\t\
group_method decides how we sample the points when grouping is on. 0: sample groups, keep all points in a group; 1: sample points from different groups for each data set. Default = 0.\n\n\
--target target_int\n\t\
target_int is the number of the column containing target. Needed only if --train_zero and/or --valid_zero are set. Default = 1.\n\n\
--train-zero train_zero_flt\n\t\
train_zero_flt is the probability to end up in the training data set for the data points with the target equal to zero. Default = train_flt.\n\n\
--valid-zero valid_zero_flt\n\t\
valid_zero_flt is the probability to end up in the training data set for the data points with the target equal to zero. Default = valid_flt.\n\n\
		");
	try{

	//mandatory arguments
	string fName(""), stem("");
	bool hasInput = false, hasStem = false;

	//set up default values of extra arguments
	char delch = '\t';
	bool hasHeader = false, all = false, test = false, doGroup = false, outDta = true, getTarget = false;
	int groupMethod = 0;
	int groupNo = -1, targetNo = -1, seed = 1;
	double propVal = -1, propTrain = -1, propValZero = -1, propTrainZero = -1;
	double groupPropVal = 1, groupPropTrain = 1;
	string outType("dta");

	//convert input parameters to string from char*
	stringv args(argc);
	for(int argNo = 0; argNo < argc; argNo++)
		args[argNo] = string(argv[argNo]);

	//set values from command line
	for(int argNo = 1; argNo < argc; argNo++)
	{
		if(!args[argNo].compare("--header"))
			hasHeader = true;
		else if(++argNo == argc)
			throw usage;
		else if(!args[argNo - 1].compare("--input"))
		{
			fName = args[argNo];
			hasInput = true;
		}
		else if(!args[argNo - 1].compare("--stem"))
		{
			stem = args[argNo];
			hasStem = true;
		}
		else if(!args[argNo - 1].compare("--rand"))
			seed = atoiExt(argv[argNo]);
		else if(!args[argNo - 1].compare("--in"))
		{
			if(!args[argNo].compare("dta"))
				delch = '\t';
			else if(!args[argNo].compare("csv"))
				delch = ',';
			else
				throw usage;
		}
		else if(!args[argNo - 1].compare("--out"))
		{
			outType = args[argNo];
			if(!outType.compare("dta"))
				outDta = true;
			else if(!outType.compare("tsv"))
				outDta = false;
			else
				throw usage;
		}
		else if(!args[argNo - 1].compare("--files-n"))
		{
			if(!args[argNo].compare("1"))
			{
				all = true;
				test = false;
				if(propTrain == -1)
					propTrain = 1.0;
				propVal = 0.0;
			}
			else if(!args[argNo].compare("2"))
			{
				all = false;
				test = false;
				if(propTrain == -1)
					propTrain = 0.5;
				if(propVal == -1)
					propVal = 0.5;
			}
			else if(!args[argNo].compare("3"))
			{
				all = false;
				test = true;
				if(propTrain == -1)
					propTrain = 0.333;
				if(propVal == -1)
					propVal = 0.333;
			}
			else
				throw usage;
		}
		else if(!args[argNo - 1].compare("--group"))
		{
			groupNo = atoiExt(argv[argNo]) - 1;
			doGroup = (groupNo >= 0);
		}
		else if (!args[argNo - 1].compare("--target"))
			targetNo = atoiExt(argv[argNo]) - 1;
		else if(!args[argNo - 1].compare("--group-method"))
			groupMethod = atoiExt(argv[argNo]);
		else if(!args[argNo - 1].compare("--train"))
			propTrain = atofExt(argv[argNo]);
		else if(!args[argNo - 1].compare("--valid"))
			propVal = atofExt(argv[argNo]);
		else if (!args[argNo - 1].compare("--train-zero"))
			propTrainZero = atofExt(argv[argNo]);
		else if (!args[argNo - 1].compare("--valid-zero"))
			propValZero = atofExt(argv[argNo]);
		else
			throw usage;
	}

	if(!(hasInput && hasStem))
		throw usage;

	if(propTrainZero == -1)
		propTrainZero = propTrain;
	if(propValZero == -1)
		propValZero = propVal;
	if((propTrain != propTrainZero) || (propVal != propValZero))
	{	
		getTarget = true;
		if(targetNo == -1)
			throw string("Error: target column number is not specified. ");
	}

	if(all && (propVal || propValZero))
		throw string("Error: cannot set validation set parameters in the mode --files-n 1");

	//decide the proportion of each group
	if(doGroup && (groupMethod == 1)){
		if(all)  //only train
		{
			groupPropTrain = 1.0;
			groupPropVal = 0;
		}
		else if(test)  //all train, valid and test
		{
			groupPropTrain = 0.333;
			groupPropVal = 0.333;
			propTrain /= groupPropTrain;
			propVal /= groupPropVal;
			propTrainZero /= groupPropTrain;
			propValZero /= groupPropVal;
		}
		else  //train and valid
		{
			groupPropTrain = 0.5;
			groupPropVal = 0.5;
			propTrain /= groupPropTrain;
			propVal /= groupPropVal;
			propTrainZero /= groupPropTrain;
			propValZero /= groupPropVal;
		}
	}

	//open files, check that they are there
	fstream fdata(fName.c_str(), ios_base::in);
	if(!fdata)
		throw string("Error: failed to open file ") + string(fName);

	string trainFName(stem);
	if(!all)
		trainFName += ".train";
	trainFName += "." + outType;

	fstream ftrain(trainFName.c_str(), ios_base::out);
	if(!ftrain)
		throw string("Error: failed to open file ") + trainFName;

	string validFName(stem + ".valid." + outType);
	fstream fvalid;
	if(!all)
	{
		fvalid.open(validFName.c_str(), ios_base::out);
		if(!fvalid)
			throw string("Error: failed to open file ") + validFName;
	}

	string testFName(stem + ".test." + outType);
	fstream ftest;
	if(!all && test)
	{
		ftest.open(testFName.c_str(), ios_base::out);
		if(!ftest)
			throw string("Error: failed to open file ") + testFName;
	}

	srand(seed);
	
	char buf[lineLen];
	if(hasHeader)
	{
		fdata.getline(buf, lineLen); //header, skip it
		string header(buf);
		if(!outDta)
		{
			outStr(header, ftrain, delch, outDta);
			if(!all)
			{
				outStr(header, fvalid, delch, outDta);
				if(test)
					outStr(header, ftest, delch, outDta);
			}
		}
	}
	fdata.getline(buf, lineLen);
	if(hasHeader && (fdata.gcount() == 0))
	{
		fdata.getline(buf, lineLen); //skip empty string
	}
	
	sDmap groups; //needed if grouping is on
	string groupID;
	sDmap::iterator groupIt = groups.end();
	double targetVal;

	if(!doGroup)
	{
		for(int i = 1; !fdata.fail(); i++)
		{
			if(i % 100000 == 0) cout << "read " << i << " lines" << endl;

				string str(buf);
				if(getTarget)
					targetVal = atofExt(cutCol(targetNo, str, delch));

				double randCoef = (double) rand() / RAND_MAX;
				if(
					(getTarget && (targetVal == 0) && (randCoef < propTrainZero)) ||
					((getTarget && (targetVal != 0) || !getTarget) && (randCoef < propTrain))
				  )
					outStr(str, ftrain, delch, outDta);
				else if ( !all && (
						 (getTarget && (targetVal == 0) && (randCoef >= (1 - propValZero))) ||
						 ((getTarget && (targetVal != 0) || !getTarget) && (randCoef >= (1 - propVal)))
						))
					outStr(str, fvalid, delch, outDta);
				else if(test)
					outStr(str, ftest, delch, outDta);

				fdata.getline(buf, lineLen); 
		}
	}
	else if(groupMethod == 0)
	{
		for (int i = 1; !fdata.fail(); i++)
		{
			if (i % 100000 == 0) cout << "read " << i << " lines" << endl;

			string str(buf);
			groupID = cutCol(groupNo, str, delch);
			groupIt = groups.find(groupID);

			enum DATASET dataset = NONE;
			double randCoef = (double)rand() / RAND_MAX;
			if(
				(groupIt != groups.end()) && (groupIt->second == TRAIN) ||
				(groupIt == groups.end()) && (randCoef < propTrain)
				)
			{//output to train
				outStr(str, ftrain, delch, outDta);
				dataset = TRAIN;
			}
			else if(!all && (
				(groupIt != groups.end()) && (groupIt->second == VALID) ||
				(groupIt == groups.end() && (randCoef >= (1 - propVal)))
				))
			{//output to validation
				outStr(str, fvalid, delch, outDta);
				dataset = VALID;
			}
			else if(test){
				outStr(str, ftest, delch, outDta);
				dataset = TEST;
			}
			if(groupIt == groups.end())
				groups.insert(sDmap::value_type(groupID, dataset));

			fdata.getline(buf, lineLen);
		}
	}
	else if(groupMethod == 1)
	{
		for(int i = 1; !fdata.fail(); i++)
		{
			if(i % 100000 == 0) cout << "read " << i << " lines" << endl;
			string str(buf);

			if(doGroup)
			{
				groupID = cutCol(groupNo, str, delch);
				groupIt = groups.find(groupID);
			}
			if(getTarget)
				targetVal = atofExt(cutCol(targetNo, str, delch));

			enum DATASET dataset = NONE;
			double randGroupCoef = (double) rand() / RAND_MAX;
			double randCoef = (double) rand() / RAND_MAX;
			if(
			   ((groupIt != groups.end()) && (groupIt->second == TRAIN)) ||
			   ((groupIt == groups.end()) && (randGroupCoef < groupPropTrain))
			  )
			{//output to train
				if((getTarget && (targetVal == 0) && (randCoef < propTrainZero)) ||
					((getTarget && (targetVal != 0) || !getTarget) && (randCoef < propTrain)))
					outStr(str, ftrain, delch, outDta);
				dataset = TRAIN;
			}
			else if(!all && (
					((groupIt != groups.end()) && (groupIt->second == VALID)) ||
					((groupIt == groups.end()) && (randGroupCoef >= 1 - groupPropVal))
					))
			{//output to validation
				if((getTarget && (targetVal == 0) && (randCoef < propValZero)) ||
					((getTarget && (targetVal != 0) || !getTarget) && (randCoef < propVal)))
					outStr(str, fvalid, delch, outDta);
				dataset = VALID;
			} else if(test){
				 //output to test 
				outStr(str, ftest, delch, outDta);
				dataset = TEST;
			}
			if(doGroup && (groupIt == groups.end()))
				groups.insert(sDmap::value_type(groupID, dataset));

			fdata.getline(buf, lineLen); 
		}
	}

	fdata.close();
	ftrain.close();
	if(!all)
	{
		fvalid.close();
		if(test)
			ftest.close();
	}

	}
	catch(string err){
		if(err.compare(usage) == 0)
			cout << err << endl;
		else
			cerr << err << endl;
		return 1;
	}
	catch(exception &e){
		string errstr(e.what());
		cerr << "Error: " << errstr << "\n";
		return 1;
	}
	catch(...){
		string errstr = strerror(errno);
		cerr << "Error: " << errstr << endl;
		return 1;
	}
	return 0;
}

void outStr(string& str_in, fstream& fout, char delch, bool outDta)
{
	if(str_in.empty())
		return;
	//output the line, replacing commas with tabs
	string str(str_in);
	while(true)
	{
		string::size_type delim = str.find(delch);
		string field = trim(str.substr(0, delim));
		if(field.empty() && outDta)
			field = "?";
		fout << field;

		if(delim == string::npos)
			break;

		fout << '\t';
		str = str.substr(delim + 1);
	}
	fout << endl;
}

//converts string to int, throws error if the string is unconvertable
int atoiExt(char* str)
{
	char *end;
	long value = strtol(str, &end, 10);
	if((end == str) || (*end != '\0'))
		throw "Error: non-numeric value for a numeric argument.\n";
	return (int)value;
}

//converts string to double, throws error if the string is unconvertable
double atofExt(char* str)
{
	char *end;
	double value = strtod(str, &end);
	if((end == str) || (*end != '\0'))
		throw  "Error: non-numeric value for a numeric argument.\n";
	return value;
}

double atofExt(string str)
{
	return atofExt((char*)str.c_str());
}

//slightly modified trim function from stack overflow by Evan Teran
//http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
string trim(string s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(),
            not1(ptr_fun<int, int>(isspace))));
    s.erase(find_if(s.rbegin(), s.rend(),
            not1(ptr_fun<int, int>(isspace))).base(), s.end());
	return s;
}

string cutCol(int inColNo, string str, char delch)
{
	string::size_type prevDelim = -1;
	for (int colNo = 0; colNo < inColNo; colNo++)
		prevDelim = str.find(delch, prevDelim + 1);
	string::size_type delim = str.find(delch, prevDelim + 1);
	return str.substr(prevDelim + 1, delim - prevDelim - 1);
}