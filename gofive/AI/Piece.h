#pragma once
#include <stdint.h>
class Piece
{
public:
	Piece();
	~Piece();
	void setRow(uint8_t uRow);
	void setCol(uint8_t uCol);
	void setXY(uint8_t uRow, uint8_t uCol);
	void setState(int uState);
	void setHot(bool isHot);
	uint8_t getRow();
	uint8_t getCol();
	int getState();
	bool isHot();
	void setThreat(int score,int side);// 0Ϊ���� 1Ϊ����
	int getThreat(int side);// 0Ϊ���� 1Ϊ����
	void clearThreat();
private:	
	int  uState;	   //����״̬��0��ʾ���ӣ�1��ʾ�ڣ�-1��ʾ��	
	int threat[2];		//��в���� 0Ϊ���� 1Ϊ����	
	uint8_t uRow;         //���ڶ�ά�������
	uint8_t uCol;         //���ڶ�λ�������
	bool hot;			//�Ƿ�Ӧ������
};

