//ItemInfo.h: ItemInfo structure
//
// (c) Daria Sorokina

#pragma once
#include "definitions.h"

//Information about a case in a tree node trainset subset or prediction of the leaf
struct ItemInfo
{
	ItemInfo(){key=0;coef=1;response=0;}

	int key;			//case id
	double coef;		//case belongs to the node with coefficient coef. 0<coef<=1
	double response;	//either true response for the train set case or leaf prediction
};

typedef vector<ItemInfo> ItemInfov; 
