#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>



void main() {
	int i = 0;
	while (1) {
		i = i + 1;
		if (i == 5) {
			break;
		}
	}
	int sum = 0;
	int j = 0;
	while (1) {
		j = j + 1;
		if (j % 2 == 0) {
			continue;
		}
		sum = sum + j;
		if (j > 10) {
			break;
		}
	}
}

