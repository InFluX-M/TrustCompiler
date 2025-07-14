#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>



void main() {
	int x = 10;
	int y = 5;
	bool b = false;
	if (x == 10) {
		y = 6;
		x = 7;
	}
	if (y <= 2 && x > 7 || b != false) {
		x = 4 + y;
	} else {
		x = 3 * y;
	}
	if (x >= 5 && y < 7 && !(b)) {
		b = true;
	} else if (x == 3) {
		x = 100;
	} else {
		b = false;
	}
}

