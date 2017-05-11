//SplitInfo.cpp: implementation of SplitInfo structure
//
// (c) Daria Sorokina

#include "SplitInfo.h"
#include "math.h"

//default constructor
SplitInfo::SplitInfo():divAttr(-1)
{
	missingL = QNAN;
	border = QNAN;
}

//init constructor
//missingL sometimes is set later
SplitInfo::SplitInfo(int attr, double point, double miss): 
	divAttr(attr), border(point), missingL(miss)
{

}


//in: value of the split attribute for the input case
//out: value of the coefficient for the case regarding going down the left branch
double SplitInfo::leftCoef(double value)
{
	//left coefficient for missing values is defined by missingL 
	if(isnan(value))
		return missingL;

	//absence of border value indicates special split - non-missing vs missing. 
	//Missing values are treated in the previous if, so this can only be a non-missing value that should go left
	if(isnan(border))	
		return 1;
	
	//the rest is standard crisp split
	if(value <= border)
		return 1;
	else //if(value > border)
		return 0;
}

