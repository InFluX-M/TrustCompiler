#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>



void main() {
	const int a = 5;
	const int b = 3;
	const int sum = a + b;
	const int product = a * b;
	const int division = a / b;
	const int mod_result = a % b;
	printf("%d\n", sum);
	printf("%d\n", product);
	printf("%d\n", division);
	printf("%d\n", mod_result);
}

