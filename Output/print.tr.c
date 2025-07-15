#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>



void main() {
	const int name = 123;
	const int age = 30;
	const int city = 5;
	printf("Trust is good\n");
	printf("My name is %d and my age is %d.\n", name, age);
	printf("I live in %d and my name is %d.\n", city, name);
	printf("Value: %d and another: %d.\n", 10 * 5, age + 2);
	printf("Formatted: %d and again: %d and %d.\n", name, name, age);
	printf("Named args: {name_val} and {age_val}.\n");
}

