//#pragma once
#include "itcdatastructs.h"

static void* itcDefaultAlloc(size_t size, void* argument);
static int itcDefaultFree(void* ptr, void* argument);
static void itcInitMemStorage(Track_MemStorage_t* storage, int block_size);
static void itcDestroyMemStorage(Track_MemStorage_t* storage);
static void itcGoNextMemBlock(Track_MemStorage_t * storage);
static void itcGrowSeq(Track_Seq_t *seq, int in_front_of);
static void itcFreeSeqBlock(Track_Seq_t *seq, int in_front_of);

// pointers to allocation functions, initially set to default
static void* p_cvAllocUserData = 0;

inline int  itcAlign(int size, int align)
{
	assert((align&(align-1))==0 && size<INT_MAX);
	return (size + align - 1) & -align;
}

inline void* itcAlignPtr( const void* ptr, int align )
{
	//align = 32;
	assert( (align & (align-1)) == 0 );
	return (void*)( ((size_t)ptr + align - 1) & ~(size_t)(align-1) );
}

inline int
	itcAlignLeft( int size, int align )
{
	return size & -align;
}



// default <malloc>
static void*
	itcDefaultAlloc( size_t size, void*  argument)
{
	//��������ڴ���Ϊ��ά���ڴ棬������Ϊ��ĳЩ�ܹ��ϣ�ֻ�б�ָ����������4,16�������ĵ�ַ���ܷ��ʣ������crash�����ͳ������
	char *ptr, *ptr0 = (char*)malloc(
		(size_t)(size + ITC_MALLOC_ALIGN*((size >= 4096) + 1) + sizeof(char*)));   //�������� ITC_MALLOC_ALIGN*((size >= 4096) + 1) + sizeof(char*)��С���ڴ�
																				   //ǰ����Ϊ�˶����Ԥ���Ŀռ䣬����Ϊ����һ��ָ����Ƭ�հ׿ռ��ָ��
																				   //ITC_MALLOC_ALIGN������32�� ��ʾʵ�ʴ洢���ݵ��׵�ַ��32�ı���
																				   //�������sizeof(char*)�������洢malloc���ص��ڴ��׵�ַ���Ա���DefaultFree�б���ȷ�ͷ�

	if( !ptr0 )
		return 0;

	// align the pointer
	ptr = (char*)itcAlignPtr(ptr0 + sizeof(char*) + 1, ITC_MALLOC_ALIGN);   //��ָ����뵽ITC_MALLOC_ALIGN��32bit��4���ֽڣ���ָ�������32���������������ַ��0��32��64�򷵻صľ��ǵ�ַ���������ַ��18�ͷ���32��36�ͷ���64��

	*(char**)(ptr - sizeof(char*)) = ptr0;	//��ptr0��¼��(ptr �C sizeof(char*))������Ŀռ��ַ����������ڴ����ʼλ�á�

	return ptr;//���ص�Ϊ���ÿռ��ַ
}

// default <free>
static int
	itcDefaultFree( void* ptr, void* argument)
{
	// Pointer must be aligned by CV_MALLOC_ALIGN
	if( ((size_t)ptr & (ITC_MALLOC_ALIGN-1)) != 0 )		//��ָ����뵽֮ǰ������(char*)λ�����ͷ��ڴ�
		return ITC_BADARG_ERR;
	free( *((char**)ptr - 1) );		//	*((char**)ptr-1)Ϊ֮ǰ������ڴ��ָ�룬��free�������ͷ���

	return ITC_OK;
}

// pointers to allocation functions, initially set to default
//static CvAllocFunc p_cvAlloc = icvDefaultAlloc;

void*  itcAlloc( size_t size )
{
	void* ptr = 0;

	//CV_FUNCNAME( "cvAlloc" );

	__BEGIN__;

	if ((size_t)size > ITC_MAX_ALLOC_SIZE)
		ITC_ERROR_DETAIL(ITC_StsOutOfRange, "");

	ptr = itcDefaultAlloc( size, p_cvAllocUserData );
	if( !ptr )
		ITC_ERROR_DETAIL(ITC_StsNoMem,"");

	__END__;

	return ptr;
}

void  itcFree_( void* ptr )
{
	//CV_FUNCNAME( "cvFree_" );

	__BEGIN__;

	if( ptr )
	{
		int status = itcDefaultFree( ptr, p_cvAllocUserData );
		if (status < 0)
			printf("Deallocation error\n");
	}

	__END__;
}

/****************************************************************************************\
*            Functions for manipulating memory storage - list of memory blocks           *
\****************************************************************************************/

/* initializes allocated storage */
static void itcInitMemStorage( Track_MemStorage_t* storage, int block_size )
{
	//CV_FUNCNAME( "icvInitMemStorage " );

	__BEGIN__;

	if (!storage)
		ITC_ERROR_DETAIL(ITC_StsNullPtr, "");

	if( block_size <= 0 )
		block_size = ITC_STORAGE_BLOCK_SIZE;//block_size==0������СĬ��Ϊ65408

	block_size = itcAlign( block_size, ITC_STRUCT_ALIGN );//������ռ��С����Ϊ8�ֽڵı���
	assert( sizeof(Track_MemBlock_t) % ITC_STRUCT_ALIGN == 0 );

	memset( storage, 0, sizeof( *storage ));	//��storage��ʼ��
	storage->signature = ITC_STORAGE_MAGIC_VAL;
	storage->block_size = block_size;

	__END__;
}

/* creates root memory storage */
/*
	ΪItcMemStorage�����ڴ棬block_sizeΪ��С��0ΪĬ��ֵ��Ĭ�ϴ�СΪ65408
*/
Track_MemStorage_t* itcCreateMemStorage(int block_size)//��block_size==0��������СΪĬ��ֵ
{
	Track_MemStorage_t *storage = 0;

	storage = (Track_MemStorage_t *)itcAlloc(sizeof(Track_MemStorage_t));

	itcInitMemStorage( storage, block_size );

	return storage;
}

/* creates child memory storage */
/*
	�Ӹ�storage����ȡ���ڴ��
	���ڴ��ĺô��ǣ�������̬����ʱ�����Խ����ݷ����ӿ���в������õ��Ľ�����ظ��鲢�ͷŸ��ֿ飬
	������������е���ʱ���������ӿ�ͬʱ�ͷţ�������ռ�ø���Ŀռ䡣
*/
Track_MemStorage_t* itcCreateChildMemStorage(Track_MemStorage_t * parent)
{
	Track_MemStorage_t *storage = 0;
	//CV_FUNCNAME( "cvCreateChildMemStorage" );

	__BEGIN__;

	if (!parent)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");

	//CV_CALL( storage = cvCreateMemStorage(parent->block_size));
	storage = itcCreateMemStorage(parent->block_size);
	storage->parent = parent;

	__END__;

	/*if (cvGetErrStatus() < 0)
		itcFree(&storage);*/

	return storage;
}

