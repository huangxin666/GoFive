#pragma once
#include <stdint.h>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
using namespace std;

//�����巽�鶨��
//State
#define STATE_EMPTY			0
#define STATE_CHESS_BLACK	1
#define STATE_CHESS_WHITE	-1

#define BELONGTOAI			0
#define BELONGTOMAN			1

//���̴�С
#define BOARD_ROW_MAX 15
#define BOARD_COL_MAX 15

//����������ӽڵ�
#define GAMETREE_CHILD_MAX 225
#define GAMETREE_CHILD_SEARCH 10//��ʼ�������������
#define UPDATETHREAT_SEARCH_MAX 4
//���߳�
#define MultipleThread_MAXIMUM 128 //ͬʱ����߳���

struct STEP
{
public:
	uint8_t uRow;
	uint8_t uCol;
	uint8_t step;//����,��ǰstep
	bool isBlack;
	STEP(uint8_t s, uint8_t r, uint8_t c, bool b) {
		step = s;
		uRow = r;
		uCol = c;
		isBlack = b;
	};
	STEP() {};
	int getColor()
	{
		return isBlack ? 1 : -1;
	}
	void setColor(int color)
	{
		isBlack = (color == 1) ? true : false;
	}
};	// �����岽��stepList

struct AISTEP
{
	int score;          //����
	uint8_t x;
	uint8_t y;				//��ǰstep	
	AISTEP(uint8_t i, uint8_t j, int s) {
		score = s; x = i; y = j;
	};
	AISTEP() { score = 0; x = 0; y = 0; };
};// ������ṹ��

struct  THREATINFO
{
	int totalScore;     //����
	int HighestScore;	//��߷�
	THREATINFO(int total, int Highest)
	{
		totalScore = total; HighestScore = Highest;
	};
	THREATINFO() { totalScore = 0; HighestScore = 0; };
};// ������ṹ��

struct Position
{
	int row, col;
	Position(int r, int c) :row(r), col(c) {};
	Position() {};
};

//����(4��)
#define DIRECTION4_ROW 0
#define DIRECTION4_COL 1
#define DIRECTION4_RC1 2  //������б��
#define DIRECTION4_RC2 3	 //������б��
//����(8��)
#define DIRECTION8_L	 0 //��
#define DIRECTION8_R	 1 //��
#define DIRECTION8_U     2 //��
#define DIRECTION8_D     3 //��
#define DIRECTION8_LU    4 //�I
#define DIRECTION8_RD    5 //�K
#define DIRECTION8_LD    6 //�L
#define DIRECTION8_RU    7 //�J

//����
struct STR_INFO
{
	int continues, stops, blanks;
	STR_INFO()
	{
		stops = 0;
		continues = 0;
		blanks = 0;
	}
};
#define STR_MAX_LENGTH			6
#define STR_COUNT				15	//����
#define STR_SIX_CONTINUE		0	//oooooo ���֣��ǽ��ֵ�ͬ��1
#define STR_FIVE_CONTINUE		1	//ooooo ����
#define STR_FOUR_CONTINUE		2	//?oooo? ���� ���Լ���1������
#define STR_FOUR_CONTINUE_DEAD	3	//?oooox ���ȼ�max��һ�Ŷ��� �֣��ǶԷ��ģ����ȼ�max���Լ��ģ����ȼ����Ի�һ��
#define STR_FOUR_BLANK			4	//o?ooo?? ���ȼ�max������ͳ�11
#define STR_FOUR_BLANK_DEAD		5	//ooo?o ���ȼ�max��һ�Ŷ���
#define STR_FOUR_BLANK_M		6	//oo?oo ���ȼ�max��һ�Ŷ���
#define STR_THREE_CONTINUE		7	//?ooo?? ����
#define STR_THREE_BLANK			8	//?o?oo?
#define STR_THREE_CONTINUE_F	9	//?ooo? �ٻ���
#define STR_THREE_CONTINUE_DEAD	10	//??ooox
#define STR_THREE_BLANK_DEAD1	11	//?o?oox
#define STR_THREE_BLANK_DEAD2	12	//?oo?ox
#define STR_TOW_CONTINUE		13	//?oo?
#define STR_TOW_BLANK			14	//?o?o?



struct CHILDINFO
{
	int value;
	int key;
	CHILDINFO(){};
	CHILDINFO(int k, int v){ key = k; value = v; };
};
class hxtools
{
public:
	static void insert(CHILDINFO e, CHILDINFO * a, int left, int right)
	{
		while (right >= left&&e.value<a[right].value)
		{
			a[right + 1] = a[right];
			right--;
		}
		a[right + 1] = e;
	}

	static void insertionsort(CHILDINFO * a, int left, int right)
	{
		for (int i = left + 1; i <= right; i++)
		{
			insert(a[i], a, left, i - 1);
		}
	}

	static void interchange(CHILDINFO *list, int a, int b)
	{
		CHILDINFO temp = list[a];
		list[a] = list[b];
		list[b] = temp;
	}

	static void quicksort(CHILDINFO * a, int left, int right)
	{
		if (left < right)
		{
			int l = left , r = right+1;
			while (l<r)
			{
				do l++; while (a[l].value < a[left].value&&l < right);
				do r--; while (a[r].value > a[left].value);
				if (l < r) interchange(a, l, r);
			}
			interchange(a, left, r);
			if (r - 1 - left>9)
				quicksort(a, left, r - 1);
			//����С��9�Ͳ�����		
			if (right - r - 1>9)
				quicksort(a, r + 1, right);
		}
	}

	static void sort(CHILDINFO * a, int left, int right)
	{
		quicksort(a,left,right);
		insertionsort(a, left, right);
	}
};