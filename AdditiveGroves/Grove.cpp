// Additive Groves / Grove.cpp: implementation of class Grove
//
// (c) Daria Sorokina

#include "Grove.h"
#include "functions.h"
#include "ag_functions.h"

#include <math.h>
#include <algorithm>
#include <fstream>
#include <iostream>

INDdata* CGrove::pData;
#ifndef _WIN32
TThreadPool* CGrove::pPool;

//job class, splits a node, used for multithreading in unix
class CNodeSplitJob : public TThreadPool::TJob
{
public:
    
    CNodeSplitJob() : TThreadPool::TJob() { }
    
    void Run(void* ptr)
    {
		JobData* pJD = (JobData*) ptr;
		nodeip curNH = pJD->curNH;
		TCondition& nodesCond = *pJD->pNodesCond;

		double h = curNH.second;
		double curAlpha = pow(2, - ( pJD->b +  pJD->H) * h /  pJD->H +  pJD->b);
		bool notLeaf = curNH.first->split(curAlpha);
		
		nodesCond.Lock();
		if(notLeaf)
		{
			pJD->pNodes->push(nodeip(curNH.first->left, curNH.second + 1));
			pJD->pNodes->push(nodeip(curNH.first->right, curNH.second + 1));
			*pJD->pToDoN += 1;
		}
		else
			*pJD->pToDoN -= 1;
		
		nodesCond.Unlock();
		nodesCond.Signal();

		delete pJD;
    }
};
#endif

CGrove::CGrove(double alphaIn, int tigNIn): alpha(alphaIn), tigN(tigNIn), roots(tigNIn)
{
}

CGrove::CGrove(double alphaIn, int tigNIn, intv& interactionIn): 
	alpha(alphaIn), tigN(tigNIn), roots(tigNIn), interaction(interactionIn)
{
}

//Builds the (tigN, alpha) grove with previous grove (represented by sinpreds and jointpreds) 
//as a starting point. Keeps regrowing trees until convergence. Returns rmse on on bag data and on oob data.
//in:	sinpreds - predictions of single trees on the training set
//		jointpreds - predictions of the whole grove on the training set
ddpair CGrove::converge(doublevv& sinpreds, doublev& jointpreds)
{

	//get out of bag data
	intv outofbag;	//indexes
	doublev oobtar, oobwt;	//targets, weights
	int oobN = pData->getOutOfBag(outofbag, oobtar, oobwt);
			
	//get current bag of data
	intv bag;		//indexes
	doublev bagtar, bagwt; //targets, weights
	int itemN = pData->getCurBag(bag, bagtar, bagwt);

	//calculate previous oob rmse - rmse of the original grove on current oob data
	doublev oobpreds(oobN);
	for(int oobNo = 0; oobNo < oobN; oobNo++)
		oobpreds[oobNo] = jointpreds[outofbag[oobNo]];
	double oobPrevRMS = rmse(oobpreds, oobtar, oobwt);	

	//calculate previous bag rmse - rmse of the original grove on current bag data
	doublev bagpreds(itemN);
	for(int itemNo = 0; itemNo < itemN; itemNo++)
		bagpreds[itemNo] = jointpreds[bag[itemNo]];
	double bagPrevRMS = rmse(bagpreds, bagtar, bagwt);	

	//rebuild the trees in turn until the changes will be unsignificant
	double oobRMS;	//rmse on oob data
	double bagRMS;	//rmse on bag data
	while(true)
	{
		//rebuild one tree
		for(int treeNo = 0; treeNo < tigN; treeNo++)
		{
			int curTreeNo = (treeNo + tigN - 1) % tigN; //begin from the last (often new) tree
			genTreeInGrove(sinpreds[curTreeNo], jointpreds, curTreeNo);
		}

		//calculate rmse for out of bag data
		for(int oobNo = 0; oobNo < oobN; oobNo++)
			oobpreds[oobNo] = jointpreds[outofbag[oobNo]];
		oobRMS = rmse(oobpreds, oobtar, oobwt);	

		//calculate rmse for bag data
		for(int itemNo = 0; itemNo < itemN; itemNo++)
			bagpreds[itemNo] = jointpreds[bag[itemNo]];
		bagRMS = rmse(bagpreds, bagtar, bagwt);

		//check the endofloop condition - negative or small changes in rmse
		//first check in bag results, then (if bag preds are ideal) check results on oob data
		bool bag_converged = (bagPrevRMS - bagRMS) / bagPrevRMS <= 0.002;
		bool oob_converged = (oobPrevRMS - oobRMS) / oobPrevRMS <= 0.002; 

		if((bagRMS != 0) && bag_converged ||
			(bagRMS == 0) && ((oobRMS == 0) || oob_converged))
			return ddpair(bagRMS, oobRMS);
		else
		{
			bagPrevRMS = bagRMS;
			oobPrevRMS = oobRMS;
		}
	}
}

