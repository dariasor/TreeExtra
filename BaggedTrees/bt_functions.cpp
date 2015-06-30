// Bagged Trees / bt_functions.cpp: definitions of global functions for Bagged Trees
// 
// (c) Daria Sorokina

#include "bt_functions.h"


//comparison by the second element
bool idGreater(idpair id1, idpair id2)
{
	return id1.second > id2.second;
}
