#include "AIGameTree.h"
#include "TreeNode.h"

AIGameTree::AIGameTree()
{
}


AIGameTree::~AIGameTree()
{
}

Position AIGameTree::getNextStep(ChessBoard cb, AIParam param)
{
	Position result;
	cb.setGlobalThreat(param.ban);

	TreeNode root(cb, param.caculateSteps, 1);
	root.setBan(param.ban);
	root.setPlayerColor(cb.lastStep.getColor());
	result = root.searchBest();

	
	return result;
}

