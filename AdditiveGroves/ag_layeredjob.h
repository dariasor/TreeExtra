//ag_layeredjob.h: 
//
// (c) Xiaojie Wang

#include "TrainInfo.h"
#include "INDdata.h"
#include "Grove.h"

#include "functions.h"

#include <fstream>
#include <cmath>

#ifndef _WIN32
#include "thread_pool.h"

struct LayeredArg;
void doLayered(LayeredArg* ptr);
class LayeredJob;
#endif

//trains and saves a Layered Groves ensemble (Additive Groves trained in layered style)
double layeredGroves(
		INDdata& data, 
		TrainInfo& ti, 
		string modelFName, 
		TThreadPool& pool
		); // XW

//runs Layered Groves repeatN times, returns average performance and standard deviation, saves the last model 
double meanLG(
		INDdata& db, 
		TrainInfo ti, 
		int repeatN, 
		double& resStd, 
		string modelFName,
		TThreadPool& pool
		); // XW
