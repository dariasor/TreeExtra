// Additive Groves / ag_functions.h: declarations of Additive Groves global functions
// (c) Daria Sorokina

#include "definitions.h"
#include "TrainInfo.h"
#include "INDdata.h"

//saves a vector into a binary file
fstream& operator << (fstream& fbin, doublev& vec);

//saves a vector of vectors into a binary file
fstream& operator << (fstream& fbin, doublevv& mx);

//saves a vector of vectors of vectors into a binary file
fstream& operator << (fstream& fbin, doublevvv& trivec);

//reads a vector from a binary file
fstream& operator >> (fstream& fbin, doublev& vec);

//reads a vector of vectors from a binary file
fstream& operator >> (fstream& fbin, doublevv& mx);

//reads a vector of vectors of vectors from a binary file
fstream& operator >> (fstream& fbin, doublevvv& trivec);

//generates output files for train and expand commands
void trainOut(TrainInfo& ti, doublevv& dir, doublevvv& rmsV, doublevvv& surfaceV, doublevvv& predsumsV, 
			  double trainN, doublevv& dirStat, double validStD = -1.0, int startAlphaNo = 0, int startTiGNNo = 0);

//converts the number of a valid alpha value into the actual value
double alphaVal(int alphaNo);

//converts the number of a valid TiG value into the actual value
int tigVal(int tigNNo);

//rounds tigN down to the closest appropriate value
int adjustTiGN(int tigN);

//converts min alpha value into the number of alpha values
int getAlphaN(double minAlphaVal, double trainV);

//converts max tigN value into the number of tigN values
int getTiGNN(int tigN);

//converts number to string
std::string itoa(int value, int base);

//trains and saves a Layered Groves ensemble (Additive Groves trained in layered style)
double layeredGroves(INDdata& data, TrainInfo& ti, string modelFName);

//runs Layered Groves repeatN times, returns average performance and standard deviation, saves the last model 
double meanLG(INDdata& db, TrainInfo ti, int repeatN, double& resStd, string modelFName);

//implementation for erase for reverse iterator
void rerase(intv& vec, intv::reverse_iterator& iter);

//calculate and output effect of an attribute in a model
void outEffects(INDdata& data, intv attrIds, int quantN, string modelFName, string suffix = "");

//calculate and output joint effects for pairs of attributes in a model
void outIPlots(INDdata& data, iipairv interactions, int quantN1, int quantN2, string modelFName, 
			   string suffix="", string fixedFName="" 
			   /*last two parameters are valid only for a list consisting of a single interaction*/);

//calculate the best place on the performance grid for the interaction detection
bool bestForID(doublevvv& surfaceV, bool rms, int& bestTiGNNo, int& bestAlphaNo);

