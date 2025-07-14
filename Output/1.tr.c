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
	const int xx = 10 + 5 * 2 - 3;
	const bool bb = true && false || true;
}

