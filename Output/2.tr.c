#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


int add(int a, bool b);

int add(int a, bool b) {
	return a;
}

void main() {
	const void result = add(5);
	printf("%d\n", result);
}

