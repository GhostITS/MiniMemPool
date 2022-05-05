#include "MiniMemPool.h"
#include <stdio.h>
#include <string.h>

#define _DEBUG
#ifdef _DEBUG
#define LOG_DEBUG(...)                                 \
    {                                                  \
        printf("[Debug] %s:%d: ", __FILE__, __LINE__); \
        printf(__VA_ARGS__);                           \
        printf("\n");                                  \
    }
#else
#define LOG_DEBUG(...)
#endif

#define NULL_BLOCK 0
#define ValidBlock(x) (x > NULL_BLOCK)

enum MMemBlockType
{
    FREE = 0,
    USED = 1,
};

static MMemBlock *MMemBlockIdx(MMemPool *Pool, int idx)
{
    idx--;
    if (idx < 0)
    {
        return NULL;
    }
    return (MMemBlock *)(Pool->Data + idx * Pool->FullUnit);
}

void* MMemPoolGet(MMemPool* Pool, int idx)
{
    MMemBlock* pBlock = MMemBlockIdx(Pool, idx);
    if(!pBlock || pBlock->Valid!=MMemBlockType::USED)
    {
        return NULL;
    }
    return pBlock;
}

int MMemPoolInit(MMemPool *&Pool, void *MemAddr, int Unit, int Count)
{
    if (Count <= 0 || Unit <= 0)
    {
        LOG_DEBUG("Count <= 0|| Unit<=0");
        return ERR_OBJECT_SIZE_INVALID;
    }
    Pool = (MMemPool *)MemAddr;
    BZERO(*Pool);

    int Size = GetMemPoolSize(Unit, Count);
    Pool->Valid = 1;
    Pool->Count = Count;
    Pool->Size = Size;
    Pool->Unit = Unit;
    Pool->FullUnit = Unit + MMemBlockHeadSize;

    Pool->HeadFree = 1;
    Pool->TailFree = Pool->HeadFree;

    MMemBlock *allocBlock = MMemBlockIdx(Pool, Pool->HeadFree);
    BZERO(*allocBlock);

    allocBlock->Count = Count;

    //需要初始化所有block的标记
    for(int i = 0; i < Pool->Count; i++)
    {
        (allocBlock+i)->Valid=MMemBlockType::FREE;
    }

    return 0;
}

int MMemPoolAttach(MMemPool *&Pool, void *MemAddr, int Unit, int Count)
{
    Pool = (MMemPool *)MemAddr;
    if (Pool->Size != GetMemPoolSize(Unit, Count))
    {
        LOG_DEBUG("Pool->Size != GetMemPoolSize");
        return ERR_MEMORY_ALIGN_FAIL;
    }
    if (Count <= 0 || Unit <= 0)
    {
        LOG_DEBUG("Count <= 0|| Unit<=0");
        return ERR_OBJECT_SIZE_INVALID;
    }

    if (Count != Pool->Count || Unit != Pool->Unit)
    {
        LOG_DEBUG("Count != Pool->Count || Unit!=Pool->Unit");
        return ERR_OBJECT_SIZE_INVALID;
    }

    //TODO 检查链表

    return 0;
}

static void InitMemBlock(MMemBlock *pMemBlock, int iCount, int iPrev, int iNext)
{
    pMemBlock->Count = iCount;
    pMemBlock->Prev = iPrev;
    pMemBlock->Next = iNext;
}

