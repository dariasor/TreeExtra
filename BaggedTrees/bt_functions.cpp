// Bagged Trees / bt_functions.cpp: definitions of global functions for Bagged Trees
// 
// (c) Daria Sorokina

#include "bt_functions.h"

#include "functions.h" // XW

//comparison by the second element
bool idGreater(idpair id1, idpair id2)
{
	return id1.second > id2.second;
}

// XW
string getModelFName(string modelFName, int bagNo)
{
	string _modelFName = string("./BTTemp/") 
			+ insertSuffix(modelFName, "b." + itoa(bagNo, 10));
	return _modelFName;
}