/* releases all blocks of the storage (or returns them to parent if any) */
static void itcDestroyMemStorage(Track_MemStorage_t* storage)
{
	//CV_FUNCNAME( "icvDestroyMemStorage" );

	__BEGIN__;

	int k = 0;

	Track_MemBlock_t *block;
	Track_MemBlock_t *dst_top = 0;

	if (!storage)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");

	if( storage->parent )
		dst_top = storage->parent->top;

	for( block = storage->bottom; block != 0; k++ )
	{
		Track_MemBlock_t *temp = block;

		block = block->next;
		if( storage->parent )
		{
			if( dst_top )
			{
				temp->prev = dst_top;
				temp->next = dst_top->next;
				if( temp->next )
					temp->next->prev = temp;
				dst_top = dst_top->next = temp;//���ϵ������ǽ��и�����ڴ��е�block�Żظ����top֮��
			}
			else
			{
				dst_top = storage->parent->bottom = storage->parent->top = temp;
				temp->prev = temp->next = 0;
				storage->free_space = storage->block_size - sizeof( *temp );//�ͷ�storage�еĿռ仹��parent
			}
		}
		//���û��parent������ͷŵ�storage��ÿ��block���ڴ�
		else
		{
			itcFree( &temp );
			//itcFree_(&temp);
		}
	}

	storage->top = storage->bottom = 0;
	storage->free_space = 0;

	__END__;
}

/* releases memory storage */
/*
	�ͷ�storage�������ڴ�
*/
void
	itcReleaseMemStorage( Track_MemStorage_t** storage )
{
	Track_MemStorage_t *st;
	//CV_FUNCNAME( "cvReleaseMemStorage" );

	__BEGIN__;

	if( !storage )
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");

	//Ϊʲô��ֱ���ͷ��ڴ棿
	st = *storage;
	*storage = 0;

	if( st )
	{
		itcDestroyMemStorage(st);
		itcFree(&st);
		//CV_CALL( icvDestroyMemStorage( st ));
		//cvFree( &st );
	}

	__END__;
}

/* clears memory storage (returns blocks to the parent if any) */
/*
	���storage���ڴ棬����Ӹ���̳е��ڴ��򻹸�����
*/
void
itcClearMemStorage(Track_MemStorage_t * storage)
{
	//CV_FUNCNAME( "cvClearMemStorage" );

	__BEGIN__;

	if( !storage )
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");

	if( storage->parent )
	{
		itcDestroyMemStorage( storage );//������parent��ѿռ仹��parent
	}
	else
	{
		//û��parent����ո�storage��ֻ����գ������ͷ��ڴ�
		storage->top = storage->bottom;
		storage->free_space = storage->bottom ? storage->block_size - sizeof(Track_MemBlock_t) : 0;
	}

	__END__;
}

/* moves stack pointer to next block.
   If no blocks, allocate new one and link it to the storage */
/*
	itcGoNextMemBlock����top֮�����һ��MemBlock��������top������ӵ�MemBlock������free_space��ά��һ��MemBlock��˫������
*/
static void
itcGoNextMemBlock( Track_MemStorage_t * storage )
{
    //CV_FUNCNAME( "icvGoNextMemBlock" );
    
    __BEGIN__;
    
	if (!storage)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");

    if( !storage->top || !storage->top->next )
    {
		Track_MemBlock_t *block;

        if( !(storage->parent) )
        {
            //CV_CALL( block = (CvMemBlock *)cvAlloc( storage->block_size ));
			block = (Track_MemBlock_t *)itcAlloc(storage->block_size);
        }
        else
        {
            Track_MemStorage_t *parent = storage->parent;
            Track_MemStoragePos_t parent_pos;

            itcSaveMemStoragePos( parent, &parent_pos );
			itcGoNextMemBlock(parent);
            //CV_CALL( icvGoNextMemBlock( parent ));

            block = parent->top;
            itcRestoreMemStoragePos( parent, &parent_pos );

            if( block == parent->top )  /* the single allocated block */
            {
                assert( parent->bottom == block );
                parent->top = parent->bottom = 0;
                parent->free_space = 0;
            }
            else
            {
                /* cut the block from the parent's list of blocks */
                parent->top->next = block->next;
                if( block->next )
                    block->next->prev = parent->top;
            }
        }

        /* link block */
        block->next = 0;
        block->prev = storage->top;

        if( storage->top )
            storage->top->next = block;
        else
            storage->top = storage->bottom = block;
    }

    if( storage->top->next )
        storage->top = storage->top->next;
    storage->free_space = storage->block_size - sizeof(Track_MemBlock_t);
    assert( storage->free_space % ITC_STRUCT_ALIGN == 0 );

    __END__;
}

/* remembers memory storage position */
/*
	ͨ���˺���������storage��topָ���λ�õ�ItcMemStoragePos��
*/
void
	itcSaveMemStoragePos( const Track_MemStorage_t * storage, Track_MemStoragePos_t * pos )
{
	//CV_FUNCNAME( "cvSaveMemStoragePos" );

	__BEGIN__;

	if( !storage || !pos )
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");

	pos->top = storage->top;
	pos->free_space = storage->free_space;

	__END__;
}

/* restores memory storage position */
/*
	ͨ���˺�����ItcMemStoragePos�лָ�֮ǰ�����topλ�á�
*/
void
itcRestoreMemStoragePos( Track_MemStorage_t * storage, Track_MemStoragePos_t * pos )
{
   // CV_FUNCNAME( "cvRestoreMemStoragePos" );

    __BEGIN__;

    if( !storage || !pos )
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
    if( pos->free_space > storage->block_size )
		ITC_ERROR_DETAIL(ITC_StsBadSize,"");
        //CV_ERROR( CV_StsBadSize, "" );

    /*
    // this breaks icvGoNextMemBlock, so comment it off for now
    if( storage->parent && (!pos->top || pos->top->next) )
    {
        CvMemBlock* save_bottom;
        if( !pos->top )
            save_bottom = 0;
        else
        {
            save_bottom = storage->bottom;
            storage->bottom = pos->top->next;
            pos->top->next = 0;
            storage->bottom->prev = 0;
        }
        icvDestroyMemStorage( storage );
        storage->bottom = save_bottom;
    }*/

    storage->top = pos->top;
    storage->free_space = pos->free_space;

    if( !storage->top )
    {
        storage->top = storage->bottom;
        storage->free_space = storage->top ? storage->block_size - sizeof(Track_MemBlock_t) : 0;
    }

    __END__;
}

