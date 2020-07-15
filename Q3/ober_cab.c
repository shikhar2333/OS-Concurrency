#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <stdbool.h>
#define waitState 0
#define onRidePremier 1
#define onRidePoolFull 2
#define onRidePoolOne 3
#define TIMEOUT 0 
#define NOTIMEOUT 1
typedef struct PremierCab{
    int Cab_State;
    //void (*MakePayment)();
    time_t pres_wait_time;
    int cur_rider_index;
}PremierCab;
typedef struct PoolCab{
    int Cab_State;
    //void (*BookCab)(int cabType, int maxWaitTime, int RideTime);
    //void (*MakePayment)();
    time_t pres_wait_time[2];
    int cur_rider_index[2];
}PoolCab;
typedef struct Rider{
    int Rider_Id;
    int CabType;       
    int maxWaitTime;
    int RideTime;
    bool RideStatus;     // true means riding and false means not riding
    bool PaymentPending;  // true means Payment pending and false means payment done/not riding cab.
    int Cab_Id;
    int PoolRiderIndex;
    int (*BookCab)(struct Rider** rider, int BookTime);
    void (*MakePayment)(struct Rider** rider);
    void (*RideLoop)(int t, int x);
}Rider;
typedef struct Server
{
    bool isBusy;    // true means payment server is busy and false means it's free
    int cur_user;
    int cur_cab;
    int cur_cabtype;   
}Server;
pthread_t* Rider_Threads;
pthread_t* Server_Threads;
pthread_mutex_t* Rider_mutex;
pthread_mutex_t* Server_mutex;
PremierCab* PremierCabs;
PoolCab* PoolCabs;
Rider* Riders;
Server* Servers;
int noCabs, noRiders, noPaymentServers, no_pool_cabs, no_premier_cabs;
int kill = 0;

