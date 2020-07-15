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
typedef struct args{
    int low;
    int high;
    int* arr;
}Args;
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
void* ThreadQuickSort(void *a)
{ 
    Args *args = (Args*)a;
    int low = args->low;
    int high = args->high;
    int* arr = args->arr;
    if(low>=high)
    {
        return NULL;
    }
    if( (high-low+1) <= 5 )
    {
        insertion_sort(&arr[low], high-low+1);
        return NULL;
    }
    int pivot;
    pivot = partition(arr, low, high);
    Args arg1;
    arg1.low = low;
    arg1.high = pivot;
    arg1.arr = arr;
    pthread_t tid1;
    Args arg2;
    arg2.low = pivot+1;
    arg2.high = high;
    arg2.arr = arr;
    pthread_t tid2;
    pthread_create(&tid1, NULL, ThreadQuickSort, &arg1);
    pthread_create(&tid2, NULL, ThreadQuickSort, &arg2);
    
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
}
int main(int argc, char* argv[])
{
    int *arr = (int *)malloc(sizeof(int)*ARRAY_SIZE);
    FILE* fp = fopen(argv[1], "r");
    int x;
    int size_input = 0;
    pthread_t tid;
    while (fscanf(fp, "%d", &x)!=EOF)
    {
        arr[size_input++] = x;
    }
    fclose(fp);
    Args a;
    a.arr = arr;
    a.low = 0;
    a.high = size_input - 1;
    long double st,en,t1;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    pthread_create(&tid, NULL, ThreadQuickSort, &a);
    pthread_join(tid, NULL); 

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);  
    print_arr(a.arr, size_input);
    free(arr);
    return 0;
}