//attrbool.cpp: the single source file for the attrbool.exe tool
//takes 
	//- either an "all cont" attribute file, or a list of attributes, one name per line in the right order 
	//- the data file
	//- the name of the class attribute
//produces a new attribute file with boolean features
//marked as "0,1" and useless (only 0 values) features as "never"
//important note: it loses all information about original types of attributes (in particular, nominals), 
//and inactive attributes. All this information has to be restored in the new file manually.

#pragma warning(disable : 4996) //complaints about strerror function

#include <vector>
#include <string>
#include <limits>
#include <fstream>
#include <iostream>
#include <sstream>
#include <errno.h>
#include <math.h>
#include <string.h>

using namespace std;

typedef vector<float> floatv;
typedef vector<int> intv;
typedef vector<string> stringv;
typedef numeric_limits<float> flim;

#define LINE_LEN 20000	//maximum length of line in the input file
#define QNAN flim::quiet_NaN()

#if defined(__VISUALC__)
    #define wxisNaN(n) _isnan(n)
#elif defined(__GNUC__)
    #define wxisNaN(n) isnan(n)
#else
    #define wxisNaN(n) ((n) != (n))
#endif

void getLineExt(fstream& fin, char* buf);
bool mv(string& str);
string trimSpace(string& str);
void readData(char* buf, int buflen, floatv& retv, int retvlen);

//attrbool _attr_file_ _data_file_ _new_attr_file_
int main(int argc, char* argv[])
{
	try{
	//check presence of input arguments
	if(argc != 5)
		throw string("Usage: attrbool _attr_file_ _data_file_ _new_attr_file_ _target_name_");

	//open files, check that they are there
	fstream fattr(argv[1], ios_base::in);
	if(!fattr)
		throw string("Error: failed to open attribute file ") + string(argv[1]);

	fstream fdata(argv[2], ios_base::in);
	if(!fdata)
		throw string("Error: failed to open data file ") + string(argv[2]);

	fstream fnew(argv[3], ios_base::out);
	if(!fnew)
		throw string("Error: failed to open data file ") + string(argv[3]);

	string tarName(argv[4]);

	//Read attribute file	
	
	//read attr file, collect info about boolean attributes and attrN
	char buf[LINE_LEN];	//buffer for reading from input files
 	getLineExt(fattr, buf);

	//read names of attributes, figure out their number
	stringv attrNames;
	int attrN; // counter
	for(attrN = 0; fattr.gcount(); attrN++)
	{
		string attrStr(buf);	//a line of an attr file (corresponds to 1 attribute)
		attrStr = trimSpace(attrStr);
		if((attrStr.find("contexts") != -1) || (attrStr.size() == 0)) 
			break; //end of listed attributes
		
		//parse attr name
		string::size_type colonPos = attrStr.find(":");
		string::size_type nameLen = (colonPos == string::npos) ? attrStr.size() : colonPos;
		string attrName = attrStr.substr(0, nameLen);
		attrNames.push_back(trimSpace(attrName));

		getLineExt(fattr, buf);
	}
	fattr.close();

//Read data file, collect info about attribute values
	intv attrFlags(attrN, 0); //0 - only zero values, 1 - 0/1 values, 2 - other values present (mv ignored)

	getLineExt(fdata, buf);
	floatv item;	//single data point
	int caseNo;
	for(caseNo = 0; fdata.gcount(); caseNo++)
	{//read one line of data file, save class value in targets, attribute values in data
		if((caseNo + 1) % 100000 == 0)
			cout << "Analyzed " << caseNo + 1 << " lines" << endl;

		try
		{
			readData(buf, (int)fdata.gcount(), item, attrN);
		}
		catch(string err)
		{
			stringstream s;
			s << "Line " << caseNo + 1 << ". " << err;
			throw s.str();
		}

		for(int attrNo = 0; attrNo < attrN; attrNo++)
			if(!wxisNaN(item[attrNo]) && (item[attrNo] != 0) && (attrFlags[attrNo] < 2))
				if(item[attrNo] == 1)
					attrFlags[attrNo] = 1;
				else 
					attrFlags[attrNo] = 2;
		getLineExt(fdata, buf);
	}
	fdata.close();

//Output new attribute file
	bool tarFound = false;
	for(int attrNo = 0; attrNo < attrN; attrNo++)
	{
		fnew << attrNames[attrNo] << ": ";
		if(attrFlags[attrNo] == 2)
			fnew << "cont"; 
		else
			fnew << "0,1";
		if (tarName.compare(attrNames[attrNo]) == 0)
		{
			fnew << "(class)";
			tarFound = true;
		}

		fnew << "." << endl;
	}
	if (!tarFound)
		throw string("Error: Could not find the target attribute ") + tarName;

	fnew << "contexts:" << endl;
	for(int attrNo = 0; attrNo < attrN; attrNo++)
		if(attrFlags[attrNo] == 0)
			fnew << attrNames[attrNo] << " never" << endl;
	fnew.close();

	}catch(string err){
	  cerr << err << endl;
		return 1;
	}catch(...){
		string errstr = strerror(errno);
		cerr << errstr << endl;
		return 1;
	}
	return 0;
}

//extends fstream::getline with check on exceeding the buffer size
void getLineExt(fstream& fin, char* buf)
{
	fin.getline(buf, LINE_LEN);
	if(fin.gcount() == LINE_LEN - 1)
		throw string("Error: lines in an input file exceed current limit.");
}

bool mv(string& str)
{
	string trimstr = string(str.c_str());
	return !trimstr.compare("?");
}

//Deletes spaces from the beginning and from the end of the string
//By "spaces" I mean spaces only, not white spaces
string trimSpace(string& str)
{
	int n = (int)str.size();
	
	int b; 
	for(b = 0; (b < n) && (str[b] == ' '); b++);
	
	if(b == n)
	{
		str.clear();
		return string();
	}
	
	int e;
	for(e = n - 1; (str[e] == ' ') || (str[e] == '\r'); e--);
	
	return str.substr(b, e - b + 1);
}

//Gets a line of text, returns a vector with data points 
//Missing values should be encoded as '?', they get converted to NANs
void readData(char* buf, int buflen, floatv& retv, int retvlen)
{
	//replace spaces with '_' (there can be spaces in nominal values, space should not be a delimiter)
	for(int chNo = 0; chNo < buflen; chNo ++)
		if(buf[chNo] == ' ')
			buf[chNo] = '_';

	retv.resize(retvlen);
	stringstream itemstr(buf);
	string singleItem;
	for(int attrId = 0; attrId < retvlen; attrId++)
	{
		itemstr >> singleItem;
		if(itemstr.fail())
			throw string("Error: fewer attributes than in the attribute file.");
		singleItem = string(singleItem.c_str()); //to trim '\0'
		if(singleItem.compare("?"))
		{//should be a number, convert it
			stringstream sistr(singleItem);
			sistr >> retv[attrId];
		}
		else //missing value
			retv[attrId] = QNAN;
	}

	itemstr >> singleItem;
	if(!itemstr.fail())
		throw string("Error: more attributes than in the attribute file.");
	
}
