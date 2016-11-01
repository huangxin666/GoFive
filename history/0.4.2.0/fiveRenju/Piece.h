#pragma once

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
	byte getRow();
	byte getCol();
	int getState();
	bool isHot();
	void setThreat(int score,int side);// 0Ϊ���� 1Ϊ����
	int getThreat(int side);// 0Ϊ���� 1Ϊ����
	void clearThreat();
private:	
	int  uState;	   //����״̬��0��ʾ���ӣ�1��ʾ�ڣ�-1��ʾ��	
	int threat[2];		//��в���� 0Ϊ���� 1Ϊ����
	bool hot;			//�Ƿ�Ӧ������
	byte uRow;         //���ڶ�ά�������
	byte uCol;         //���ڶ�λ�������
};

