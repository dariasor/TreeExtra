//cont_rms.cpp: main function of executable cont_rms
//Calculates root mean squared error from predictions and target values. 
//Both targets and predictions can take on any continuous values
//
//(c) Daria Sorokina, Carnegie Mellon University, 2009

#pragma warning(disable : 4996)

#include <fstream>
#include <iostream>
#include <errno.h>
#include <vector>
#include <string>
#include <math.h>
#include "string.h"

using namespace std;

typedef vector<double> doublev;

//cont_rms _targets_ _predictions_
int main(int argc, char* argv[])
{
	try{

	//check presence of input arguments
	if(argc != 3)
		throw string("Usage: cont_rms _targets_ _predictions_");

	fstream ftar(argv[1], ios_base::in);
	if(!ftar)
		throw string("Error: failed to open file ") + string(argv[1]);

	fstream fpred(argv[2], ios_base::in);
	if(!fpred)
		throw string("Error: failed to open file ") + string(argv[2]);

	//load target and prediction values;
	doublev tars, preds;
	double hold;
	ftar >> hold;
	while(!ftar.fail())
	{
		tars.push_back(hold);
		ftar >> hold;
	}
	fpred >> hold;
	while(!fpred.fail())
	{
		preds.push_back(hold);
		fpred >> hold;
	}
	if(tars.size() != preds.size())
		throw string("Error: different number of values in targets and predictions files");

	int itemN = tars.size();

	double mse = 0;
	for(int itemNo = 0; itemNo < itemN; itemNo++)
		mse += pow((preds[itemNo] - tars[itemNo]), 2);

	mse /= itemN;
	cout << "RMSE: " << sqrt(mse) << endl;

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
