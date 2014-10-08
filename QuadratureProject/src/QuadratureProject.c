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
#include <math.h>

#define EPSILON 10e-7
#define TRESHOLD 10e-20
#define assert(b) if(!b) {printf("assertation error at%s(%d)\n", __FILE__, __LINE__); getchar(); exit(0);}

#define AREA(i, e, f) ((f(i) + f(e)) *(e - i)) * 0.5

typedef double (*function)(double);

double xLinear(double x) {
	return x;
}

double xQuad(double x) {
	return x * x;
}

double adaptQuad(double init, double end, function f){

	double areaBig =  AREA(init, end, f);//Area considerando somente as extremidades
	double mid = init + ((end - init) / 2.0);
	double areaSmall  = (AREA(init, mid, f)) + (AREA(mid, end, f)); //Area considerando os dois trapezios

	if(fabs(areaBig - areaSmall) < TRESHOLD){ // Se a diferenca for insignificante, retornamos este valor
		return areaSmall;
	} else { //se nao, vamos continuar nossa busca por um valor mais decente
		return adaptQuad(init, mid, f) + adaptQuad(mid, end, f);
	}
}

double calculateIntegral1(double init, double end, function f) {

	assert(end > init);
	double interval = end - init;
	double sum = 0;
	#pragma omp parallel shared(sum)
	{
		int tid, nThreads;
		tid = omp_get_thread_num();
		nThreads = omp_get_num_threads();
		double myInterval = (interval/nThreads);
		double myInit = tid * myInterval;
		//printf("Thread(%d): MyInit->%.2f, MyEnd->%.2f\n", tid, myInit, myInit + myInterval);
		//printf("Calling adaptquad\n");
		double mySum = adaptQuad(myInit, myInit+ myInterval, f);

		#pragma omp critical
		{
			printf("Thread(%d): MySum->%.2f\n", tid, mySum);
			sum += mySum;
		}

	}

	return sum;
}

int main(int argc, char **argv) {
	//Parse arguments
	int nThreads;
	if (argc > 1) {
		nThreads = atoi(argv[1]);
		omp_set_num_threads(nThreads);
		printf("N��mero de threads setado para:%d\n", nThreads);
	} else {
		omp_set_num_threads(8);
	}

	//Configura����o da integral

	double init = 0;
	double end = 100;
	function f = &xQuad;

	//Chamar func��o que calcula:
	double result = calculateIntegral1(init, end, f);
	printf("Result=%.20f\n", result);


	return EXIT_SUCCESS;
}