void *MMemPoolAlloc(MMemPool *Pool, int* idx)
{
    if (Pool->Count <= Pool->UsedCnt || !ValidBlock(Pool->HeadFree))
    {
        LOG_DEBUG("Pool->Count <= Pool->UsedCnt");
        return NULL;
    }

    int iAllocBlock = Pool->HeadFree;
    MMemBlock *allocBlock = (MMemBlock *)MMemBlockIdx(Pool, iAllocBlock);
    //MMemBlock* next = (MMemBlock*)MMemPoolGet(Pool, allocBlock->Next);
    MMemBlock *next = NULL;
    int iNewHead = Pool->HeadFree;
    int iNewTailFree = Pool->TailFree;
    //分割一块出来
    if (allocBlock->Count > 1)
    {
        iNewHead = iAllocBlock + 1;
        MMemBlock *pNewHead = (MMemBlock *)MMemBlockIdx(Pool, iNewHead);
        //BZERO(*pNewHead);
        InitMemBlock(pNewHead, allocBlock->Count - 1, NULL_BLOCK, allocBlock->Next);
        
        allocBlock->Count = 1;
        //因为头的结构被分割了, 所以要处理下next节点或者tail节点
        if (ValidBlock(allocBlock->Next))
        {
            MMemBlock *pNext = (MMemBlock *)MMemBlockIdx(Pool, allocBlock->Next);
            pNext->Prev = iNewHead;
        }
        else
        {
            iNewTailFree = iNewHead;
        }
    }
    else
    {
        //最后一个空间了;
        if (!ValidBlock(allocBlock->Next))
        {
            LOG_DEBUG("last free will use[%d] idx[%d]", Pool->UsedCnt, iAllocBlock);
            iNewTailFree = iNewHead = NULL_BLOCK;
        }
        else
        {
            iNewHead = allocBlock->Next;
            MMemBlock *pNewHead = (MMemBlock *)MMemBlockIdx(Pool, iNewHead);
            pNewHead->Prev = NULL_BLOCK;
        }
    }

    Pool->HeadFree = iNewHead;
    Pool->TailFree = iNewTailFree;

    //插到已分配链表的头部
    Pool->UsedCnt++;

    allocBlock->Prev = 0;
    allocBlock->Next = Pool->HeadUsed;
    allocBlock->Valid = MMemBlockType::USED;

    //如果原本链表不空, 需要旧表头prev指向新表头
    if (ValidBlock(Pool->HeadUsed))
    {
        MMemBlock *pOldHeadUsed = (MMemBlock *)MMemBlockIdx(Pool, Pool->HeadUsed);
        pOldHeadUsed->Prev = iAllocBlock;
        Pool->HeadUsed = iAllocBlock;
    }
    else
    {
        Pool->HeadUsed = iAllocBlock;
        Pool->TailUsed = iAllocBlock;
    }

    if(idx)
    {
        *idx = iAllocBlock;
    }
    return allocBlock->Data;
}

int MMemPoolFree(MMemPool *Pool, void *Object)
{
    long long Offset = (long long)Object - ((long long)((MMemBlock *)0)->Data) - (long long)Pool->Data;

    //TOCO 往下的成员变量地址一定是高位吗 Offset小于0是否正常
    if (Offset < 0 || Offset % Pool->FullUnit != 0)
    {
        LOG_DEBUG("Offset[%llu]", Offset);
        return ERR_UNKNOW;
    }

    if (Pool->UsedCnt == 0 || Pool->HeadUsed == 0)
    {
        LOG_DEBUG("Free Fail UsedCnt[%d] HeadUsed[%d]", Pool->UsedCnt, Pool->HeadUsed);
        return ERR_UNKNOW;
    }

    //Block的idx从1算起,所以+1;
    int iFreeBlock = (Offset / Pool->FullUnit) + 1;
    MMemBlock *pFreeBlock = (MMemBlock *)MMemBlockIdx(Pool, iFreeBlock);

    if(!pFreeBlock || pFreeBlock->Valid!=MMemBlockType::USED)
    {
        LOG_DEBUG("Free Fail pFreeBlock[%d]", pFreeBlock?1:0);
        return ERR_UNKNOW;
    }

    int iNewHeadUsed = Pool->HeadUsed;
    int iNewTailUsed = Pool->TailUsed;

    int iPrev = pFreeBlock->Prev;
    int iNext = pFreeBlock->Next;
    if (ValidBlock(iPrev))
    {
        MMemBlock *pPrev = (MMemBlock *)MMemBlockIdx(Pool, iPrev);
        pPrev->Next = iNext;
        if (iNext == 0)
        {
            iNewTailUsed = iPrev;
        }
    }
    else
    {
        iNewHeadUsed = iNext;
    }
    if (iNext)
    {
        MMemBlock *pNext = (MMemBlock *)MMemBlockIdx(Pool, iNext);
        pNext->Prev = iPrev;
        if (iPrev == 0)
        {
            iNewHeadUsed = iNext;
        }
    }
    else
    {
        iNewTailUsed = iPrev;
    }

    Pool->UsedCnt--;
    Pool->HeadUsed = iNewHeadUsed;
    Pool->TailUsed = iNewTailUsed;

    //插入空闲链表头部
    pFreeBlock->Prev = 0;
    pFreeBlock->Next = Pool->HeadFree;

    if (ValidBlock(Pool->HeadFree))
    {
        MMemBlock *pHeadFree = (MMemBlock *)MMemBlockIdx(Pool, Pool->HeadFree);
        pHeadFree->Prev = iFreeBlock;
        Pool->HeadFree = iFreeBlock;
    }
    else
    {
        Pool->HeadFree = iFreeBlock;
        Pool->TailFree = iFreeBlock;
    }

    return 0;
}