/* Allocates continuous buffer of the specified size in the storage */
/*
	�÷��������ѷ����storage��top�����size��С�Ŀռ�
*/
void*
	itcMemStorageAlloc( Track_MemStorage_t* storage, size_t size )
{
	char *ptr = 0;

	//CV_FUNCNAME( "cvMemStorageAlloc" );

	__BEGIN__;

	if( !storage )
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR( CV_StsNullPtr, "NULL storage pointer" );

	if( size > INT_MAX )
		ITC_ERROR_DETAIL(ITC_StsOutOfRange,"");
		//CV_ERROR( CV_StsOutOfRange, "Too large memory block is requested" );

	assert( storage->free_space % ITC_STRUCT_ALIGN == 0 );

	if( (size_t)storage->free_space < size )
	{
		size_t max_free_space = itcAlignLeft(storage->block_size - sizeof(Track_MemBlock_t), ITC_STRUCT_ALIGN);// ���������ʣ��ռ�����ܹ��ж��
		if (max_free_space < size)// ���Ҫ����Ŀռ�ܴ���߲��������Ǹ�����
			//CV_ERROR( CV_StsOutOfRange, "requested size is negative or too big" );
			ITC_ERROR_DETAIL(ITC_StsOutOfRange, "requested size is negative or too big");

		//CV_CALL( icvGoNextMemBlock( storage ));
		itcGoNextMemBlock(storage); //����һ���µ�MemBlock
	}

	ptr = ITC_FREE_PTR(storage);// �꺯���ҵ���ǰfree space���׵�ַ
	assert( (size_t)ptr % ITC_STRUCT_ALIGN == 0 );
	storage->free_space = itcAlignLeft(storage->free_space - (int)size, ITC_STRUCT_ALIGN );//����Ҫ�����ڴ���䣬ֻҪ����free_space����

	__END__;

	return ptr;
}

/****************************************************************************************\
*                               Sequence implementation                                  *
\****************************************************************************************/

/* creates empty sequence */
/*
	seq_flags Ϊ������Ԫ�ص�����
	header_size Ϊͷ�Ĵ�С��������ڵ���ItcSeq�Ĵ�С
	elem_size Ϊ������Ԫ�صĴ�С
	storage Ϊ���е��ڴ�ռ�

	�������������Ҫ���������е��ڴ�飬����ʹ��itcReleaseMemStorage�������ڴ���ͷ��ڴ�ķ�������ɶԳ��֣�
	�����ͷŶ�������Զ�����ڴ棬ֱ���ͷŻ�����ڴ����������������

	ItcSeq *runs = itcCreateSeq(ITC_SEQ_ELTYPE_POINT, sizeof(ItcSeq), sizeof(ItcPoint), storage);
*/
Track_Seq_t *
	itcCreateSeq( int seq_flags, int header_size, int elem_size, Track_MemStorage_t * storage )
{
		Track_Seq_t *seq = 0;

	//CV_FUNCNAME( "cvCreateSeq" );

	__BEGIN__;

	if (!storage)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
	if (header_size < (int)sizeof(Track_Seq_t) || elem_size <= 0)
		ITC_ERROR_DETAIL(ITC_StsBadSize,"");

	/* allocate sequence header */
	//CV_CALL( seq = (CvSeq*)cvMemStorageAlloc( storage, header_size ));
	seq = (Track_Seq_t*)itcMemStorageAlloc(storage, header_size);
	memset( seq, 0, header_size );

	seq->header_size = header_size;
	seq->flags = (seq_flags & ~ITC_MAGIC_MASK) | ITC_SEQ_MAGIC_VAL;
	{
		int elemtype = ITC_MAT_TYPE(seq_flags);
		int typesize = ITC_ELEM_SIZE(elemtype);

		if (elemtype != ITC_SEQ_ELTYPE_GENERIC &&
			typesize != 0 && typesize != elem_size)
			ITC_ERROR_DETAIL(ITC_StsBadSize, "Specified element size doesn't match to the size of the specified element type \
			(try to use 0 for element type)");
			//CV_ERROR( CV_StsBadSize,
			//"Specified element size doesn't match to the size of the specified element type "
			//"(try to use 0 for element type)" );
	}
	seq->elem_size = elem_size;
	seq->storage = storage;

	//CV_CALL( cvSetSeqBlockSize( seq, (1 << 10)/elem_size ));
	itcSetSeqBlockSize(seq, (1 << 10)/elem_size );
	// �趨delta_elems����ʾ��Seq�еĿռ䲻��ʱ�����Ӷ��ռ䣬
	//�����趨��delta_elems��((1<< 10)/elem_size)��cvAlignLeft(seq->storage->block_size- sizeof(CvMemBlock) -sizeof(CvSeqBlock), 
	//CV_STRUCT_ALIGN)����Сֵ

	__END__;

	return seq;
}

/* adjusts <delta_elems> field of sequence. It determines how much the sequence
   grows if there are no free space inside the sequence buffers */
void
itcSetSeqBlockSize(Track_Seq_t *seq, int delta_elements)
{
    int elem_size;
    int useful_block_size;

    //CV_FUNCNAME( "cvSetSeqBlockSize" );

    __BEGIN__;
	
	if (!seq || !seq->storage)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
	if (delta_elements < 0)
		ITC_ERROR_DETAIL(ITC_StsBadSize,"");

    useful_block_size = itcAlignLeft(seq->storage->block_size - sizeof(Track_MemBlock_t) -
                                    sizeof(Track_SeqBlock_t), ITC_STRUCT_ALIGN);
    elem_size = seq->elem_size;

    if( delta_elements == 0 )
    {
        delta_elements = (1 << 10) / elem_size;
        delta_elements = ITC_MAX( delta_elements, 1 );
    }
    if( delta_elements * elem_size > useful_block_size )
    {
        delta_elements = useful_block_size / elem_size;
		if (delta_elements == 0)
			ITC_ERROR_DETAIL(ITC_StsBadSize, "Storage block size is too small to fit the sequence elements");
            //CV_ERROR( CV_StsOutOfRange, "Storage block size is too small "
                                       // "to fit the sequence elements" );
    }

    seq->delta_elems = delta_elements;

    __END__;
}

/* finds sequence element by its index */
/*
	*seqΪĿ������
	indexΪ����������Ҫ��ȡ��Ԫ�ص�����

	ItcPoint *p = (ItcPoint*)itcGetSeqElem(runs, i);
	��������֧�ָ�����-1��Ϊ���һ��Ԫ�أ�-2��Ϊ�����ڶ���Ԫ�ء�
*/
char*
	itcGetSeqElem( const Track_Seq_t *seq, int index )
{
	Track_SeqBlock_t *block;
	int count, total = seq->total;

	if( (unsigned)index >= (unsigned)total )
	{
		index += index < 0 ? total : 0;
		index -= index >= total ? total : 0;
		if( (unsigned)index >= (unsigned)total )
			return 0;
	}

	block = seq->first;
	if( index + index <= total )
	{
		while( index >= (count = block->count) )
		{
			block = block->next;
			index -= count;
		}
	}
	else
	{
		do
		{
			block = block->prev;
			total -= block->count;
		}
		while( index < total );
		index -= total;
	}

	return block->data + index * seq->elem_size;
}

