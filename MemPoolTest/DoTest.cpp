#include "MiniMemPool.h"
#include <string.h>
#include <iostream>
#include <vector>
#include <time.h>

#include <queue>

using namespace std;


struct TestUnit
{
    int a[1000];
};

void lookPool(MMemPool* memPool)
{
    cout<<"HeadUsed "<<memPool->HeadUsed<<endl;
    cout<<"TailUsed "<<memPool->TailUsed<<endl;
    cout<<"UsedCnt "<<memPool->UsedCnt<<endl;
    cout<<"HeadFree "<<memPool->HeadFree<<endl;
    cout<<"TailFree "<<memPool->TailFree<<endl;
}

int main()
{
    srand(time(0));
    int memSize, count;
    cout<<"memSzie:";
    cin>>memSize;
    cout<<"count:";
    cin>>count;
    void* memAddr = malloc(memSize);
    
    if(!memAddr)
    {
        printf("!memAddr\n");
        return 0;
    }

    MMemPool* memPool;

    int PoolSize = GetMemPoolSize(sizeof(TestUnit), count);
    cout<<"pollSize:"<<PoolSize<<endl;
    if(PoolSize > memSize)
    {
        cout<<"eeee2"<<endl;
        return 0;
    }
    int ret = MMemPoolInit(memPool, memAddr, sizeof(TestUnit), count);
    if(ret != 0)
    {
        cout<<"Ret:"<<ret<<endl;
        return 0;
    }

    int n=0;
    int op=-1;
    int data = 0;
    vector<TestUnit*> arr;
    while(true)
    {
        if(op < 0)
        {
            while(op++ < 0)
            {
                TestUnit* pT = (TestUnit*)MMemPoolAlloc(memPool, NULL);
                if(pT)
                {
                    pT->a[0] = (10000000+(data++%10000000));
                    cout<<"pT:"<<pT<<" "<<pT->a[0]<<endl;
                    if(pT)arr.push_back(pT);
                }
                else
                {
                    cout<<"PoolFull"<<arr.size()<<" poolused:"<<memPool->UsedCnt<<endl;
                    lookPool(memPool);

                    n=-1;
                }
            }
        }
        else
        {
            while(arr.size() > 0 && op-- > 0)
            {
                int idx = rand()%arr.size();
                swap(arr[idx], arr[arr.size()-1]);
                int ret = MMemPoolFree(memPool, arr.back());
                if(ret != 0)
                {
                    cout<<"free fail "<< arr.size()<<" op:"<< op<<endl;
                }
                arr.pop_back();
            }
            lookPool(memPool);
        }

        cout<<"--------------------"<<endl;
        lookPool(memPool);

        n++;
        cin>>op;
    }
}