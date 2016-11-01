#pragma once
#include "defines.h"
class Piece
{
public:
	Piece();
	~Piece();
	void setRow(UINT uRow);
	void setCol(UINT uCol);
	void setXY(UINT uRow, UINT uCol);
	void setState(int uState);
	void setHot(bool isHot);
	UINT getRow();
	UINT getCol();
	int getState();
	bool isHot();
	void save(CArchive &oar);
	void load(CArchive &oar);
	void setThreat(int score,int side);// 0Ϊ���� 1Ϊ����
	int getThreat(int side);// 0Ϊ���� 1Ϊ����
	void clearThreat();
private:
	UINT uRow;         //���ڶ�ά�������
	UINT uCol;         //���ڶ�λ�������
	int  uState;	   //����״̬��0��ʾ���ӣ�1��ʾ�ڣ�-1��ʾ��
	bool hot;			//�Ƿ�Ӧ������
	int threat[2];		//��в���� 0Ϊ���� 1Ϊ����
};

