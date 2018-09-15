//nom_to_ord.cpp - preprocessing of nominals. 
//Levels are sorted by average value of response and then converted to ordinals
//nom_levels - aggregating levels with low counts - should be run before nom_to_ord


#pragma warning(disable : 4996) //complaints about strerror function

#include <vector>
#include <string>
#include <string.h>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <errno.h>

using namespace std;

typedef vector<string> strv;
typedef vector<strv> strvv;
typedef vector<int> intv;
typedef vector<intv> intvv;

typedef pair<double, string> dspair;
typedef vector<dspair> dspv;

typedef pair<double, int> dipair;
typedef map<string, dipair> sdipmap;
typedef vector<sdipmap> sdipmv;

#define LINE_LEN 20000	//maximum length of line in the input file

void getLineExt(fstream& fin, char* buf);
bool mv(string& str);

//nominals _attr_file_ _data_file_
int main(int argc, char* argv[])
{
	try{

	//check presence of input arguments
	if(argc != 3)
		throw string("Usage: nom_to_ord _attr_file_ _data_file_");

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
	int labelNo = -1;
	for(attrNo = 0; fattr.gcount(); attrNo++)
	{
		string attrStr(buf);	//a line of an attr file (corresponds to 1 attribute)
		if(attrStr.find("nom") != string::npos)
			nomAttrs.push_back(attrNo);
		//find response class
		if(attrStr.find("class") != string::npos)
			labelNo = attrNo;
		if((attrStr.find("contexts") != -1) || (attrStr.find(":") == -1)) 
			break; //end of listed attributes
		getLineExt(fattr, buf);
	}
	int attrN = attrNo;
	int nomAttrN = (int)nomAttrs.size();
	if(labelNo == -1)
		throw string("Error: no class attribute");

	//read data from data file
	strvv data;
	strv item(attrN);
	for(attrNo = 0; attrNo < attrN; attrNo++)
		fdata >> item[attrNo];
	while(!fdata.fail())
	{
		data.push_back(item);
		for(attrNo = 0; attrNo < attrN; attrNo++)
			fdata >> item[attrNo];
	}
	int itemN = (int)data.size();

	//create count-based integer values for nominal features
	sdipmv counts(nomAttrN);
	for(int nomNo = 0; nomNo < nomAttrN; nomNo++)
	{
		attrNo = nomAttrs[nomNo];
		//collect counts;
		for(int itemNo = 0; itemNo < itemN; itemNo++)
		{
			if(mv(data[itemNo][labelNo]))
			  continue; //test set data point, no label
			string key = data[itemNo][attrNo];
			sdipmap::iterator countIt = counts[nomNo].find(key);
			if(countIt == counts[nomNo].end())
				countIt = counts[nomNo].insert(sdipmap::value_type(key, dipair(0, 0))).first;
			countIt->second.first += atof(data[itemNo][labelNo].c_str());
			countIt->second.second++;
		}		
		
		//calculate average label values, move them to a vector, sort
		dspv pairs;
		for(sdipmap::iterator countIt = counts[nomNo].begin(); countIt != counts[nomNo].end(); countIt++)
		{
			countIt->second.first /= countIt->second.second;
			pairs.push_back(dspair(countIt->second.first, countIt->first));
		}
		sort(pairs.begin(),pairs.end());

		int valN = (int)pairs.size();
		for(int valNo = 0; valNo < valN; valNo++)
		{
			sdipmap::iterator countIt = counts[nomNo].find(pairs[valNo].second);
			countIt->second.first = valNo;
		}
	}

	//output new data
	for(int itemNo = 0; itemNo < itemN; itemNo++)
	{
		int nomNo = 0;
		for(int attrNo = 0; attrNo < attrN; attrNo++)
		{
			if((nomNo >= nomAttrN) || (nomAttrs[nomNo] != attrNo))
				cout << data[itemNo][attrNo];
			else if(mv(data[itemNo][attrNo]))//"?" - missing value
			{
				cout << "?";
				nomNo++;
			}
			else 
			{
			  sdipmap::iterator countIt  = counts[nomNo].find(data[itemNo][attrNo]);
			  if(countIt == counts[nomNo].end()) //the value happens only in the test set
			    cout << "?";
			  else
			    cout << countIt->second.first;
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
		throw "Lines in an input file exceed current limit.";
}

bool mv(string& str)
{
	string trimstr = string(str.c_str());
	return !trimstr.compare("?");
}
