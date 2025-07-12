#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>



void main() {
	bool x[200];
	int y = 1;
	const int c = y;
	const int a = y + 42;
	int rr;
	x[a] = true;
}