void* ServerThread(void *argv)
{
    Server* server = (Server*)argv;
    while(!kill)
    {
        if(server->isBusy)
        {
            sleep(2);
            pthread_mutex_lock(&Rider_mutex[server->cur_user]);
            Rider* rider;
            rider = &Riders[server->cur_user];
            if(!(server->cur_cabtype))
            {
                PremierCab* premier;
                premier = &PremierCabs[server->cur_cab];
                premier->Cab_State = waitState;
                premier->pres_wait_time = -1;
                premier->cur_rider_index = -1;
                rider->PaymentPending = false;
            }
            else if (server->cur_cabtype>0)
            {
                PoolCab* pool;
                pool = &PoolCabs[server->cur_cab];
                if(pool->Cab_State==onRidePoolOne)
                {
                    pool->cur_rider_index[rider->PoolRiderIndex] = -1;
                    pool->pres_wait_time[rider->PoolRiderIndex] = -1;
                    pool->Cab_State = waitState;
                    rider->PaymentPending = false;
                }
                else if(pool->Cab_State==onRidePoolFull)
                {
                    pool->cur_rider_index[rider->PoolRiderIndex] = -1;
                    pool->pres_wait_time[rider->PoolRiderIndex] = -1;
                    pool->Cab_State = onRidePoolOne;
                    rider->PaymentPending = false;
                } 
            }
            pthread_mutex_unlock(&Rider_mutex[server->cur_user]);
        }
        else if(!(server->isBusy))
        {
            continue;
        }
    }
    return NULL;
}
int BookCab(Rider** rider, int BookTime)
{
    //int BookTime = time(NULL);
    while( !( (*rider)->RideStatus ) ) 
    {
        int CurTime = time(NULL);
        if( (CurTime - BookTime)>=(*rider)->maxWaitTime )
        {
            printf("Rider with rider id = %d Timed Out\n", (*rider)->Rider_Id);
            (*rider)->RideStatus = true;
            return TIMEOUT;
        }
        // printf("%d ", (*rider)->CabType);
        // printf("%d\n", no_pool_cabs);
        int RideTime = (*rider)->RideTime;
        int RiderId = (*rider)->Rider_Id;
        int i = 0;
        if(  !( (*rider)->CabType ) )
        {
            // for(int i = 0; i<no_premier_cabs; i++)
            while(i<no_premier_cabs)
            {
                if(PremierCabs[i].Cab_State==waitState)
                {
                    PremierCabs[i].Cab_State = onRidePremier;
                    PremierCabs[i].pres_wait_time = RideTime;
                    PremierCabs[i].cur_rider_index = RiderId;
                    (*rider)->Cab_Id = i;
                    (*rider)->RideStatus = true;
                    (*rider)->PaymentPending = true;
                    break;
                }
                i++;
            }
        }
        else if((*rider)->CabType>0)
        {
            int i = 0;
            while(i<no_pool_cabs)
            {
                //printf("%d\n", PoolCabs[i].Cab_State);
                if(PoolCabs[i].Cab_State==waitState || PoolCabs[i].Cab_State==onRidePoolOne)
                {
                    (*rider)->Cab_Id = i;
                    (*rider)->RideStatus = true;
                    (*rider)->PaymentPending = true;
                }
                if(PoolCabs[i].Cab_State==waitState)
                {
                    PoolCabs[i].Cab_State = onRidePoolOne;
                    PoolCabs[i].pres_wait_time[0] = RideTime;
                    PoolCabs[i].cur_rider_index[0] = RiderId;
                    (*rider)->PoolRiderIndex = 0;
                    // (*rider)->Cab_Id = i;
                    // (*rider)->RideStatus = true;
                    // (*rider)->PaymentPending = true;
                    break;
                }
                if(PoolCabs[i].Cab_State==onRidePoolOne)
                {
                    PoolCabs[i].Cab_State = onRidePoolFull;
                    PoolCabs[i].pres_wait_time[1] = RideTime;
                    PoolCabs[i].cur_rider_index[1] = RiderId;
                    // (*rider)->Cab_Id = i;
                    // (*rider)->RideStatus = true;
                    // (*rider)->PaymentPending = true;
                    (*rider)->PoolRiderIndex = 1;
                    break;
                }
                i++;
            }
        }
    }
    return NOTIMEOUT;
}
void RideLoop(int InitTime, int RideTime)
{
    int CurTime = time(NULL);
    while(true)
    {   
        if( (CurTime - InitTime )> RideTime ) 
        {
            break;
        }
        CurTime = time(NULL);
    }
}
void MakePayment(Rider** rider)
{
    while((*rider)->PaymentPending)
    {
        int PayOnServer = -1;
        int i = 0;
        while(i<noPaymentServers)
        {
            if(!Servers[i].isBusy)
            {
                printf("Rider with rider id = %d starting payment on server with server id = %d\n",(*rider)->Rider_Id, i);
                pthread_mutex_lock(&Server_mutex[i]);
                PayOnServer = i;
                Servers[i].isBusy = true;
                Servers[i].cur_user = (*rider)->Rider_Id;
                Servers[i].cur_cabtype = (*rider)->CabType;
                Servers[i].cur_cab = (*rider)->Cab_Id;
                pthread_mutex_unlock(&Server_mutex[i]);
                break;
            }
            i++;
        }
        if(PayOnServer!=-1)
        {
            while( Servers[PayOnServer].isBusy && (*rider)->PaymentPending );
            (*rider)->PaymentPending = false;
            pthread_mutex_lock(&Server_mutex[PayOnServer]);
            printf("Payment by rider with rider id = %d done for cabid = %d for CabType = %d\n", (*rider)->Rider_Id, (*rider)->Cab_Id, (*rider)->CabType);
            Servers[PayOnServer].isBusy = false;
            pthread_mutex_unlock(&Server_mutex[PayOnServer]);
        }
    }
}
void* RiderThread(void *argv)
{
    Rider* rider = (Rider*)argv;
    pthread_mutex_lock(&Rider_mutex[rider->Rider_Id]);
    //srand(time(NULL)); 
    rider->CabType = rand()%2;
    rider->maxWaitTime = rand()%10 + 5;
    rider->RideTime = rand()%15 + 5;
    printf("Rider with rider id = %d wants a ride of type %d with RideTime = %d and maxWaitTime = %d\n", rider->Rider_Id, rider->CabType, rider->RideTime, rider->maxWaitTime);
    int return_val = rider->BookCab(&rider, time(NULL));
    if(return_val==TIMEOUT)
    {
        return NULL;
    }
    pthread_mutex_unlock(&Rider_mutex[rider->Rider_Id]);
    printf("Rider with rider id = %d has booked cab %d of type %d with RideTime = %d\n", rider->Rider_Id, rider->Cab_Id, rider->CabType, rider->RideTime);
    rider->RideLoop(time(NULL), rider->RideTime);
    printf("Rider with rider id = %d has finished cab with Cab id = %d and Cabtype = %d\n", rider->Rider_Id, rider->Cab_Id, rider->CabType);
    rider->MakePayment(&rider);
    return NULL;
}
void InitThreads(void)
{
    Rider_Threads = malloc(sizeof(pthread_t) * noRiders);
    Rider_mutex = malloc(sizeof(pthread_mutex_t) * noRiders);

    Server_Threads = malloc(sizeof(pthread_t) * noPaymentServers);
    Server_mutex = malloc(sizeof(pthread_mutex_t) * noPaymentServers);

    int i = 0;
    while(i<noRiders)
    {
        pthread_create(&Rider_Threads[i], NULL, RiderThread, (void*)&Riders[i]);
        pthread_mutex_init(&Rider_mutex[i], NULL);
        i++;
    }
    i = 0;
    while(i<noPaymentServers)
    {
        pthread_create(&Server_Threads[i], NULL, ServerThread, (void*)&Servers[i]);
        pthread_mutex_init(&Server_mutex[i], NULL);
        i++;
    }
}
int main(int argc, char* argv[])
{
    scanf("%d %d %d", &noCabs, &noRiders, &noPaymentServers);
    no_premier_cabs = noCabs/2;
    no_pool_cabs = noCabs - no_premier_cabs;
    PremierCabs = malloc(sizeof(PremierCab)*no_premier_cabs);
    PoolCabs = malloc(sizeof(PoolCab)*no_pool_cabs);
    Riders = malloc(sizeof(Rider)*noRiders);
    Servers = malloc(sizeof(Server)*noPaymentServers);
    srand(time(NULL));
    for(int i = 0; i<no_pool_cabs; i++)
    {
        PoolCabs[i].Cab_State = waitState;
        //PoolCabs[i].BookCab = BookCab;
        PoolCabs[i].pres_wait_time[0] = -1;
        PoolCabs[i].pres_wait_time[1] = -1;
        PoolCabs[i].cur_rider_index[0] = -1;
        PoolCabs[i].cur_rider_index[1] = -1;
    }
    for(int i = 0; i<no_premier_cabs; i++)
    {
        PremierCabs[i].Cab_State = waitState;
        //PremierCabs[i].BookCab = BookCab;
        PremierCabs[i].pres_wait_time = -1;
        PremierCabs[i].cur_rider_index = -1;
    }
    for(int i = 0; i<noRiders; i++)
    {
        Riders[i].Rider_Id = i;
        Riders[i].CabType = -1;
        Riders[i].maxWaitTime = -1;
        Riders[i].RideTime = -1;
        Riders[i].RideStatus = false;
        Riders[i].PaymentPending = false;
        Riders[i].Cab_Id = -1;
        Riders[i].PoolRiderIndex = -1;
        Riders[i].BookCab = BookCab;
        Riders[i].RideLoop = RideLoop;
        Riders[i].MakePayment = MakePayment;
    }
    for(int i = 0; i<noPaymentServers; i++)
    {
        Servers[i].isBusy = false;
        Servers[i].cur_user = -1;
        Servers[i].cur_cab = -1;
        Servers[i].cur_cabtype = -1;
    }
    
    InitThreads();
    for (int i = 0; i < noRiders; i++)
    {
        pthread_join(Rider_Threads[i], NULL);
    }
    kill = 1;
    for (int i = 0; i < noPaymentServers; i++)
    {
        pthread_join(Server_Threads[i], NULL);
    }
    pthread_exit(NULL);
    return 0;
}