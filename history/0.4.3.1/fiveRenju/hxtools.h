#pragma once
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
			//个数小于9就不管了		
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