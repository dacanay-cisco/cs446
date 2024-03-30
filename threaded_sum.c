//Cisco Dacanay
//PA2: Threaded Sum
//10/3/23

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>

#define MAX_VALUES 1000000

int readFile(char filename[], int values[]);
void* arraySum(void* thread);

typedef struct _thread_data_t {
const int *data;
int startInd;
int endInd;
pthread_mutex_t *lock;
long long int *totalSum;
} thread_data_t;

int main(int argc, char *argv[]) {
    const int num_args = argc, num_threads = atoi(argv[2]);    //convert 2nd arg from string to int

    if(num_args != 3) {
        printf("Not enough parameters\n");
        return -1;
    }

    long long int totalSum = 0;
    int values[MAX_VALUES];
    const int num_values = readFile(argv[1], values);

    if(num_values == -1) {
        printf("File not found.\n");
        return -1;
    }
    else if(num_threads > num_values) {
        printf("Too many threads requested\n");
        return -1;
    }

    struct timeval startTime, endTime;
    gettimeofday(&startTime, NULL);

    pthread_mutex_t totalLock;

    thread_data_t dataArray[num_threads];
    int startIndex = 0, thread_counter = num_threads, value_counter = num_values, value_alloc = 0;
    for(int i = 0; i < num_threads; i++) {
        dataArray[i].data = values;
        dataArray[i].startInd = startIndex;
        value_alloc = value_counter / thread_counter;   //assign each thread (# values left / # threads left) # of values
        dataArray[i].endInd = startIndex + value_alloc - 1;
        //printf("Thread %d -- Start: %d, End: %d\n", i, startIndex, startIndex + value_alloc - 1); //show index assignments for each thread
        value_counter -= value_alloc;
        thread_counter--;   //decrement unassigned values/threads left
        startIndex += value_alloc;
        dataArray[i].lock = &totalLock;
        dataArray[i].totalSum = &totalSum;
    }

    pthread_t threads[num_threads];
    for(int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, &arraySum, &dataArray[i]);
    }

    for(int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    gettimeofday(&endTime, NULL);
    long int execTime = (endTime.tv_usec - startTime.tv_usec);   //was only getting 0s with ms, was also seeing overflow so needed long int
    printf("Sum: %lld\n", totalSum);
    printf("Exec Time: %ld us\n", execTime);

    pthread_mutex_destroy(&totalLock);

    return 0;
}

int readFile(char filename[], int values[]) {
    FILE* fileptr = fopen(filename, "r");
    if(fileptr == NULL) {
        return -1;
    }
    int i = 0;
    while(fscanf(fileptr, "%d", &values[i]) == 1) {
        i++;
    }
    return i;
}

void* arraySum(void* thread) {
    thread_data_t threadData = *(thread_data_t*)thread;   //typecast void to thread data
    long long int threadSum = 0;
    for(int i = threadData.startInd; i <= threadData.endInd; i++) {
        threadSum += threadData.data[i];
    }
    pthread_mutex_lock(threadData.lock);    //lock right before adding to totalSum was most efficient
    *threadData.totalSum += threadSum;
    pthread_mutex_unlock(threadData.lock);
    
    return 0;   //gets rid of "control reaches end of non-void function" warning
}