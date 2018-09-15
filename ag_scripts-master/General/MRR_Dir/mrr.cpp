//mrr.cpp: main function of executable ndcg
//Calculates MRR from predictions and group and target values. Reciprocal rank (RR) is calculated separately
//for each group, then averaged.
//
//(c) Daria Sorokina

#pragma warning(disable : 4996)

#include <fstream>
#include <iostream>
#include <errno.h>
#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <string.h>
#include "math.h"


using namespace std;

typedef vector<double> doublev;
typedef pair<double, double> ddpair;
typedef vector<ddpair> ddpairv;
typedef set<int> intset;
typedef vector<int> intv;
typedef pair<string, int> sipair;
typedef vector<sipair> sipairv;
typedef vector<string> stringv;

double atofExt(string str)
{
	char *end;
	double value = strtod(str.c_str(), &end);
	if((end == str) || (*end != '\0'))
		throw string("Error: non-numeric value \"") + string(str) + string("\"");
	return value;
}

bool greater1(const ddpair& d1, const ddpair& d2)
{
	return (d1.first > d2.first);
}

double rr(doublev::iterator preds, doublev::iterator tars, int itemN)
{
	ddpairv data(itemN);
	for(int i = 0; i < itemN; i++)
	{
		data[i].first = preds[i];
		data[i].second = tars[i];
	}

	sort(data.begin(), data.end(), greater1);

	int firstRank = -1;
	for(int i = 0; (i < itemN) && (firstRank == -1); i++)
	{
		if(data[i].second > 0.0)	//relevant
			firstRank = i + 1;
	}
	return (firstRank == -1) ? 0 : (1.0 / firstRank);
}

//rr averaged across groups
double mrr(doublev& preds_in, doublev& tars_in, stringv& groups_in, bool sorted)
{
	//if the data is not sorted by group, create sorted copies of all input arrays
	int itemN = (int)preds_in.size();
	doublev preds_s, tars_s; 
	stringv groups_s;
	if(!sorted)
	{
		sipairv groupids;
		groupids.resize(itemN);
		for(int itemNo = 0; itemNo < itemN; itemNo++)
		{
			groupids[itemNo].first = groups_in[itemNo];
			groupids[itemNo].second = itemNo;
		}
		sort(groupids.begin(), groupids.end());
		
		preds_s.resize(itemN);
		tars_s.resize(itemN);
		groups_s.resize(itemN);
		for(int itemNo = 0; itemNo < itemN; itemNo++)
		{
			preds_s[itemNo] = preds_in[groupids[itemNo].second];
			tars_s[itemNo] = tars_in[groupids[itemNo].second];
			groups_s[itemNo] = groups_in[groupids[itemNo].second];
		}
	}
	doublev& preds = sorted ? preds_in : preds_s;
	doublev& tars = sorted ? tars_in : tars_s;
	stringv& groups = sorted ? groups_in : groups_s;

	//now calculate rr for every group
	double meanVal = 0;
	int groupN = 0;
	int curBegins = 0;
	string curGr = groups[0];
	for(int itemNo = 0; itemNo < itemN; itemNo++)
	{
		if((itemNo == itemN - 1) || (groups[itemNo + 1].compare(curGr) != 0))
		{
			groupN++;
			meanVal += rr(preds.begin() + curBegins, tars.begin() + curBegins, itemNo + 1 - curBegins);

			if(itemNo != itemN - 1)
			{
				curGr = groups[itemNo + 1];
				curBegins = itemNo + 1;
			}
		}
	}
	return meanVal / groupN;
}


//mrr _targets_ _predictions_ _groups_
int main(int argc, char* argv[])
{
	try{

	//check presence of input arguments
	if(argc != 4)
		throw string("Usage: mrr _targets_ _predictions_ _groups_");

	fstream ftar(argv[1], ios_base::in);
	if(!ftar)
		throw string("Error: failed to open file ") + string(argv[1]);

	fstream fpred(argv[2], ios_base::in);
	if(!fpred)
		throw string("Error: failed to open file ") + string(argv[2]);

	fstream fgroup(argv[3], ios_base::in);
	if(!fgroup)
		throw string("Error: failed to open file ") + string(argv[3]);

	//load target and prediction values;
	doublev tars, preds; 
	stringv groups;
	double hold_d;
	string hold_s;
	fpred >> hold_s;
	try
	{
		hold_d = atofExt(hold_s);
	}
	catch(...) {
		//header present, remove it form all columns
		ftar >> hold_s;
		fgroup >> hold_s;
		fpred >> hold_s;
		hold_d = atofExt(hold_s);
	}

	while(!fpred.fail())
	{
		preds.push_back(hold_d);
		fpred >> hold_s;
		hold_d = atofExt(hold_s);
	}
	ftar >> hold_d;
	while(!ftar.fail())
	{
		tars.push_back(hold_d);
		ftar >> hold_d;
	}
	fgroup >> hold_s;
	while(!fgroup.fail())
	{
		groups.push_back(hold_s);
		fgroup >> hold_s;
	}
	if(tars.size() != preds.size())
		throw string("Error: different number of values in targets and predictions files");
	if(tars.size() != groups.size())
		throw string("Error: different number of values in targets and groups files");

	double retVal = mrr(preds, tars, groups, false);
	cout << "MRR: " << retVal << endl;

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
