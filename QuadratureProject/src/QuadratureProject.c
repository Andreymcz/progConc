/*
 ============================================================================
 Name        : QuadratureProject.c
 Author      : Andrey
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

typedef double (*function)(double);

double calculateIntegral1(double init, double end, function f)
{
	int tid;
	#pragma omp parallel for private(tid)



	return f(100);
}

double xQuad(double x)
{
	return x * x;
}


int main(int argc, char **argv)
{
	//Parse arguments
	int nThreads;
	if(argc > 1) {
		nThreads = atoi(argv[1]);
		omp_set_num_threads(nThreads);
		printf("Número de threads setado para:%d\n", nThreads);
	}

	//Configuração da integral

	double init = 0;
	double end = 10;
	function f = &xQuad;

	//Chamar funcão que calcula:

	double result = calculateIntegral1(init, end, f, nThreads);

	printf("Result=%.5f\n", result);

	return EXIT_SUCCESS;
}

