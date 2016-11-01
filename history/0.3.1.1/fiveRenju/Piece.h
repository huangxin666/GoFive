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
	void setFocus(bool isFocus);
	void setFlag(bool f);
	void setHot(bool isHot);

	UINT getRow();
	UINT getCol();
	int getState();
	bool isFocus();
	bool isFlag();
	bool isHot();
private:
	UINT uRow;         //���ڶ�ά�������
	UINT uCol;         //���ڶ�λ�������
	int  uState;	   //����״̬��0��ʾ���ӣ�1��ʾ�ڣ�-1��ʾ��
	bool focus;	   //�Ƿ���ʾ����
	bool flag;		//�Ƿ񱻱��
	bool hot;			//�Ƿ�Ӧ������
};

