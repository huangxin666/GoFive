#ifndef PIECE_H
#define PIECE_H

#include <stdint.h>
class Piece
{
public:
    Piece();
    ~Piece();
    inline void setState(int uState) {
        this->uState = uState;
    }
    inline void setHot(bool isHot) {
        this->hot = isHot;
    }
    inline int getState() {
        return uState;
    }
    inline bool isHot() {
        return hot;
    }
    void setThreat(int score, int side);// 0Ϊ���� 1Ϊ����
    int getThreat(int side);// 0Ϊ���� 1Ϊ����
    void clearThreat();
private:
    int threat[2];		//��в���� 0Ϊ���� 1Ϊ����	
    int8_t uState;	    //����״̬��0��ʾ���ӣ�1��ʾ�ڣ�-1��ʾ��	
    bool hot;			//�Ƿ�Ӧ������
};

#endif