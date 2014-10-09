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

///////////////////
/// 	DEFINES
////////////////
#define dprintf //
//#define dprintf printf
#define true 1
#define false 0
#define NUMINICIAL 20
#define EPSILON 10e-7
#define TRESHOLD 10e-10
#define assert(b) if(!b) {printf("assertation error at%s(%d)\n", __FILE__, __LINE__); getchar(); exit(0);}
#define AREA(i, e, f) ((f(i) + f(e)) *(e - i)) * 0.5
typedef double (*function)(double);

//Global variables
typedef struct task {
	double init;
	double end;
	struct task *next;
	struct task *prev;
} task;

typedef struct queue {
	int size;
	struct task *front;
	struct task *tail;
} queue;

queue *q;

int terminated = 0;
int *idle;

void initQueue() {
	q = malloc(sizeof(queue));
	q->front = NULL;
	q->tail = NULL;
	q->size = 0;
}

void enqueue(task* t_new) {
	task *old_tail = q->tail;
	if (old_tail == NULL) {
		t_new->prev = NULL;
		t_new->next = NULL;
		q->front = t_new;
		q->tail = t_new;
	} else {
		q->tail = t_new;
		t_new->next = old_tail;
		t_new->prev = NULL;
		old_tail->prev = t_new;
	}

	q->size = q->size + 1;
}

task *dequeue() {
	if (q->size == 0)
		return NULL;
	dprintf("q.size:%d\n", q->size);
	task *t = q->front;

	if (t != NULL) {
		dprintf("q.front!=null\n");
		//Atualizar o front
		q->front = t->prev;
		if (q->front != NULL)
			q->front->next = NULL;

		if(q->size == 1)
			q->tail = NULL;

		q->size = q->size - 1;
	} else {
		dprintf("q.front==NULL\n");
	}
	return t;
}

///////////////////
/// 	FUNCOES DE INTEGRAL
////////////////
double xLinear(double x) {
	return x;
}

double xQuad(double x) {
	return x * x;
}

double xCompose(double x) {
	if (x < 10) {
		return xLinear(x);
	} else if (x < 20) {
		return 1.0 / xLinear(x);
	} else {
		return xQuad(x);
	}
}

///////////////////
/// 	METODO DE CALCULO 1
////////////////
double adaptQuad(double init, double end, function f) {

	double areaBig = AREA(init, end, f); //Area considerando somente as extremidades
	double mid = init + ((end - init) / 2.0);
	double areaSmall = (AREA(init, mid, f)) + (AREA(mid, end, f)); //Area considerando os dois trapezios

	if (fabs(areaBig - areaSmall) < TRESHOLD) { // Se a diferenca for insignificante, retornamos este valor
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
		double myInterval = (interval / nThreads);
		double myInit = tid * myInterval;
		//printf("Thread(%d): MyInit->%.2f, MyEnd->%.2f\n", tid, myInit, myInit + myInterval);
		//printf("Calling adaptquad\n");
		double mySum = adaptQuad(myInit, myInit + myInterval, f);

#pragma omp critical (sum)
		{
			dprintf("Thread(%d): MySum->%.2f\n", tid, mySum);
			sum += mySum;
		}

	}

	return sum;
}

///////////////////
/// 	METODO DE CALCULO 2
////////////////

task* receiveTask() {
	task *t;
#pragma omp critical (task)
	{
		t = dequeue();
	}
	return t;
}

void sendTask(double init, double end) {
#pragma omp critical (task)
	{
		task *t = malloc(sizeof(task));
		t->init = init;
		t->end = end;
		enqueue(t);

	}
}

void sendResult(double result, double *sum) {
#pragma omp critical (sum)
	{
		*sum = *sum + result;
	}
}

int notTerminated() {
	return !terminated;
}
//Workers
void executeTasks(function f, double *sum, int tid) {
	task *bag;
	while (notTerminated()) {
		dprintf("Thread%d, waiting a new task\n", tid);
		if ((bag = receiveTask()) != 0) {
			idle[tid] = false;
			dprintf("Thread%d, received a new task: %.2f,%.2f\n", tid, bag->init, bag->end);
			//calculate area
			double areaBig = AREA(bag->init, bag->end, f);//Area considerando somente as extremidades
			double mid = bag->init + ((bag->end - bag->init) / 2.0);
			double areaSmall = (AREA(bag->init, mid, f))
					+ (AREA(mid, bag->end, f)); //Area considerando os dois trapezios

			if (fabs(areaBig - areaSmall) < TRESHOLD) { // Se a diferenca for insignificante, retornamos este valor
				dprintf("Thread%d, sending result: a new task: %.25f,%.25f = %.10f\n", tid, bag->init, bag->end, areaSmall);
				sendResult(areaSmall, sum);

			} else {
				dprintf("diff too large:%.25f, TRESHOLD:%.25f\n",fabs(areaBig - areaSmall), TRESHOLD);
				// enviamos as subtarefas para a fila
				sendTask(bag->init, mid);
				sendTask(mid, bag->end);
			}
			//Agora podemos jogar fora a task atual
			free(bag);
		}
		idle[tid] = true;
	}
}

//Master
void sendTasksAndWaitForResponse(double init, double end, function f,
		double *sum) {

	int nThreads = omp_get_num_threads();

	double interval = (end - init) / NUMINICIAL;
	for (; init < end; init += interval) {
		dprintf("sending task:%.2f, %.2f\n", init, init + interval);
		sendTask(init, init + interval);
	}

	while (!terminated) {
		int n_idle = 0;
		for (int n = 1; n < nThreads; ++n) {
			n_idle += idle[n];
		}

		terminated = (q->size == 0) && (n_idle >= (nThreads - 1)); //se todas as threads estiverem idle(menos a master), entao acabamos
		dprintf("terminated:%d , size:%d , n_idle:%d\n", terminated, q->size, n_idle);
	}


}
double calculateIntegral2(double init, double end, function f) {
	double sum = 0;

	//Init queue
	initQueue();

#pragma omp parallel shared(sum, idle)
	{
		int tid;
		tid = omp_get_thread_num();
		int nThreads = omp_get_num_threads();
		idle = malloc(nThreads * sizeof(int));
		for (int i = 0; i < nThreads; ++i) {
			idle[i] = 0; // A principio nenhuma thread esta sem trabalho
		}

		if (tid == 0) { //Master
			sendTasksAndWaitForResponse(init, end, f, &sum);
		} else { //Slaves
			executeTasks(f, &sum, tid);
		}
	}
	return sum;
}
///////////////////
/// 	MAIN
////////////////
int main(int argc, char **argv) {
	//Parse arguments
	int nThreads;
	if (argc > 1) {
		nThreads = atoi(argv[1]);
		omp_set_num_threads(nThreads);
		printf("N��mero de threads setado para:%d\n", nThreads);
	}
//	omp_set_num_threads(6);
	//Configura����o da integral

	double init = 0;
	double end = 100000;
	//function f = &xLinear;
//	function f = &xQuad;
	function f = &xCompose;

	//Chamar func��o que calcula:
	//double result = calculateIntegral1(init, end, f);
	double result = calculateIntegral2(init, end, f);

	printf("Result=%.20f\n", result);

	return EXIT_SUCCESS;
}

