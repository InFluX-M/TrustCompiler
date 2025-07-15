#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


int add(int a, int b);
int multiply(int x, int y);
void print_message();

int multiply(int x, int y) {
	const int z = x * y;
	return z;
}

int add(int a, int b) {
	return a + b;
}

void print_message() {
	printf("Hello from a function!\n");
}

void main() {
	const int result1 = multiply(3, 4);
	const int result2 = add(result1, 10);
	print_message();
}

