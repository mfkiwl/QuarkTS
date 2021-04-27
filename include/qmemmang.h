/*This file is part of the QuarkTS distribution.*/
#ifndef QMEMMANG_H  
    #define  QMEMMANG_H

    #include "qtypes.h"

    #ifdef __cplusplus
    extern "C" {
    #endif


    #ifndef Q_MEMORY_MANAGER
        #define Q_MEMORY_MANAGER        ( 1 )
    #endif

    #if ( Q_MEMORY_MANAGER == 1 )

    #ifndef Q_BYTE_ALIGNMENT    
        #define Q_BYTE_ALIGNMENT        ( 8 )
    #endif

    #ifndef Q_DEFAULT_HEAP_SIZE    
        #define Q_DEFAULT_HEAP_SIZE     ( 512 )
    #endif

    #if ( Q_DEFAULT_HEAP_SIZE < 64 )
        #error Q_DEFAULT_HEAP_SIZE it is too small. Min(64).
    #endif

    #if ( ( Q_BYTE_ALIGNMENT != 1 ) && ( Q_BYTE_ALIGNMENT != 2 ) && ( Q_BYTE_ALIGNMENT != 4 ) && ( Q_BYTE_ALIGNMENT != 8 ) )
        #error Q_BYTE_ALIGNMENT value not allowed, use only 1,2,4 or 8(default).
    #endif    

    /* Linked list structure to connect the free blocks in order of their memory address. */
    typedef struct _qMemMang_BlockConnect_s{
        struct _qMemMang_BlockConnect_s *Next;      /*< Points to the next free block in the list*/
        size_t BlockSize;	                        /*< The size of the free block*/     
    }qMemMang_BlockConnect_t;

    /* Please don't access any members of this structure directly */
    typedef struct{
        struct _qMemMang_Pool_Private_s{
            qMemMang_BlockConnect_t *End;           /*< Points to the last block of the list. */
            qUINT8_t *PoolMemory;                   /*< Points to the beginning of the heap area statically allocated. */
            size_t PoolMemSize;                     /*< The size of the memory block pointed by "heap". */
            size_t FreeBytesRemaining;              /*< The number of free bytes in the heap. */
            size_t BlockAllocatedBit;               /*< A bit that is set when the block belongs to the application. Clearead when the block is part of the free space (only the MSB is used) */    
            qMemMang_BlockConnect_t Start;          /*< The first block of the heap. */
        }qPrivate;
    }qMemMang_Pool_t;
    
    qBool_t qMemMang_Pool_Setup( qMemMang_Pool_t * const mPool, void* Area, size_t Size );
    void qMemMang_Pool_Select( qMemMang_Pool_t * const mPool );
    size_t qMemMang_Get_FreeSize( qMemMang_Pool_t *mPool );    

    void* qMemMang_Allocate( qMemMang_Pool_t *mPool, size_t Size );
    void qMemMang_Free( qMemMang_Pool_t *mPool, void *ptr );

    void* qMalloc( size_t Size );
    void qFree(void *ptr);

    #endif

    #ifdef __cplusplus
    }
    #endif

#endif
