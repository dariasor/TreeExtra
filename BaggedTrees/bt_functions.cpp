// Bagged Trees / bt_functions.cpp: definitions of global functions for Bagged Trees

#include "bt_functions.h"

#include "functions.h"

//comparison by the second element
bool idGreater(idpair id1, idpair id2)
{
	return id1.second > id2.second;
}

string getModelFName(string modelFName, int bagNo)
{
	string _modelFName = string("./BTTemp/") 
			+ insertSuffix(modelFName, "b." + itoa(bagNo, 10));
	return _modelFName;
}
