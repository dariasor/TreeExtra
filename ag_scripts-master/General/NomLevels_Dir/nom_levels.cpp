//nom_levels.cpp: the single source file for the nom_levels.exe tool
//Implements idea of Uni Melb team from KDD Cup'09. 
//"levels with fewer than 100 instances in the training data are aggregated into a "small" category, 
//those with 100-500 instances are aggregated into a "medium" category, and those with 500-1000 instances 
//aggregated into a "large" category." Here we take a minimum between absolute Uni Melb thresholds and thresholds relative 
// to the train set size: < 1/500, 1/500 - 1/100, 1/100 - 1/50. 
//
//Here "small" = -3, "medium" = -2, "large" = -1.

#pragma warning(disable : 4996) //complaints about strerror function

#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <errno.h>
#include <string.h>

using namespace std;

typedef vector<int> intv;
typedef vector<string> strv;
typedef vector<strv> strvv;

typedef map<string, int> strimap;
typedef vector<strimap> strimapv;


#define LINE_LEN 20000	//maximum length of line in the input file

void getLineExt(fstream& fin, char* buf);
bool mv(string& str);
void readData(char* buf, streamsize buflen, strv& retv, int retvlen);

//nom_levels _attr_file_ _data_file_
int main(int argc, char* argv[])
{
	try{

	//check presence of input arguments
	if(argc != 3)
		throw string("Usage: nom_levels _attr_file_ _data_file_");

	//open files, check that they are there
	fstream fattr(argv[1], ios_base::in);
	if(!fattr)
		throw string("Error: failed to open attribute file ") + string(argv[1]);

	fstream fdata(argv[2], ios_base::in);
	if(!fdata)
		throw string("Error: failed to open data file ") + string(argv[2]);

	//read info from attr file
	intv nomAttrs; //ids of nominal features

	char buf[LINE_LEN];	//buffer for reading from input files
	getLineExt(fattr, buf);
	int attrNo;
	for(attrNo = 0; fattr.gcount(); attrNo++)
	{
		string attrStr(buf);	//a line of an attr file (corresponds to 1 attribute)
		if(attrStr.find("nom") != string::npos)
			nomAttrs.push_back(attrNo);

		if((attrStr.find("contexts") != -1) || (attrStr.find(":") == -1))
			break; //end of listed attributes

		getLineExt(fattr, buf);
	}
	int attrN = attrNo;
	int nomAttrN = (int)nomAttrs.size();
	
	//read data from data file
	strvv data;
	strv item(attrN);
	getLineExt(fdata, buf);
	int itemNo;
	for(itemNo = 0;	fdata.gcount(); itemNo++)
	{
		readData(buf, fdata.gcount(), item, attrN);
		data.push_back(item);
		getLineExt(fdata, buf);
	}
	int itemN = itemNo;

	//figure out level thresholds
	int thresh[3];
	thresh[0] = min(itemN / 500, 100);
	thresh[1] = min(itemN / 100, 500);
	thresh[2] = min(itemN / 50, 1000);

	strimapv counts(nomAttrN);
	//work through nominals, figure out the required modifications
	for(int nomNo = 0; nomNo < nomAttrN; nomNo++)
	{
		attrNo = nomAttrs[nomNo];
		//collect counts;
		for(int itemNo = 0; itemNo < itemN; itemNo++)
		{
			string key = data[itemNo][attrNo];
			strimap::iterator countIt = counts[nomNo].find(key);
			if(countIt == counts[nomNo].end())
				countIt = counts[nomNo].insert(strimap::value_type(key, 0)).first;
			countIt->second++;
		}

		//replace counts with categories. 0 means "leave as is".
		for(strimap::iterator countIt = counts[nomNo].begin(); countIt != counts[nomNo].end(); countIt++)
			if(countIt->second < thresh[0])
				countIt->second = -3;
			else if(countIt->second < thresh[1])
				countIt->second = -2;
			else if(countIt->second < thresh[2])
				countIt->second = -1;
			else
				countIt->second = 0;
	}

	//output new data
	for(int itemNo = 0; itemNo < itemN; itemNo++)
	{
		int nomNo = 0;
		for(int attrNo = 0; attrNo < attrN; attrNo++)
		{
			if((nomNo >= nomAttrN) || (nomAttrs[nomNo] != attrNo))
				cout << data[itemNo][attrNo];
			else
			{
				int countId = counts[nomNo].find(data[itemNo][attrNo])->second;
				if(mv(data[itemNo][attrNo]) || (countId == 0))
					cout << data[itemNo][attrNo];
				else
					cout << countId;
				nomNo++;
			}
			if(attrNo < attrN - 1)
				cout << "\t";
		}
		cout << endl;
	}


	}catch(string err){
		cerr << err << endl;
		return 1;
	}catch(...){
		string errstr = strerror(errno);
		cerr << "Error: " << errstr << endl;
		return 1;
	}
	return 0;
}

//extends fstream::getline with check on exceeding the buffer size
void getLineExt(fstream& fin, char* buf)
{
	fin.getline(buf, LINE_LEN);
	if(fin.gcount() == LINE_LEN - 1)
		throw string("Erros: lines in an input file exceed current limit.");
}

bool mv(string& str)
{
	string trimstr = string(str.c_str());
	return !trimstr.compare("?");
}

void readData(char* buf, streamsize buflen, strv& retv, int retvlen)
{
	//remove spaces (there can be spaces in nominal values, space should not be a delimiter
	//and also should be ignored when it is next to a number)

	string line;
	line.reserve((size_t)buflen);
	for(int chNo = 0; chNo < buflen; chNo ++)
		if(buf[chNo] != ' ')
			line.push_back(buf[chNo]);

	retv.resize(retvlen);
	stringstream itemstr(line.c_str());
	string singleItem;
	for(int attrId = 0; attrId < retvlen; attrId++)
	{
		itemstr >> retv[attrId];
		if(itemstr.fail())
			throw string("Error: data has fewer attributes than listed in the attribute file.");
	}

	itemstr >> singleItem;
	if(!itemstr.fail())
	  throw string("Error: data has more attributes than listed in the attribute file.");
}
