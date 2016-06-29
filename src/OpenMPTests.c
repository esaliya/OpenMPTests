/*
 ============================================================================
 Name        : OpenMPTests.c
 Author      : Saliya Ekanayake
 Version     :
 Copyright   : 
 Description : Hello OpenMP World in C
 ============================================================================
 */
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sched.h>
#include <cstdio>
/**
 * Hello OpenMP World prints the number of threads and the current thread id
 */
int main(int argc, char *argv[]) {

	int numThreads, tid;

	int tcount = 2;

	if (tcount > 1) {
		/* This creates a team of threads; each thread has own copy of variables  */
		omp_set_num_threads(2);
#pragma omp parallel private(numThreads, tid)
		{
			// Mask will contain the current affinity
			cpu_set_t mask;
			CPU_ZERO(&mask);
			// We need the thread pid (even if we are in openmp)
			pid_t tid = (pid_t) syscall(SYS_gettid);
			// Get the affinity
			if( sched_getaffinity(tid, sizeof(mask), (cpu_set_t*)&mask) == -1){
				printf("Error cannot do sched_getaffinityn");
			}
			// Print it
			printf("Thread %d, tid %d, affinity %lun", omp_get_thread_num(), tid, (size_t)mask);

			tid = omp_get_thread_num();
			printf("Hello World from thread number %d\n", tid);

			/* The following is executed by the master thread only (tid=0) */
			if (tid == 0) {
				numThreads = omp_get_num_threads();
				printf("Number of threads is %d\n", numThreads);
			}
		}
	}
	return 0;
}