//trains the grove using "layered" version of the algorithm (fixed #trees, increase alpha on every step)
void CGrove::trainLayered()
{
	double minAlpha = alpha;		//alpha parameter of the final grove
	int itemN = pData->getTrainN();	//number of data points in the train set
	int alphaN = getAlphaN(minAlpha, itemN);	//number of different alpha values to use

	doublev jointpreds(itemN, 0);	//prediction of the whole grove on the train set data points 
	doublevv sinpreds(tigN, doublev(itemN, 0));	//predictions by tree 

	for(int alphaNo = 0; alphaNo < alphaN; alphaNo++)
	{
		if(alphaNo < alphaN - 1)
			alpha = alphaVal(alphaNo);
		else	//this is a special case because minAlpha can be zero
			alpha = minAlpha;

		pData->newBag();
		converge(sinpreds, jointpreds);
	}
}

//Generates a tree as part of training the whole additive grove 
//in-out: sinpredsx - predictions of this tree 
//in-out: jointpreds - predictions of the whole grove (sum of trees)
void CGrove::genTreeInGrove(doublev& sinpredsx, doublev& jointpreds, int treeNo)
{
	int itemN = pData->getTrainN(); 
	doublev othpreds(itemN); //prediction of all other trees
	
	//calculate joint prediction of other trees
	for(int itemNo = 0; itemNo < itemN; itemNo++)
		othpreds[itemNo] = jointpreds[itemNo] - sinpredsx[itemNo];
	
	//initialize the root
	roots[treeNo].setRoot();
	roots[treeNo].resetRoot(othpreds);

	//build tree
	if(interaction.size() == 0)
		growTree(roots[treeNo]);
	else
		chooseTree(roots[treeNo], othpreds);

	//recalculate single predictions of this tree and joint predictions of the grove
	for(int itemNo = 0; itemNo < itemN; itemNo++)
	{
		sinpredsx[itemNo] = localPredict(roots[treeNo], itemNo, TRAIN);
		jointpreds[itemNo] = othpreds[itemNo] + sinpredsx[itemNo];
	}
}

//Grows the tree. Assumes that the root is all set (contains train set, attrs, etc.).
void CGrove::growTree(CTreeNode& root)
{	
	double nzAlpha = (alpha == 0) ? (1.0 / pData->getTrainN()) : alpha; 
	double b = - log((double) pData->getTrainN()) / log(2.0);
	double H = - log((double) nzAlpha) / log(2.0);

	if(b >= -H)
		b = -H;

	//place root into the stack of nodes for splitting
	nodehstack nodes;	//stack
	nodes.push(nodeip(&root, 0));

#ifdef _WIN32	
	//grow the tree: take nodes from the stack, try to split them, 
	//if the result is positive, place child nodes into the same stack
	while(!nodes.empty())
	{
		nodeip curNH = nodes.top();
		nodes.pop();

		double h = curNH.second;
		double curAlpha = pow(2, - (b + H) * h / H + b);
		bool notLeaf = curNH.first->split(curAlpha);
		if(notLeaf)
		{//process child nodes of this node
			nodes.push(nodeip(curNH.first->left, curNH.second + 1));
			nodes.push(nodeip(curNH.first->right, curNH.second + 1));
		}
	}
#else
	//multithreaded version of the same process
	int toDoN = 1; //how many nodes exists but have not been finished processing at the moment
	while(toDoN > 0)
	{
		nodesCond.Lock();
		while(nodes.empty() && (toDoN > 0))
		    nodesCond.Wait();
		if(toDoN > 0)
		{
			nodeip curNH = nodes.top(); //pointer to the node that will be attempted to split next
			nodes.pop();
			nodesCond.Unlock();

			JobData* pJD = new JobData(curNH, &nodes, &nodesCond, &toDoN, b, H);
			pPool->Run(new CNodeSplitJob(), pJD, true);
		}
		else
			nodesCond.Unlock();
	}
#endif
}

