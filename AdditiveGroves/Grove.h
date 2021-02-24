// Additive Groves / Grove.h: interface of class Grove
//
// (c) Daria Sorokina

#pragma once
#include "TreeNode.h"

#ifndef _WIN32
#include "thread_pool.h"
#endif

//Grove model: additive ensemble of several trees
class CGrove
{
public:
	//set function for static data pointer
	static void setData(INDdata& data){pData = &data;}

#ifndef _WIN32
	static void setPool(TThreadPool& pool){pPool = &pool;}
#endif

	//constructor
	CGrove(double alpha, int tigN);

	//constructor
	CGrove(double alpha, int tigN, intv& interaction);

	//rebuilds grove until convergence with predictions of other grove as starting point
	ddpair converge(doublevv& sinpreds, doublev& jointpreds);

	//trains the grove using "layered" version of the algorithm (fixed #trees, increase alpha on every step)
	void trainLayered();

	//saves the grove into the binary file
	void save(const char* fileName);

	//loads the grove from the binary file
	void load(fstream& fload);

	//calculates prediction of the whole grove for a single item
	double predict(int itemNo, DATA_SET dset);

	//returns predictions of single trees and the whole model for all data points in the train set
	void batchPredict(doublevv& sinpreds, doublev& jointpreds);

	//outputs code for a tree in a grove
	void treeCode(int treeNo, fstream& fcode);

private:
	//trains a single tree as part of training a grove
	void genTreeInGrove(doublev& sinpredsx, doublev& jointpreds, int treeNo);

	//grows a tree 
	void growTree(CTreeNode& root);

	//trains several restricted trees, chooses the best
	void chooseTree(CTreeNode& root, doublev& othpreds);

	//calculates prediction of a single tree for a single item
	double localPredict(CTreeNode& root, int itemNo, DATA_SET dset);

private:
	static INDdata* pData;	//data access pointer

#ifndef _WIN32
	static TThreadPool* pPool;	//thread pool pointer
	TCondition nodesCond;	//condition, used for multithreading control 
#endif

	CTreeNodev roots;		//roots of trees in the grove
	double alpha;			//one of two key parameters: controls size of tree
	int tigN;				//one of two key parameters: number of trees in the grove

	intv interaction;	//a higher-order interaction between all these attributes 
							//should not be allowed in the model (model is restricted on interaction)

};


#ifndef _WIN32 
//Information required for a single node splitting job to run. Used for multithreading
struct JobData
{	
	JobData(nodeip in_curNH, nodehstack* in_pNodes, TCondition* in_pNodesCond, int* in_pToDoN, 
			double in_b, double in_H):
	curNH(in_curNH), pNodes(in_pNodes), pNodesCond(in_pNodesCond), pToDoN(in_pToDoN), b(in_b), H(in_H){}

	nodeip curNH; 
	nodehstack* pNodes;
	TCondition* pNodesCond;
	int* pToDoN;
	double b;
	double H;
};
#endif
