//Cisco Dacanay
//HW3 - Scheduling
//10/17/23

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>

#define ARRAY_SIZE 2000000

//print_progress
#include <ctype.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#define ANSI_COLOR_GRAY    "\x1b[30m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1b[37m"

#define ANSI_COLOR_RESET   "\x1b[0m"

#define TERM_CLEAR() printf("\033[H\033[J")
#define TERM_GOTOXY(x,y) printf("\033[%d;%dH", (y), (x))

void print_progress(pid_t localTid, size_t value) {
    pid_t tid = syscall(__NR_gettid);

    TERM_GOTOXY(0,localTid+1);

	char prefix[256];
    size_t bound = 100;
    sprintf(prefix, "%d: %ld (ns) \t[", tid, value);
	const char suffix[] = "]";
	const size_t prefix_length = strlen(prefix);
	const size_t suffix_length = sizeof(suffix) - 1;
	char *buffer = calloc(bound + prefix_length + suffix_length + 1, 1);
	size_t i = 0;

	strcpy(buffer, prefix);
	for (; i < bound; ++i)
	{
	    buffer[prefix_length + i] = i < value/10000 ? '#' : ' ';
	}
	strcpy(&buffer[prefix_length + i], suffix);
        
    if (!(localTid % 7)) 
        printf(ANSI_COLOR_WHITE "\b%c[2K\r%s\n" ANSI_COLOR_RESET, 27, buffer);  
    else if (!(localTid % 6)) 
        printf(ANSI_COLOR_BLUE "\b%c[2K\r%s\n" ANSI_COLOR_RESET, 27, buffer);  
    else if (!(localTid % 5)) 
        printf(ANSI_COLOR_RED "\b%c[2K\r%s\n" ANSI_COLOR_RESET, 27, buffer);  
    else if (!(localTid % 4)) 
        printf(ANSI_COLOR_GREEN "\b%c[2K\r%s\n" ANSI_COLOR_RESET, 27, buffer);  
    else if (!(localTid % 3)) 
        printf(ANSI_COLOR_CYAN "\b%c[2K\r%s\n" ANSI_COLOR_RESET, 27, buffer);  
    else if (!(localTid % 2)) 
        printf(ANSI_COLOR_YELLOW "\b%c[2K\r%s\n" ANSI_COLOR_RESET, 27, buffer);  
    else if (!(localTid % 1)) 
        printf(ANSI_COLOR_MAGENTA "\b%c[2K\r%s\n" ANSI_COLOR_RESET, 27, buffer);  
    else
        printf("\b%c[2K\r%s\n", 27, buffer);

    fflush(stdout);
	free(buffer);
}
//end print_progress

void* arraySum(void* thread);

typedef struct _thread_data_t {
    int localTid;
    const int *data;
    int numVals;
    pthread_mutex_t *lock;
    long long int *totalSum;
} thread_data_t;

int main(int argc, char *argv[]) {
    if(argc != 2) {
        printf("Not enough parameters\n");
        return -1;
    }

    TERM_CLEAR();

    const int num_threads = atoi(argv[1]);
    int* num_array = (int*)malloc(ARRAY_SIZE*sizeof(int));
    long long int totalSum = 0;

    pthread_mutex_t totalLock;
    pthread_mutex_init(&totalLock, NULL);

    thread_data_t thread_array[num_threads];
    for(int i = 0; i < num_threads; i++) {
        thread_array[i].localTid = i;
        thread_array[i].data = num_array;
        thread_array[i].numVals = ARRAY_SIZE;
        thread_array[i].lock = &totalLock;
        thread_array[i].totalSum = &totalSum;
    }

    pthread_t threads[num_threads];
    for(int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, &arraySum, &thread_array[i]);
    }

    for(int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    free(num_array);

    return 0;
}

void* arraySum(void* thread) {
    thread_data_t threadData = *(thread_data_t*)thread;   //typecast void to thread data
    long long int threadSum = 0;
    while(1) {
        long latency_max = 0;
        for(int i = 0; i < threadData.numVals; i++) {
            struct timespec start;
            clock_gettime(CLOCK_REALTIME, &start);
            threadSum += threadData.data[i];
            struct timespec end;
            clock_gettime(CLOCK_REALTIME, &end);
            if(latency_max < end.tv_nsec - start.tv_nsec) {
                latency_max = end.tv_nsec - start.tv_nsec;
            }
        }
        pthread_mutex_lock(threadData.lock);
        *threadData.totalSum += threadSum;
        pthread_mutex_unlock(threadData.lock);

        print_progress(threadData.localTid, latency_max);
    }
    return 0;   //gets rid of "control reaches end of non-void function" warning
}