//trains several restricted trees - each missing an attribute from interaction, chooses the best
void CGrove::chooseTree(CTreeNode& root, doublev& othpreds)
{
	//get out of bag data
	intv outofbag;	//indexes
	doublev oobtar, oobwt;	//targets
	int oobN = pData->getOutOfBag(outofbag, oobtar, oobwt);
	doublev sinoobtar(oobN, 0);
	for(int oobNo = 0; oobNo < oobN; oobNo++)
		sinoobtar[oobNo] = oobtar[oobNo] - othpreds[outofbag[oobNo]];
	
	int interN = (int)interaction.size(); //order of interactions, number of variables to test
	doublev rPerfs(interN, 1); //performance of restricted trees

	//build interN restricted trees on the same train set 
	CTreeNodev rRoots(interN, root);
	for(int interNo = 0; interNo < interN; interNo++)
	{
		rRoots[interNo].delAttr(interaction[interNo]);
		growTree(rRoots[interNo]);

		doublev sinoobpreds(oobN, 0);
		for(int oobNo = 0; oobNo < oobN; oobNo++)
			sinoobpreds[oobNo] = localPredict(rRoots[interNo], outofbag[oobNo], TRAIN);
		rPerfs[interNo] = rmse(sinoobpreds, sinoobtar, oobwt);													
	}
	
	//insert the winning tree and its results into grove building process
	int bestNo = min_element(rPerfs.begin(), rPerfs.end()) - rPerfs.begin();
	root = rRoots[bestNo];
	rRoots[bestNo].left = NULL;
	rRoots[bestNo].right = NULL;
}

//Calculates predictions of one of the trees for one item
double CGrove::localPredict(CTreeNode& root, int itemNo, DATA_SET dset)
{
	//Implementation note: because of missing values, the item can end up in several leaves with
	//different coefficients. We trace which nodes it visits and what coefficients it has in each
	//node

	stack<nodecoefp, vector<nodecoefp> > nodes;	//stack of node-coef pairs which this case visits
	vector<nodecoefp> leaves; //list of leaves where this case ends up 
	nodecoefp btnLOut, btnROut, btnIn; //additional variables to work with nodes and coefs
	
	//place the item in the root with coef 1
	nodes.push(nodecoefp(&root, 1));

	//follow the item down the tree
	while(!nodes.empty())
	{
		//pick up the node from the stack
		btnIn = nodes.top();
		nodes.pop();

		if(btnIn.first->isLeaf())
		{
			//reached the leaf, update leaves list
			leaves.push_back(btnIn);
		}
		else
		{//this node is not a leaf. Figure out, which branches the case follows down from here 
		 //and place corresponing child node-coef pairs into the stack  
			btnIn.first->traverse(itemNo, btnIn.second, btnLOut.second, btnROut.second, dset);
			if(btnLOut.second)
			{//key follows left branch
				btnLOut.first = btnIn.first->left;
				nodes.push(btnLOut);
			}
			if(btnROut.second)
			{//key follows right branch
				btnROut.first = btnIn.first->right;
				nodes.push(btnROut);
			}
		}
	}
	
	//calculate aggregated prediction from all leaves
	double ret = 0; 
	int leafN = (int)leaves.size();
	for(int leafNo = 0; leafNo < leafN; leafNo++)
		ret += leaves[leafNo].first->getResp() * leaves[leafNo].second;

	return ret;
}

