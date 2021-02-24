// Additive Groves / ag_layered.h: declarations of functions for training layered Additive Groves models

#pragma once

#include "TrainInfo.h"
#include "INDdata.h"

//trains and saves a Layered Groves ensemble (Additive Groves trained in layered style)
double layeredGroves(INDdata& data, TrainInfo& ti, string modelFName);

//runs Layered Groves repeatN times, returns average performance and standard deviation
//saves the model from the last run
double meanLG(INDdata& data, TrainInfo ti, int repeatN, double& resStd, string modelFName);
