#pragma once
typedef struct 
{
	UINT uRow;         //���ڶ�ά�������
	UINT uCol;         //���ڶ�λ�������
	int  uState;	   //����״̬��0��ʾ���ӣ�1��ʾ�ڣ�-1��ʾ��
	bool isFocus;	   //�Ƿ���ʾ����
	bool isFlag;		//�Ƿ񱻱��
} FIVEWND;	// ������ṹ��

class FiveBoard
{
public:
	FiveBoard(void);
	~FiveBoard(void);
	FIVEWND m_pFive[15][15];
};