/* calculates index of sequence element */
/*
	seqΪĿ������
	_elementΪԪ�ص�ִ��
	block��Ϊ�գ��ǿ�ʱ���Ű�����ָԪ�صĿ�ĵ�ַ

	����ֵΪ��ӦԪ�ص����������ظ������ʾ�޸�Ԫ��
*/
int
	itcSeqElemIdx( const Track_Seq_t* seq, const void* _element, Track_SeqBlock_t** _block )
{
	const char *element = (const char *)_element;
	int elem_size;
	int id = -1;
	Track_SeqBlock_t *first_block;
	Track_SeqBlock_t *block;

	//CV_FUNCNAME( "cvSeqElemIdx" );

	__BEGIN__;

	if (!seq || !element)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");

	block = first_block = seq->first;
	elem_size = seq->elem_size;

	for( ;; )
	{
		if( (unsigned)(element - block->data) < (unsigned) (block->count * elem_size) )
		{
			if( _block )
				*_block = block;
			if( elem_size <= ITC_SHIFT_TAB_MAX && (id = itcPower2ShiftTab[elem_size - 1]) >= 0 )
				id = (int)((size_t)(element - block->data) >> id);
			else
				id = (int)((size_t)(element - block->data) / elem_size);
			id += block->start_index - seq->first->start_index;
			break;
		}
		block = block->next;
		if( block == first_block )
			break;
	}

	__END__;

	return id;
}

/* pushes element to the sequence */
/*
	seqΪĿ������
	*elementΪ��Ҫ�������е�Ԫ��

	ItcPoint pt1 = itcPoint(i, i);
	itcSeqPush(runs, &pt1);

	�����Ҫ���䲢ʹ�õĿռ�ϴ���ʹ��itcStartWriteSeq����غ������ٶȸ���
*/
char*
	itcSeqPush( Track_Seq_t *seq, void *element )
{
	char *ptr = 0;
	size_t elem_size;

	//CV_FUNCNAME( "cvSeqPush" );

	__BEGIN__;

	if( !seq )
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR( CV_StsNullPtr, "" );

	elem_size = seq->elem_size;
	ptr = seq->ptr;

	if( ptr >= seq->block_max )
	{
		//CV_CALL( icvGrowSeq( seq, 0 ));
		itcGrowSeq( seq, 0 );
		ptr = seq->ptr;
		assert( ptr + elem_size <= seq->block_max /*&& ptr == seq->block_min */  );
	}

	if( element )
		ITC_MEMCPY_AUTO(ptr, element, elem_size);
	seq->first->prev->count++;
	seq->total++;
	seq->ptr = ptr + elem_size;

	__END__;

	return ptr;
} //20

/* pops the last element out of the sequence */
/*
	seq ΪĿ������
	*element Ϊ��Ҫ������Ԫ�����͵�ָ��

	ItcPoint temp ;
	itcSeqPop(runs, &temp);
*/
void itcSeqPop(Track_Seq_t *seq, void *element)
{
	char *ptr;
	int elem_size;

	//CV_FUNCNAME("cvSeqPop");

	__BEGIN__;

	if (!seq)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(ITC_StsNullPtr, "");
	if (seq->total <= 0)
		ITC_ERROR_DETAIL(ITC_StsBadSize,"");
		//CV_ERROR(ITC_StsBadSize, "");

	elem_size = seq->elem_size;
	seq->ptr = ptr = seq->ptr - elem_size;

	if (element)
		ITC_MEMCPY_AUTO(element, ptr, elem_size);
	seq->ptr = ptr;
	seq->total--;

	if (--(seq->first->prev->count) == 0)
	{
		itcFreeSeqBlock(seq, 0);
		assert(seq->ptr == seq->block_max);
	}

	__END__;
}

/* the function allocates space for at least one more sequence element.
   if there are free sequence blocks (seq->free_blocks != 0),
   they are reused, otherwise the space is allocated in the storage */
static void
itcGrowSeq( Track_Seq_t *seq, int in_front_of )
{
    //CV_FUNCNAME( "icvGrowSeq" );

    __BEGIN__;

    Track_SeqBlock_t *block;

	if (!seq)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
        //CV_ERROR( CV_StsNullPtr, "" );
    block = seq->free_blocks;

    if( !block )
    {
        int elem_size = seq->elem_size;
        int delta_elems = seq->delta_elems;
        Track_MemStorage_t *storage = seq->storage;

        if( seq->total >= delta_elems*4 )
            itcSetSeqBlockSize( seq, delta_elems*2 );

        if( !storage )
			ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
            //CV_ERROR( CV_StsNullPtr, "The sequence has NULL storage pointer" );

        /* if there is a free space just after last allocated block
           and it's big enough then enlarge the last block
           (this can happen only if the new block is added to the end of sequence */
        if( (unsigned)(ITC_FREE_PTR(storage) - seq->block_max) < ITC_STRUCT_ALIGN &&
            storage->free_space >= seq->elem_size && !in_front_of )
        {
            int delta = storage->free_space / elem_size;

            delta = ITC_MIN( delta, delta_elems ) * elem_size;
            seq->block_max += delta;
            storage->free_space = itcAlignLeft((int)(((char*)storage->top + storage->block_size) -
                                              seq->block_max), ITC_STRUCT_ALIGN );
            EXIT;
			
        }
        else
        {
            int delta = elem_size * delta_elems + (int)itcAlign(sizeof(Track_SeqBlock_t), ITC_STRUCT_ALIGN);

            /* try to allocate <delta_elements> elements */
            if( storage->free_space < delta )
            {
                int small_block_size = ITC_MAX(1, delta_elems/3)*elem_size +
					(int)itcAlign(sizeof(Track_SeqBlock_t), ITC_STRUCT_ALIGN);
                /* try to allocate smaller part */
                if( storage->free_space >= small_block_size + ITC_STRUCT_ALIGN )
                {
					delta = (storage->free_space - (int)itcAlign(sizeof(Track_SeqBlock_t), ITC_STRUCT_ALIGN)) / seq->elem_size;
					delta = delta*seq->elem_size + (int)itcAlign(sizeof(Track_SeqBlock_t), ITC_STRUCT_ALIGN);
                }
                else
                {
                    //CV_CALL( icvGoNextMemBlock( storage ));
					itcGoNextMemBlock(storage);
                    assert( storage->free_space >= delta );
                }
            }

            //CV_CALL( block = (CvSeqBlock*)cvMemStorageAlloc( storage, delta ));
			block = (Track_SeqBlock_t*)itcMemStorageAlloc(storage, delta);
            block->data = (char*)itcAlignPtr( block + 1, ITC_STRUCT_ALIGN );
			block->count = delta - (int)itcAlign(sizeof(Track_SeqBlock_t), ITC_STRUCT_ALIGN);
            block->prev = block->next = 0;
        }
    }
    else
    {
        seq->free_blocks = block->next;
    }

    if( !(seq->first) )
    {
        seq->first = block;
        block->prev = block->next = block;
    }
    else
    {
        block->prev = seq->first->prev;
        block->next = seq->first;
        block->prev->next = block->next->prev = block;
    }

    /* for free blocks the <count> field means total number of bytes in the block.
       And for used blocks it means a current number of sequence
       elements in the block */
    assert( block->count % seq->elem_size == 0 && block->count > 0 );

    if( !in_front_of )
    {
        seq->ptr = block->data;
        seq->block_max = block->data + block->count;
        block->start_index = block == block->prev ? 0 :
            block->prev->start_index + block->prev->count;
    }
    else
    {
        int delta = block->count / seq->elem_size;
        block->data += block->count;

        if( block != block->prev )
        {
            assert( seq->first->start_index == 0 );
            seq->first = block;
        }
        else
        {
            seq->block_max = seq->ptr = block->data;
        }

        block->start_index = 0;

        for( ;; )
        {
            block->start_index += delta;
            block = block->next;
            if( block == seq->first )
                break;
        }
    }

    block->count = 0;

    __END__;
}

