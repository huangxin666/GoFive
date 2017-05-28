#include "utils.h"


//int fastfind(int f[], const string &p, int size_o, char o[], int range)
//{
//    int size_p = p.length();
//    int i = SEARCH_LENGTH - range, j = 0;
//    size_o = size_o - SEARCH_LENGTH + range;
//    while (i < size_o) {
//        if (o[i] == p[j]) {
//            i++; j++;
//        }
//        else {
//            if (j == 0)
//                i++;
//            else
//                j = f[j - 1] + 1;
//        }
//        if (j == size_p) {
//            /*sum++; j = f[j - 1] + 1;*/
//            return 1;
//        }
//    }
//    return 0;
//}
//
//void insert(SortInfo e, SortInfo * a, int left, int right)
//{
//    while (right >= left&&e.value < a[right].value)
//    {
//        a[right + 1] = a[right];
//        right--;
//    }
//    a[right + 1] = e;
//}
//
//void insertionsort(SortInfo * a, int left, int right)
//{
//    for (int i = left + 1; i <= right; i++)
//    {
//        insert(a[i], a, left, i - 1);
//    }
//}
//
//void quicksort(SortInfo * a, int left, int right)
//{
//    if (left < right)
//    {
//        int l = left, r = right + 1;
//        while (l < r)
//        {
//            do l++; while (a[l].value < a[left].value&&l < right);
//            do r--; while (a[r].value > a[left].value);
//            if (l < r) interchange(a, l, r);
//        }
//        interchange(a, left, r);
//        if (r - 1 - left > 9)
//            quicksort(a, left, r - 1);
//        //个数小于9就不管了		
//        if (right - r - 1 > 9)
//            quicksort(a, r + 1, right);
//    }
//}

