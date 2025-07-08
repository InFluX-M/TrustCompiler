#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


int add(int a, bool b);

int add(int a, bool b) {
	return a;
}

void main() {
	const int result = add(5, true);
	printf("%d\n", result);
}