/* recycles a sequence block for the further use */
static void itcFreeSeqBlock(Track_Seq_t *seq, int in_front_of)
{
	/*CV_FUNCNAME( "icvFreeSeqBlock" );*/

	__BEGIN__;

	Track_SeqBlock_t *block = seq->first;

	assert((in_front_of ? block : block->prev)->count == 0);

	if (block == block->prev)  /* single block case */
	{
		block->count = (int)(seq->block_max - block->data) + block->start_index * seq->elem_size;
		block->data = seq->block_max - block->count;
		seq->first = 0;
		seq->ptr = seq->block_max = 0;
		seq->total = 0;
	}
	else
	{
		if (!in_front_of)
		{
			block = block->prev;
			assert(seq->ptr == block->data);

			block->count = (int)(seq->block_max - seq->ptr);
			seq->block_max = seq->ptr = block->prev->data +
				block->prev->count * seq->elem_size;
		}
		else
		{
			int delta = block->start_index;

			block->count = delta * seq->elem_size;
			block->data -= block->count;

			/* update start indices of sequence blocks */
			for (;;)
			{
				block->start_index -= delta;
				block = block->next;
				if (block == seq->first)
					break;
			}

			seq->first = block->next;
		}

		block->prev->next = block->next;
		block->next->prev = block->prev;
	}

	assert(block->count > 0 && block->count % seq->elem_size == 0);
	block->next = seq->free_blocks;
	seq->free_blocks = block;

	__END__;
}

/* pushes element to the front of the sequence */
/*
	ItcPoint temp = itcPoint(101,102);
	itcSeqPushFront(runs, &temp);
*/
char* itcSeqPushFront(Track_Seq_t *seq, void *element)
{
	char* ptr = 0;
	int elem_size;
	Track_SeqBlock_t *block;

	//CV_FUNCNAME("cvSeqPushFront");

	__BEGIN__;

	if (!seq)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(CV_StsNullPtr, "");

	elem_size = seq->elem_size;
	block = seq->first;

	if (!block || block->start_index == 0)
	{
		//CV_CALL(icvGrowSeq(seq, 1));
		itcGrowSeq(seq, 1);

		block = seq->first;
		assert(block->start_index > 0);
	}

	ptr = block->data -= elem_size;

	if (element)
		ITC_MEMCPY_AUTO(ptr, element, elem_size);
	block->count++;
	block->start_index--;
	seq->total++;

	__END__;

	return ptr;
}

/* pulls out the first element of the sequence */
/*
	�÷�ͬitcSeqPop
*/
void itcSeqPopFront(Track_Seq_t *seq, void *element)
{
	int elem_size;
	Track_SeqBlock_t *block;

	//CV_FUNCNAME("cvSeqPopFront");

	__BEGIN__;

	if (!seq)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(CV_StsNullPtr, "");
	if (seq->total <= 0)
		ITC_ERROR_DETAIL(ITC_StsBadSize,"");
		//CV_ERROR(CV_StsBadSize, "");

	elem_size = seq->elem_size;
	block = seq->first;

	if (element)
		ITC_MEMCPY_AUTO(element, block->data, elem_size);
	block->data += elem_size;
	block->start_index++;
	seq->total--;

	if (--(block->count) == 0)
	{
		itcFreeSeqBlock(seq, 1);
	}

	__END__;
}

/* inserts new element in the middle of the sequence */
/*
	seq ΪĿ������
	before_index ΪҪ�����λ�ã������Գ������е�Ԫ������Ŀ
	*element Ϊ����Ԫ�ص�����

*/
char* itcSeqInsert(Track_Seq_t *seq, int before_index, void *element)
{
	int elem_size;
	int block_size;
	Track_SeqBlock_t *block;
	int delta_index;
	int total;
	char* ret_ptr = 0;

	//CV_FUNCNAME("cvSeqInsert");

	__BEGIN__;

	if (!seq)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(CV_StsNullPtr, "");

	total = seq->total;
	before_index += before_index < 0 ? total : 0;
	before_index -= before_index > total ? total : 0;

	if ((unsigned)before_index > (unsigned)total)
		ITC_ERROR_DETAIL(ITC_StsOutOfRange, "Invalid index");
		//CV_ERROR(CV_StsOutOfRange, "");

	if (before_index == total)
	{
		//CV_CALL(ret_ptr = cvSeqPush(seq, element));
		ret_ptr = itcSeqPush(seq, element);
	}
	else if (before_index == 0)
	{
		//CV_CALL(ret_ptr = cvSeqPushFront(seq, element));
		ret_ptr = itcSeqPushFront(seq, element);
	}
	else
	{
		elem_size = seq->elem_size;

		if (before_index >= total >> 1)
		{
			char *ptr = seq->ptr + elem_size;

			if (ptr > seq->block_max)
			{
				//CV_CALL(icvGrowSeq(seq, 0));
				itcGrowSeq(seq, 0);

				ptr = seq->ptr + elem_size;
				assert(ptr <= seq->block_max);
			}

			delta_index = seq->first->start_index;
			block = seq->first->prev;
			block->count++;
			block_size = (int)(ptr - block->data);

			while (before_index < block->start_index - delta_index)
			{
				Track_SeqBlock_t *prev_block = block->prev;

				memmove(block->data + elem_size, block->data, block_size - elem_size);
				block_size = prev_block->count * elem_size;
				memcpy(block->data, prev_block->data + block_size - elem_size, elem_size);
				block = prev_block;

				/* check that we don't fall in the infinite loop */
				assert(block != seq->first->prev);
			}

			before_index = (before_index - block->start_index + delta_index) * elem_size;
			memmove(block->data + before_index + elem_size, block->data + before_index,
				block_size - before_index - elem_size);

			ret_ptr = block->data + before_index;

			if (element)
				memcpy(ret_ptr, element, elem_size);
			seq->ptr = ptr;
		}
		else
		{
			block = seq->first;

			if (block->start_index == 0)
			{
				//CV_CALL(icvGrowSeq(seq, 1));
				itcGrowSeq(seq, 1);

				block = seq->first;
			}

			delta_index = block->start_index;
			block->count++;
			block->start_index--;
			block->data -= elem_size;

			while (before_index > block->start_index - delta_index + block->count)
			{
				Track_SeqBlock_t *next_block = block->next;

				block_size = block->count * elem_size;
				memmove(block->data, block->data + elem_size, block_size - elem_size);
				memcpy(block->data + block_size - elem_size, next_block->data, elem_size);
				block = next_block;
				/* check that we don't fall in the infinite loop */
				assert(block != seq->first);
			}

			before_index = (before_index - block->start_index + delta_index) * elem_size;
			memmove(block->data, block->data + elem_size, before_index - elem_size);

			ret_ptr = block->data + before_index - elem_size;

			if (element)
				memcpy(ret_ptr, element, elem_size);
		}

		seq->total = total + 1;
	}

	__END__;

	return ret_ptr;
}

