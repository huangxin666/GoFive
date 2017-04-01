#include "utils.h"


ChessModeData chessMode[TRIE_COUNT] = {
    {"o?ooo?o",  12000, 12000,                     4},
    {"oo?oo?oo", 12000, 12000,                     4},
    {"ooo?o?ooo",12000, 12000,                     4},
    {"o?oooo?",  12000, 12000,                     5},
    {"?oooo?o",  12000, 12000,                     4},
    {"o?oooox",  1211,  1000,                      5},
    {"xoooo?o",  1211,  1000,                      4},
    {"oo?ooo??", 1300,  1030,                      5},
    {"??ooo?oo", 1300,  1030,                      4},
    {"ooo?oo",   1210,  999,                       5},
    {"oo?ooo",   1210,  999,                       5},
    {"oooooo",   SCORE_5_CONTINUE,SCORE_5_CONTINUE,5},
    {"ooooo",    SCORE_5_CONTINUE,SCORE_5_CONTINUE,4},
    {"?oooo?",   12000, 12000,                     4},
    {"?oooox",   1211,  1000,                      4},
    {"xoooo?",   1211,  1000,                      4},
    {"o?ooo??",  1300,  1030,                      4},
    {"??ooo?o",  1300,  1030,                      4},
    {"ooo?o",    1210,  999,                       4},
    {"o?ooo",    1210,  999,                       4},
    {"oo?oo",    1210,  999,                       4},
    {"?ooo??",   1100,  1200,                      3},
    {"??ooo?",   1100,  1200,                      4},
    {"?o?oo?",   1080,  100,                       5},
    {"?oo?o?",   1080,  100,                       5},
    {"?ooo?",    20,    20,                        3},
    {"??ooox",   20,    20,                        4},
    {"xooo??",   20,    20,                        3},
    {"?o?oox",   5,     5,                         4},
    {"xoo?o?",   5,     5,                         4},
    {"?oo?ox",   10,    10,                        4},
    {"xo?oo?",   10,    10,                        4},
    {"?oo?",     35,    10,                        2},
    {"?o?o?",    30,    5,                         3},
};


int fastfind(int f[], const string &p, int size_o, char o[], int range)
{
    int size_p = p.length();
    int i = SEARCH_LENGTH - range, j = 0;
    size_o = size_o - SEARCH_LENGTH + range;
    while (i < size_o) {
        if (o[i] == p[j]) {
            i++; j++;
        }
        else {
            if (j == 0)
                i++;
            else
                j = f[j - 1] + 1;
        }
        if (j == size_p) {
            /*sum++; j = f[j - 1] + 1;*/
            return 1;
        }
    }
    return 0;
}

void insert(SortInfo e, SortInfo * a, int left, int right)
{
    while (right >= left&&e.value < a[right].value)
    {
        a[right + 1] = a[right];
        right--;
    }
    a[right + 1] = e;
}

void insertionsort(SortInfo * a, int left, int right)
{
    for (int i = left + 1; i <= right; i++)
    {
        insert(a[i], a, left, i - 1);
    }
}

void quicksort(SortInfo * a, int left, int right)
{
    if (left < right)
    {
        int l = left, r = right + 1;
        while (l < r)
        {
            do l++; while (a[l].value < a[left].value&&l < right);
            do r--; while (a[r].value > a[left].value);
            if (l < r) interchange(a, l, r);
        }
        interchange(a, left, r);
        if (r - 1 - left > 9)
            quicksort(a, left, r - 1);
        //个数小于9就不管了		
        if (right - r - 1 > 9)
            quicksort(a, r + 1, right);
    }
}

