#pragma once
class Piece
{
public:
	UINT uRow;         //���ڶ�ά�������
	UINT uCol;         //���ڶ�λ�������
	int  uState;	   //����״̬��0��ʾ���ӣ�1��ʾ�ڣ�-1��ʾ��
	bool isFocus;	   //�Ƿ���ʾ����
	bool isFlag;		//�Ƿ񱻱��
	bool isHot;			//�Ƿ�Ӧ������
	Piece();
	~Piece();
	void setFlag(bool f);
};

