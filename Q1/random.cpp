#include<bits/stdc++.h>
using namespace std;

int main() 
{   
    int size;
    //cout<< "how big do you want the array?" << endl;
    cin>>size;
    int *arr = new int[size];
    cout<<size<<endl;
    srand((unsigned)time(0)); 
        
    for(int i=0; i<size; i++)
    { 
        arr[i] = (rand());  
        printf("%d\n", arr[i]);
    } 
    delete arr;
}
