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
	if (param.multithread)
	{
		TreeNode root(cb, param.calculateStepCount, 1);
		root.setBan(param.ban);
		root.setPlayerColor(cb.lastStep.getColor());
		result = root.searchBest2();
	}
	else
	{
		TreeNode root(cb, 4, 1);
		root.setBan(param.ban);
		root.setPlayerColor(cb.lastStep.getColor());
		result = root.searchBest2();
	}
	return result;
}

