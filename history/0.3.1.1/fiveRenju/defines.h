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

//���̴�С
#define BOARD_ROW_MAX 15
#define BOARD_COL_MAX 15

//����������ӽڵ�
#define GAMETREE_CHILD_MAX 225
typedef struct tagSTEP
{
public:
	UINT step;          //����
	UINT uCol;
	UINT uRow;		//��ǰstep	
	tagSTEP(UINT s, UINT r, UINT c){
		step = s;
		uRow = r;
		uCol = c;
	}
	tagSTEP(){

	}
}STEP;	// �����岽��stepList

typedef struct
{
	int score;          //����
	UINT x;
	UINT y;				//��ǰstep	
} AISTEP;// ������ṹ��

typedef struct
{
	int totalScore;          //����
	int HighestScore;	//��߷�
} THREATINFO;// ������ṹ��
//����
#define STR_COUNT 20//����
#define STR_FIVE_CONTINUE 0 //ooooo ����
#define STR_FOUR_CONTINUE 1 //?oooo? ���� ���Լ���1������
#define STR_FOUR_CONTINUE_L 2 //?oooox ���ȼ�max��һ�Ŷ��� �֣��ǶԷ��ģ����ȼ�max���Լ��ģ����ȼ����Ի�һ��
#define STR_FOUR_CONTINUE_R 3 //xoooo? ���ȼ�max��һ�Ŷ���
#define STR_FOUR_BLANK_R 4 //??ooo?o ���ȼ�max������ͳ�10
#define STR_FOUR_BLANK_L 5 //o?ooo?? ���ȼ�max������ͳ�11
#define STR_FOUR_BLANK_M 6 //oo?oo ���ȼ�max��һ�Ŷ���
#define STR_THREE_CONTINUE 7 //?ooo? ���ȼ�max-1�����³�1������ͳ�2,3,4,5,6 �Լ���2,3,4,5,6������
#define STR_THREE_BLANK_R 8 //?oo?o?
#define STR_THREE_BLANK_L 9 //?o?oo?
#define STR_THREE_CONTINUE_L 10 //??ooox
#define STR_THREE_CONTINUE_R 11 //xooo??
#define STR_THREE_BLANK_L1 12 //?o?oox
#define STR_THREE_BLANK_L2 13 //xo?oo?
#define STR_THREE_BLANK_R1 14 //?oo?ox
#define STR_THREE_BLANK_R2 15 //xoo?o?
#define STR_TOW_CONTINUE 16 //?oo?
#define STR_TOW_BLANK 17 //?o?o?
#define STR_FOUR_BLANK_R_DEAD 18 //xooo?o ���ȼ�max��һ�Ŷ���
#define STR_FOUR_BLANK_L_DEAD 19 //o?ooox ���ȼ�max��һ�Ŷ���