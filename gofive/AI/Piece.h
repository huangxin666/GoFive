#ifndef PIECE_H
#define PIECE_H

#include <stdint.h>
class Piece
{
public:
	Piece();
	~Piece();
	void setState(int uState);
	void setHot(bool isHot);
	int getState();
	bool isHot();
	void setThreat(int score,int side);// 0Ϊ���� 1Ϊ����
	int getThreat(int side);// 0Ϊ���� 1Ϊ����
	void clearThreat();
private:	
	int  uState;	   //����״̬��0��ʾ���ӣ�1��ʾ�ڣ�-1��ʾ��	
	int threat[2];		//��в���� 0Ϊ���� 1Ϊ����	
	bool hot;			//�Ƿ�Ӧ������
};

#endif