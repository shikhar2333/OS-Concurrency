#define _POSIX_C_SOURCE 199309L
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>
#define ARRAY_SIZE 1000000

void print_arr(int a[], int size)
{
    for(int i = 0; i<size; i++)
    {
        printf("%d\n", a[i]);
    }
}
void swap(int *x, int *y)
{
    int temp = *x;
    *x = *y;
    *y = temp;
}
int partition(int *arr, int low, int high)
{
    srand(time(NULL)); 
    int random = low + rand() % (high - low); 
    swap(&arr[random], &arr[low]); 
    int small_index = low-1, large_index = high+1;
    int pivot_value = arr[low];
    while(1)
    {
        while(arr[++small_index]<pivot_value);   
        while(arr[--large_index]>pivot_value);
        if(small_index>=large_index)
        {
            return large_index;
        }
        swap(&arr[small_index], &arr[large_index]);
    }
}
void insertion_sort(int *arr, int size)
{
    for(int i = 1; i<size; i++)
    {
        int j = i-1;
        int check = arr[i];
        while(1)
        {
            if(j<0 || arr[j]<check)
            {
                break;
            }
            arr[j+1] = arr[j];
            j--;
        }
        arr[j+1] = check;
    }
}
void concurrent_quicksort(int *arr, int low, int high)
{ 
    if(low>=high)
    {
        return;
    }
    if( (high-low+1) <= 5 )
    {
        insertion_sort(arr+low, high-low+1);
        return;
    }
    int pivot;
    //print_arr(arr);
    pivot = partition(arr, low, high);
    //printf("%d\n", pivot);
    pid_t left_part, right_part;
    left_part = fork();
    int status;
    if(left_part<0)
    {
        perror("left fork() error: ");
        exit(EXIT_FAILURE);
    }
    if(left_part==0)
    {
        concurrent_quicksort(arr, low, pivot);
        exit(EXIT_SUCCESS);
    } 
    else
    {
        right_part = fork();
        if(right_part<0)
        {
            perror("right fork() error");
            exit(EXIT_FAILURE);
        }
        else if(right_part==0)
        {
            concurrent_quicksort(arr, pivot+1, high);
            exit(EXIT_SUCCESS);
        }
    }
    waitpid(left_part, &status, 0);
    waitpid(right_part, &status, 0);
}
int main(int aargc, char* argv[])
{
    int *arr = (int *)malloc(sizeof(int)*ARRAY_SIZE);
    FILE* fp = fopen(argv[1], "r");
    int x;
    int size_input = 0;
    while (fscanf(fp, "%d", &x)!=EOF)
    {
        arr[size_input++] = x;
    }
    fclose(fp);
    int *shm_arr;
    int shm_id = shmget(IPC_PRIVATE, size_input*sizeof(int), IPC_CREAT | 0666);
    if(shm_id<0)
    {
        perror("shmgt() error");
        exit(EXIT_FAILURE);
    }
    shm_arr = shmat(shm_id, NULL, 0);
    if(shm_arr == (int *)-1)
    {
        perror("shmat() error");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i<size_input; i++)
    {
        shm_arr[i] = arr[i];
    }
    long double st,en,t1;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    concurrent_quicksort(shm_arr, 0, size_input - 1);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    
    print_arr(shm_arr, size_input);
    int status = shmdt(shm_arr);
	if(status<0)
	{
		perror("shmdt() error");
		exit(EXIT_FAILURE);
	}
    status = shmctl(shm_id, IPC_RMID, NULL);
    if (status<0)
	{
		perror("shmctl() error");
		exit(EXIT_FAILURE);
	}
    free(arr);
    return 0;
}