/* removes element from the sequence */
/*
	�÷��ο�itcSeqPop��indexΪҪ�Ƴ���Ԫ������
*/
void itcSeqRemove(Track_Seq_t *seq, int index)
{
	char *ptr;
	int elem_size;
	int block_size;
	Track_SeqBlock_t *block;
	int delta_index;
	int total, front = 0;

	//CV_FUNCNAME("cvSeqRemove");

	__BEGIN__;

	if (!seq)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(CV_StsNullPtr, "");

	total = seq->total;

	index += index < 0 ? total : 0;
	index -= index >= total ? total : 0;

	if ((unsigned)index >= (unsigned)total)
		ITC_ERROR_DETAIL(ITC_StsOutOfRange, "Invalid index");
		//CV_ERROR(CV_StsOutOfRange, "Invalid index");

	if (index == total - 1)
	{
		itcSeqPop(seq, 0);
	}
	else if (index == 0)
	{
		itcSeqPopFront(seq, 0);
	}
	else
	{
		block = seq->first;
		elem_size = seq->elem_size;
		delta_index = block->start_index;
		while (block->start_index - delta_index + block->count <= index)
			block = block->next;

		ptr = block->data + (index - block->start_index + delta_index) * elem_size;

		front = index < total >> 1;
		if (!front)
		{
			block_size = block->count * elem_size - (int)(ptr - block->data);

			while (block != seq->first->prev)  /* while not the last block */
			{
				Track_SeqBlock_t *next_block = block->next;

				memmove(ptr, ptr + elem_size, block_size - elem_size);
				memcpy(ptr + block_size - elem_size, next_block->data, elem_size);
				block = next_block;
				ptr = block->data;
				block_size = block->count * elem_size;
			}

			memmove(ptr, ptr + elem_size, block_size - elem_size);
			seq->ptr -= elem_size;
		}
		else
		{
			ptr += elem_size;
			block_size = (int)(ptr - block->data);

			while (block != seq->first)
			{
				Track_SeqBlock_t *prev_block = block->prev;

				memmove(block->data + elem_size, block->data, block_size - elem_size);
				block_size = prev_block->count * elem_size;
				memcpy(block->data, prev_block->data + block_size - elem_size, elem_size);
				block = prev_block;
			}

			memmove(block->data + elem_size, block->data, block_size - elem_size);
			block->data += elem_size;
			block->start_index++;
		}

		seq->total = total - 1;
		if (--block->count == 0)
			itcFreeSeqBlock(seq, front);
	}

	__END__;
}

/****************************************************************************************\
*                             Sequence Writer implementation                             *
\****************************************************************************************/

/* initializes sequence writer */
/*
	��writer��seq�󶨣�ͨ����ITC_WRITE_SEQ_ELEM������seq������д��ֵ
	ITC_WRITE_SEQ_ELEM(elem,writer);
*/
void itcStartAppendToSeq(Track_Seq_t *seq, Track_SeqWriter_t * writer)
{
	//CV_FUNCNAME("cvStartAppendToSeq");

	__BEGIN__;

	if (!seq || !writer)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(CV_StsNullPtr, "");

	memset(writer, 0, sizeof(*writer));
	writer->header_size = sizeof(Track_SeqWriter_t);

	writer->seq = seq;
	writer->block = seq->first ? seq->first->prev : 0;
	writer->ptr = seq->ptr;
	writer->block_max = seq->block_max;

	__END__;
}


/* initializes sequence writer */
/*
	�½�һ������seq��������writer��
*/
void itcStartWriteSeq(int seq_flags, int header_size,
int elem_size, Track_MemStorage_t * storage, Track_SeqWriter_t * writer)
{
	Track_Seq_t *seq = 0;

	//CV_FUNCNAME("cvStartWriteSeq");

	__BEGIN__;

	if (!storage || !writer)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(CV_StsNullPtr, "");

	//CV_CALL(seq = cvCreateSeq(seq_flags, header_size, elem_size, storage));
	seq = itcCreateSeq(seq_flags, header_size, elem_size, storage);
	itcStartAppendToSeq(seq, writer);

	__END__;
}

/* updates sequence header */
void itcFlushSeqWriter(Track_SeqWriter_t * writer)
{
	Track_Seq_t *seq = 0;

	//CV_FUNCNAME("cvFlushSeqWriter");

	__BEGIN__;

	if (!writer)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(CV_StsNullPtr, "");

	seq = writer->seq;
	seq->ptr = writer->ptr;

	if (writer->block)
	{
		int total = 0;
		Track_SeqBlock_t *first_block = writer->seq->first;
		Track_SeqBlock_t *block = first_block;

		writer->block->count = (int)((writer->ptr - writer->block->data) / seq->elem_size);
		assert(writer->block->count > 0);

		do
		{
			total += block->count;
			block = block->next;
		} while (block != first_block);

		writer->seq->total = total;
	}

	__END__;
}


/* calls icvFlushSeqWriter and finishes writing process */
/*
	�ر�д�����У���ʱ�޷�ͨ������seq��д��Ԫ��
*/
Track_Seq_t * itcEndWriteSeq(Track_SeqWriter_t * writer)
{
	Track_Seq_t *seq = 0;

	//CV_FUNCNAME("cvEndWriteSeq");

	__BEGIN__;

	if (!writer)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(CV_StsNullPtr, "");

	//CV_CALL(cvFlushSeqWriter(writer));
	itcFlushSeqWriter(writer);
	seq = writer->seq;

	/* truncate the last block */
	if (writer->block && writer->seq->storage)
	{
		Track_MemStorage_t *storage = seq->storage;
		char *storage_block_max = (char *)storage->top + storage->block_size;

		assert(writer->block->count > 0);

		if ((unsigned)((storage_block_max - storage->free_space)
			- seq->block_max) < ITC_STRUCT_ALIGN)
		{
			storage->free_space = itcAlignLeft((int)(storage_block_max - seq->ptr), ITC_STRUCT_ALIGN);
			seq->block_max = seq->ptr;
		}
	}

	writer->ptr = 0;

	__END__;

	return seq;
}

