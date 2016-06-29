/*
 ============================================================================
 Name        : -program.c
 Author      : Saliya Ekanayake
 Version     :
 Copyright   : 
 Description : MPI+OpenMP program demonstrating explicit
 	 	 	   thread affinity control
 ============================================================================
 */

#define _GNU_SOURCE

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <limits.h>
#include <sched.h>
#include <mpi.h>
#include <stdarg.h>
#include <unistd.h>

void set_bit_mask(int rank, int thread_id, int tpp, int nodes, cpu_set_t *mask);
int parse_args(int argc, char **argv);

int world_proc_rank;
int world_procs_count;

int num_threads;
int num_nodes;

int main(int argc, char *argv[]) {

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &world_proc_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_procs_count);

	int ret = parse_args(argc, argv);

	if (ret) {
		MPI_Finalize();
		return -1;
	}

	if (num_threads > 1) {
		omp_set_num_threads(num_threads);
#pragma omp parallel private (ret)
		{
			int thread_id = omp_get_thread_num();
			cpu_set_t mask;
			CPU_ZERO(&mask); // clear mask
			set_bit_mask(world_proc_rank, thread_id, num_threads, num_nodes, &mask);
			ret = sched_setaffinity(0, sizeof(mask), &mask);
			if (ret < 0)
			{
				printf("Error in setting thread affinity at rank %d and thread %d\n", world_proc_rank, thread_id);
			}

			/* Print affinity */
			// We need the thread pid (even if we are in openmp)
			pid_t tid = (pid_t) syscall(SYS_gettid);
			// Get the affinity
			CPU_ZERO(&mask); // clear mask
			if (sched_getaffinity(tid, sizeof(mask), &mask) == -1) {
				printf("Error cannot do sched_getaffinity at rank %d and thread %d\n", world_proc_rank, thread_id);
			}
			// Print it
			int j;
			for (j = 0; j < CPU_SETSIZE; ++j) {
				if (CPU_ISSET(j, &mask)) {
					printf("Rank %d Thread %d, tid %d, affinity %d\n",
							world_proc_rank, thread_id, tid, j);
				}

			}
		}
	}

	MPI_Finalize();
	return 0;
}

void set_bit_mask(int rank, int thread_id, int tpp, int nodes, cpu_set_t *mask) {
	/* Hard coded values for Juliet*/
	int cps = 12; // cores per socket
	int spn = 2; // sockets per node
	int htpc = 2; // hyper threads per core
	int cpn = cps * spn; // cores per node

	int ppn = cpn / tpp; // process per node
	int cpp = cpn / ppn; // cores per process

	// Assuming continuous ranking within a node
	int node_local_rank = rank % ppn;

	int j;
	for (j = 0; j < htpc; ++j) {
		CPU_SET(node_local_rank * cpp + thread_id + (cpn * j), mask);
	}
}

int parse_args(int argc, char **argv) {
	int index;
	int c;

	opterr = 0;
	while ((c = getopt(argc, argv, "T:N:")) != -1)
		switch (c) {
		case 'T':
			num_threads = atoi(optarg);
			break;
		case 'N':
			num_nodes = atoi(optarg);
			break;
		case '?':
			if (optopt == 'T' || optopt == 'N')
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint(optopt))
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			return 1;
		default:
			abort();
		}
	if (world_proc_rank == 0) {
		printf("Program Arguments\n");
		printf(" T = %d\n",num_threads);
	}

	for (index = optind; index < argc; index++)
		printf("Non-option argument %s\n", argv[index]);
	return 0;
}