//Saves the grove into the binary file. 
//Nodes of each tree are packed into the file in preorder. Trees are packed consecutively.
void CGrove::save(const char* fileName)
{
	fstream fsave(fileName, ios_base::binary | ios_base::out | ios_base::app);	//file
	for(int treeNo = 0; treeNo < tigN; treeNo++)
	{
		stack<CTreeNode*> nodes;	//stack for keeping roots of subtrees in the packing order
		nodes.push(&roots[treeNo]);
		while(!nodes.empty())
		{
			CTreeNode* pNode = nodes.top(); //current node to be packed
			nodes.pop();
			pNode->save(fsave);
			if(!pNode->isLeaf())
			{
				nodes.push(pNode->right);
				nodes.push(pNode->left);
			}
		}
	}
	fsave.close();
}

//Loads the grove from the binary file. 
//Nodes of each tree are packed into the file in preorder. Trees are packed consecutively.
void CGrove::load(fstream& fload)
{
	for(int treeNo = 0; treeNo < tigN; treeNo++)
	{
		bool rootLeaf = roots[treeNo].load(fload);
		stack<CTreeNode*> nodes;	//stack for keeping nodes without leaves attached yet
		if(!rootLeaf)
			nodes.push(&roots[treeNo]);
		while(!nodes.empty())
		{
			CTreeNode* pNode = nodes.top(); //current node to be packed
			if(pNode->left == NULL) //next node is the left child of pNode
			{
				pNode->left = new CTreeNode();
				bool leaf = pNode->left->load(fload);
				if(!leaf)
					nodes.push(pNode->left);
			}
			else	//next node is the right child of pNode
			{
				nodes.pop();
				pNode->right = new CTreeNode();				
				bool leaf = pNode->right->load(fload);
				if(!leaf)
					nodes.push(pNode->right);
			}
		}
	}// end 	for(int treeNo = 0; treeNo < tigN; treeNo++)
}

//calculates prediction of the whole grove for a single item (sum of single trees' predictions)
double CGrove::predict(int itemNo, DATA_SET dset)
{
	double prediction = 0;
	for(int treeNo = 0; treeNo < tigN; treeNo++)
		prediction += localPredict(roots[treeNo], itemNo, dset);
	return prediction;
}

//returns predictions of single trees and the whole model for all data points in the train set
//in-out vectors should be already initialized with correct sizes
void CGrove::batchPredict(doublevv& sinpreds, doublev& jointpreds)
{
	int itemN = pData->getTrainN();
	for(int itemNo = 0; itemNo < itemN; itemNo++)
	{
		jointpreds[itemNo] = 0;
		for(int treeNo = 0; treeNo < tigN; treeNo++)
		{
			sinpreds[treeNo][itemNo] = localPredict(roots[treeNo], itemNo, TRAIN);
			jointpreds[itemNo] += sinpreds[treeNo][itemNo];
		}
	}
}

//outputs code for a tree in a grove
void CGrove::treeCode(int treeNo, fstream& fcode)
{
	typedef pair<CTreeNode*, int> nodeLevel; //int corresponds to the level in the tree
	stack<nodeLevel> outLeft;
	stack<nodeLevel> outRight; 

	outLeft.push(nodeLevel(&roots[treeNo], 1));

	CTreeNode* pNode;
	int level;
	while(!outLeft.empty() || !outRight.empty())
		if(!outLeft.empty())
		{
			pNode = outLeft.top().first;
			level = outLeft.top().second;
			outLeft.pop();
			for(int i = 0; i < level; i++)
				fcode << "    ";
			if(!pNode->isLeaf())
			{
				outRight.push(nodeLevel(pNode, level));
				outLeft.push(nodeLevel(pNode->left,level+1));
				//output the left branch
				fcode << "if (d[" << pNode->getDivAttr() << "] <= " << pNode->getThresh() << ")\n";
			}
			else
				fcode << "r += " << pNode->getResp() << ";\n";
		}
		else // !outRight.empty()
		{
			pNode = outRight.top().first;
			level = outRight.top().second;
			outRight.pop();
			outLeft.push(nodeLevel(pNode->right,level+1));

			//output the right branch
			for(int i = 0; i < level; i++)
				fcode << "    ";
			fcode << "else\n";
		}
}
