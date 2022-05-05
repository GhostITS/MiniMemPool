
enum MMemPoolErr
{
    ERR_OBJECT_SIZE_INVALID,
    ERR_MEMORY_ALIGN_FAIL,
    ERR_OUT_OF_MEM,
    ERR_UNKNOW,
};

#define BZERO(T) (memset(&T, 0, sizeof(T)));
#define MMemBlockHeadSize (sizeof(MMemBlock))
#define MMemPoolHeadSize (sizeof(MMemPool))
#define GetMemPoolSize(Unit, Count) (Count*(Unit+MMemBlockHeadSize) + MMemPoolHeadSize)

struct MMemPool
{
    int Valid;

    int Size;//总大小
    int Count;//内存池可分配的对象容量
    int Unit;//对象大小
    int FullUnit;//真正的MMemBlock大小

    int HeadUsed;//已分配对象链表head
    int TailUsed;//已分配对象链表tail
    int UsedCnt;//已分配对象数

    int HeadFree;//未分配内存链表head
    int TailFree;//未分配内存链表tail

    char Data[0];//MMemBlock起始地址
};

struct MMemBlock
{
    int Valid;
    int Prev;
    int Next;

    int Count;

    char Data[0];
};


// Unit=sizeof(T)
int MMemPoolInit(MMemPool *& Pool, void* MemAddr, int Unit, int Count);

int MMemPoolAttach(MMemPool *& Pool, void* MemAddr, int Unit, int Count);

void* MMemPoolGet(MMemPool* Pool, int idx);
//idx:  可用于MMemPoolGet, 
//      当使用共享内存做内存池时, 附加到共享内存后原对象地址失效，
//      用于找回对象的地址
void* MMemPoolAlloc(MMemPool* Pool, int* idx);

int MMemPoolFree(MMemPool* Pool, void* Object);



