#pragma once
//����ͷ�ļ�
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
using namespace std;

//������ض���
#define BROARD_X	532	//����X�����
#define BROARD_Y	532	//����Y�����
#define FRAME_X
#define FRAME_Y		62	//����Y�����
#define CHESS_X		36	//����X�����
#define CHESS_Y		36	//����Y�����
#define BLANK		30


//��Ϸ״̬
#define GAME_STATE_WHITEWIN	2
#define GAME_STATE_BLACKWIN	1
#define GAME_STATE_RUN		0
#define GAME_STATE_DRAW     3 //ƽ��
#define GAME_STATE_BLACKBAN 4 //���ӽ��ָ渺
#define GAME_STATE_WAIT		5 //�ȴ�AI����

#define DEFAULT_DPI 96

struct CursorPosition
{
	int row;
	int col;
	bool enable;
};

inline bool operator==(const CursorPosition &a, const CursorPosition &b) 
{
	if (a.col != b.col) return false;
	if (a.row != b.row) return false;
	if (a.enable != b.enable) return false;
	return true;
}