/* creates new sequence block */
void itcCreateSeqBlock(Track_SeqWriter_t * writer)
{
	//CV_FUNCNAME("cvCreateSeqBlock");

	__BEGIN__;

	Track_Seq_t *seq;

	if (!writer || !writer->seq)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(CV_StsNullPtr, "");

	seq = writer->seq;

	itcFlushSeqWriter(writer);

	//CV_CALL(icvGrowSeq(seq, 0));
	itcGrowSeq(seq, 0);

	writer->block = seq->first->prev;
	writer->ptr = seq->ptr;
	writer->block_max = seq->block_max;

	__END__;
}

/****************************************************************************************\
*                               Sequence Reader implementation                           *
\****************************************************************************************/

/* initializes sequence reader */
/*
	��seq��reader�󶨣�ͨ����ITC_READ_SEQ_ELEM������������seq�ж�ȡ����Ԫ�ء�
	reverse�����Ƿ������ȡ��0�����β��ͷ����0λ��ͷ��β
*/
void itcStartReadSeq(const Track_Seq_t *seq, Track_SeqReader_t * reader, int reverse)
{
	Track_SeqBlock_t *first_block;
	Track_SeqBlock_t *last_block;

	//CV_FUNCNAME("cvStartReadSeq");

	if (reader)
	{
		reader->seq = 0;
		reader->block = 0;
		reader->ptr = reader->block_max = reader->block_min = 0;
	}

	__BEGIN__;

	if (!seq || !reader)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(CV_StsNullPtr, "");

	reader->header_size = sizeof(Track_SeqReader_t);
	reader->seq = (Track_Seq_t*)seq;

	first_block = seq->first;

	if (first_block)
	{
		last_block = first_block->prev;
		reader->ptr = first_block->data;
		reader->prev_elem = ITC_GET_LAST_ELEM(seq, last_block);
		reader->delta_index = seq->first->start_index;

		if (reverse)
		{
			char *temp = reader->ptr;

			reader->ptr = reader->prev_elem;
			reader->prev_elem = temp;

			reader->block = last_block;
		}
		else
		{
			reader->block = first_block;
		}

		reader->block_min = reader->block->data;
		reader->block_max = reader->block_min + reader->block->count * seq->elem_size;
	}
	else
	{
		reader->delta_index = 0;
		reader->block = 0;

		reader->ptr = reader->prev_elem = reader->block_min = reader->block_max = 0;
	}

	__END__;
}

/* changes the current reading block to the previous or to the next */
void itcChangeSeqBlock(void* _reader, int direction)
{
	//CV_FUNCNAME("cvChangeSeqBlock");

	__BEGIN__;

	Track_SeqReader_t* reader = (Track_SeqReader_t*)_reader;

	if (!reader)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
	//CV_ERROR(CV_StsNullPtr, "");

	if (direction > 0)
	{
		reader->block = reader->block->next;
		reader->ptr = reader->block->data;
	}
	else
	{
		reader->block = reader->block->prev;
		reader->ptr = ITC_GET_LAST_ELEM(reader->seq, reader->block);
	}
	reader->block_min = reader->block->data;
	reader->block_max = reader->block_min + reader->block->count * reader->seq->elem_size;

	__END__;
}

/* returns the current reader position */
/*
	����cvGetSeqReaderPos�Żص�ǰreader��λ����(0��reader->seq->total-1֮��)
*/
int itcGetSeqReaderPos(Track_SeqReader_t* reader)
{
	int elem_size;
	int index = -1;

	//CV_FUNCNAME("cvGetSeqReaderPos");

	__BEGIN__;

	if (!reader || !reader->ptr)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(CV_StsNullPtr, "");

	elem_size = reader->seq->elem_size;
	if (elem_size <= ITC_SHIFT_TAB_MAX && (index = itcPower2ShiftTab[elem_size - 1]) >= 0)
		index = (int)((reader->ptr - reader->block_min) >> index);
	else
		index = (int)((reader->ptr - reader->block_min) / elem_size);

	index += reader->block->start_index - reader->delta_index;

	__END__;

	return index;
}

/* sets reader position to given absolute or relative
(relatively to the current one) position */
/*
	����
	����itcSetSeqReaderPos�ƶ���ȡ����ָ����λ��
	��ʽ
	void itcSetSeqReaderPos(CvSeqReader* reader,int  index, int is_relative = 0);
	����
	reader ��ȡ����״̬
	index ������λ��.���ʹ�þ���λ��,��ʵ��λ��Ϊindex % reader->seq->total.
	is_relative �����Ϊ0,������(index)ֵΪ���λ��
	˵��
	����itcSetSeqReaderPos����ȡ����λ���ƶ�������λ��,������ڵ�ǰλ�õ����λ����.
*/
void itcSetSeqReaderPos(Track_SeqReader_t* reader, int index, int is_relative)
{
	//CV_FUNCNAME("cvSetSeqReaderPos");

	__BEGIN__;

	Track_SeqBlock_t *block;
	int elem_size, count, total;

	if (!reader || !reader->seq)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(CV_StsNullPtr, "");

	total = reader->seq->total;
	elem_size = reader->seq->elem_size;

	if (!is_relative)
	{
		if (index < 0)
		{
			if (index < -total)
				ITC_ERROR_DETAIL(ITC_StsOutOfRange,"");
				//CV_ERROR(CV_StsOutOfRange, "");
			index += total;
		}
		else if (index >= total)
		{
			index -= total;
			if (index >= total)
				ITC_ERROR_DETAIL(ITC_StsOutOfRange,"");
				//CV_ERROR(CV_StsOutOfRange, "");
		}

		block = reader->seq->first;
		if (index >= (count = block->count))
		{
			if (index + index <= total)
			{
				do
				{
					block = block->next;
					index -= count;
				} while (index >= (count = block->count));
			}
			else
			{
				do
				{
					block = block->prev;
					total -= block->count;
				} while (index < total);
				index -= total;
			}
		}
		reader->ptr = block->data + index * elem_size;
		if (reader->block != block)
		{
			reader->block = block;
			reader->block_min = block->data;
			reader->block_max = block->data + block->count * elem_size;
		}
	}
	else
	{
		char* ptr = reader->ptr;
		index *= elem_size;
		block = reader->block;

		if (index > 0)
		{
			while (ptr + index >= reader->block_max)
			{
				int delta = (int)(reader->block_max - ptr);
				index -= delta;
				reader->block = block = block->next;
				reader->block_min = ptr = block->data;
				reader->block_max = block->data + block->count*elem_size;
			}
			reader->ptr = ptr + index;
		}
		else
		{
			while (ptr + index < reader->block_min)
			{
				int delta = (int)(ptr - reader->block_min);
				index += delta;
				reader->block = block = block->prev;
				reader->block_min = block->data;
				reader->block_max = ptr = block->data + block->count*elem_size;
			}
			reader->ptr = ptr + index;
		}
	}

	__END__;
}


