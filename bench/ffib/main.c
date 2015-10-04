#include <stdio.h>
#include <stdlib.h>

double ffib( double n ) {
	if( n <= 1. ) return 1.;
	return ffib(n-1.) + ffib(n-2.);
}

int main() {
	printf("%.19gf\n", ffib(39));
	return 0;
}