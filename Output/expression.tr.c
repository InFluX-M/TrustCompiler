#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>



void main() {
	int a = 10;
	const int b = 20;
	const int c = 5;
	const int d = 2;
	const bool x = true;
	const bool y = false;
	const int result_arith = a + b * c - d / c % d;
	const bool result_relational = (a < b) && (b >= c) || (a != d);
	const int result_unary = -a + b;
	if ((x || !y) && (a > b || c <= d)) {
		a = (b + c) * d;
	}
	const int complex_expr = (a + b) * (c - d) / (a % c);
	const bool bool_expr = !((x && y) || (a == b));
}