/* adds several elements to the end or in the beginning of sequence */
/*
	��������������Ԫ�أ�*_elementΪ�׵�ַ��countΪ������frontΪ��ʶ��0��Ϊ������ͷ����ӣ���0Ϊ������β�����

	itcSeqPushMulti(runs, &temp, 5, ITC_FRONT);
*/
void itcSeqPushMulti(Track_Seq_t *seq, void *_elements, int count, int front)
{
	char *elements = (char *)_elements;

	//CV_FUNCNAME("cvSeqPushMulti");

	__BEGIN__;
	int elem_size;

	if (!seq)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(CV_StsNullPtr, "NULL sequence pointer");
	if (count < 0)
		ITC_ERROR_DETAIL(ITC_StsBadSize,"");
		//CV_ERROR(CV_StsBadSize, "number of removed elements is negative");

	elem_size = seq->elem_size;

	if (!front)
	{
		while (count > 0)
		{
			int delta = (int)((seq->block_max - seq->ptr) / elem_size);

			delta = ITC_MIN(delta, count);
			if (delta > 0)
			{
				seq->first->prev->count += delta;
				seq->total += delta;
				count -= delta;
				delta *= elem_size;
				if (elements)
				{
					memcpy(seq->ptr, elements, delta);
					elements += delta;
				}
				seq->ptr += delta;
			}

			if (count > 0)
				itcGrowSeq(seq, 0);
				//CV_CALL(icvGrowSeq(seq, 0));
		}
	}
	else
	{
		Track_SeqBlock_t* block = seq->first;

		while (count > 0)
		{
			int delta;

			if (!block || block->start_index == 0)
			{
				itcGrowSeq(seq, 1);
				//CV_CALL(icvGrowSeq(seq, 1));

				block = seq->first;
				assert(block->start_index > 0);
			}

			delta = ITC_MIN(block->start_index, count);
			count -= delta;
			block->start_index -= delta;
			block->count += delta;
			seq->total += delta;
			delta *= elem_size;
			block->data -= delta;

			if (elements)
				memcpy(block->data, elements + count*elem_size, delta);
		}
	}

	__END__;
}

/* removes several elements from the end of sequence */
/*
	�÷��ο�itcSeqPushMulti
*/
void itcSeqPopMulti(Track_Seq_t *seq, void *_elements, int count, int front)
{
	char *elements = (char *)_elements;

	//CV_FUNCNAME("cvSeqPopMulti");

	__BEGIN__;

	if (!seq)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
		//CV_ERROR(CV_StsNullPtr, "NULL sequence pointer");
	if (count < 0)
		ITC_ERROR_DETAIL(ITC_StsBadSize,"");
		//CV_ERROR(CV_StsBadSize, "number of removed elements is negative");

	count = ITC_MIN(count, seq->total);

	if (!front)
	{
		if (elements)
			elements += count * seq->elem_size;

		while (count > 0)
		{
			int delta = seq->first->prev->count;

			delta = ITC_MIN(delta, count);
			assert(delta > 0);

			seq->first->prev->count -= delta;
			seq->total -= delta;
			count -= delta;
			delta *= seq->elem_size;
			seq->ptr -= delta;

			if (elements)
			{
				elements -= delta;
				memcpy(elements, seq->ptr, delta);
			}

			if (seq->first->prev->count == 0)
				itcFreeSeqBlock(seq, 0);
				//icvFreeSeqBlock(seq, 0);
		}
	}
	else
	{
		while (count > 0)
		{
			int delta = seq->first->count;

			delta = ITC_MIN(delta, count);
			assert(delta > 0);

			seq->first->count -= delta;
			seq->total -= delta;
			count -= delta;
			seq->first->start_index += delta;
			delta *= seq->elem_size;

			if (elements)
			{
				memcpy(elements, seq->first->data, delta);
				elements += delta;
			}

			seq->first->data += delta;
			if (seq->first->count == 0)
				itcFreeSeqBlock(seq, 1);
				//icvFreeSeqBlock(seq, 1);
		}
	}

	__END__;
}

/* removes all elements from the sequence */
/*
	��������е�Ԫ�أ��������ͷ�storage���ڴ档

	����������storage�е��ڴ棬������Ӱ�������е�Ԫ�أ������ԭ�򣬻��ڿ�
*/
void itcClearSeq(Track_Seq_t *seq)
{
	//CV_FUNCNAME("cvClearSeq");

	__BEGIN__;

	if (!seq)
		ITC_ERROR_DETAIL(ITC_StsNullPtr,"");
	itcSeqPopMulti(seq, 0, seq->total,0);

	__END__;
}



///////////////////////////////////������inline����///////////////////////////////////////
int itcRound(double a)
{
	return (int)(a + 0.5);
}

int itcFloor(double val)
{
	int temp = itcRound(val);
	Track_32suf_t diff;
	diff.f = (float)(val - temp);
	return temp - (diff.i < 0);
}

Track_Rect_t  itcRect(int x, int y, int width, int height)
{
	Track_Rect_t r;

	r.x = x;
	r.y = y;
	r.width = width;
	r.height = height;

	return r;
}

Track_Point_t itcPoint(int x, int y)
{
	Track_Point_t p;
	p.x = x;
	p.y = y;

	return p;
}

Track_Point2D32f_t  itcPoint2D32f(double x, double y)
{
	Track_Point2D32f_t p;

	p.x = (float)x;
	p.y = (float)y;

	return p;
}

Track_Point2D32f_t  itcPointTo32f(Track_Point_t point)
{
	return itcPoint2D32f((float)point.x, (float)point.y);
}

Track_Point_t  itcPointFrom32f(Track_Point2D32f_t point)
{
	Track_Point_t ipt;
	ipt.x = itcRound(point.x);
	ipt.y = itcRound(point.y);

	return ipt;
}

Track_Point3D32f_t  itcPoint3D32f(double x, double y, double z)
{
	Track_Point3D32f_t p;

	p.x = (float)x;
	p.y = (float)y;
	p.z = (float)z;

	return p;
}

Track_Point3D64f_t  itcPoint3D64f(double x, double y, double z)
{
	Track_Point3D64f_t p;

	p.x = x;
	p.y = y;
	p.z = z;

	return p;
}

Track_Point2D64f_t  itcPoint2D64f(double x, double y)
{
	Track_Point2D64f_t p;

	p.x = x;
	p.y = y;

	return p;
}

Track_Size_t itcSize(int width, int height)
{
	Track_Size_t s;
	s.width = width;
	s.height = height;
	return s;
}