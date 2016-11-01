#pragma once
//������ض���
#define BROARD_X	532	//����X�����
#define BROARD_Y	532	//����Y�����
#define FRAME_X
#define FRAME_Y		62	//����Y�����
#define CHESS_X		36	//����X�����
#define CHESS_Y		36	//����Y�����
#define BLANK		30
//�����巽�鶨��
//State
#define STATE_EMPTY			0
#define STATE_CHESS_BLACK	1
#define STATE_CHESS_WHITE	-1

#define BELONGTOAI			0
#define BELONGTOMAN			1

#define GAME_STATE_WHITEWIN	2
#define GAME_STATE_BLACKWIN	1
#define GAME_STATE_RUN		0
#define GAME_STATE_DRAW     3 //ƽ��
#define GAME_STATE_BLACKBAN 4 //���ӽ��ָ渺
//���̴�С
#define BOARD_ROW_MAX 15
#define BOARD_COL_MAX 15
#define UPDATETHREAT_SEARCH_MAX 5

//����������ӽڵ�
#define GAMETREE_CHILD_MAX 225
#define GAMETREE_CHILD_SEARCH 10//��ʼ�������������

struct STEP
{
public:      
	UINT uRow;
	UINT uCol;	
	UINT step;//����,��ǰstep	
	STEP(UINT s, UINT r, UINT c){
		step = s;
		uRow = r;
		uCol = c;
	};
	STEP(){};
};	// �����岽��stepList

struct AISTEP
{
	int score;          //����
	UINT x;
	UINT y;				//��ǰstep	
	AISTEP(UINT i, UINT j, int s){
		score = s; x = i; y = j;
	};
	AISTEP(){ score = 0; x = 0; y = 0; };
};// ������ṹ��

struct  THREATINFO
{
	int totalScore;     //����
	int HighestScore;	//��߷�
	THREATINFO(int total, int Highest){
		totalScore = total; HighestScore = Highest;
	};
	THREATINFO(){};
};// ������ṹ��


//����
#define STR_COUNT				14//����
#define STR_SIX_CONTINUE		0	//oooooo ���֣��ǽ��ֵ�ͬ��1
#define STR_FIVE_CONTINUE		1	//ooooo ����
#define STR_FOUR_CONTINUE		2	//?oooo? ���� ���Լ���1������
#define STR_FOUR_CONTINUE_DEAD	3	//?oooox ���ȼ�max��һ�Ŷ��� �֣��ǶԷ��ģ����ȼ�max���Լ��ģ����ȼ����Ի�һ��
#define STR_FOUR_BLANK			4	//o?ooo?? ���ȼ�max������ͳ�11
#define STR_FOUR_BLANK_DEAD		5	//ooo?o ���ȼ�max��һ�Ŷ���
#define STR_FOUR_BLANK_M		6	//oo?oo ���ȼ�max��һ�Ŷ���
#define STR_THREE_CONTINUE		7	//?ooo? ���ȼ�max-1�����³�1������ͳ�2,3,4,5,6 �Լ���2,3,4,5,6������
#define STR_THREE_BLANK			8	//?o?oo?
#define STR_THREE_CONTINUE_DEAD	9	//??ooox
#define STR_THREE_BLANK_DEAD1	10	//?o?oox
#define STR_THREE_BLANK_DEAD2	11	//?oo?ox
#define STR_TOW_CONTINUE		12	//?oo?
#define STR_TOW_BLANK			13	//?o?o?
