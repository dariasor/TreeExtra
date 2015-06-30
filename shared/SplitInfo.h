//SplitInfo.h: SplitInfo structure interface
//
// (c) Daria Sorokina

#pragma once
#include "definitions.h"
#include "INDdata.h"

//data structure describing how the node is split into two subnodes
struct SplitInfo
{
public:
	//default constructor
	SplitInfo();

	//init constructor
	SplitInfo(int attr, double point, double miss = 0.5);
	
	//returns left branch coefficient given the value of the split attribute
	double leftCoef(double value);

public:
	int divAttr;		//split attribute id
	double missingL;	//proportion of missing values going to the left
	double border;		//split point
};

typedef vector<SplitInfo> SplitInfov;
