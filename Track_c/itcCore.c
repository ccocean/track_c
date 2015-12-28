#include "itcCore.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

Itc_Mat_t itc_mat( int rows, int cols, int type, void* data)
{
	Itc_Mat_t m;

	type = ITC_MAT_TYPE(type);					//ֻ��ȡ��12λ����
	m.type = ITC_MAT_MAGIC_VAL | ITC_MAT_CONT_FLAG | type;
	m.cols = cols;
	m.rows = rows;
	m.step = m.cols*ITC_ELEM_SIZE(type);
	m.data.ptr = (itc_uchar*)data;
	m.refcount = NULL;
	m.hdr_refcount = 0;

	return m;
}

Itc_Mat_t*	itc_create_mat( int height, int width, int type )
{
	Itc_Mat_t* arr = itc_create_matHeader( height, width, type );
	if (arr == NULL)
	{
		itc_release_mat(&arr);
		return NULL;
	}
	itc_size_t step, total_size;
	Itc_Mat_t* mat = (Itc_Mat_t*)arr;
	itc_int64 _total_size;
	step = mat->step;
	
	if( mat->rows == 0 || mat->cols == 0 )
	{	
		return arr;
	}

	if( mat->data.ptr != 0 )
		free( mat->data.ptr);//"Data is already allocated"

	if( step == 0 )
		step = ITC_ELEM_SIZE(mat->type)*mat->cols;

	_total_size = (itc_int64)step*mat->rows + sizeof(int)+ITC_MALLOC_ALIGN;	//int�����ڱ���ͳ�Ƽ����ģ�ITC_MALLOC_ALIGN�����ڴ�Ķ���
	total_size = (itc_size_t)_total_size;				//����ϵͳ��ͬ����size_t���ͽ�ȡ��ϵͳ�ܷ���Ŀռ��С
	if (_total_size != (itc_int64)total_size)			//�������ȣ�˵���Ѿ����
	{
		ITC_ERROR_("Too big buffer is allocated");	//����Ŀռ䳬����ǰϵͳ��Ѱַ��Χ}
		itc_release_mat(&arr);
		return arr;
	}

	mat->refcount = (int*)itcAlloc((itc_size_t)total_size);
	if (mat->refcount == NULL)
	{
		itc_release_mat(&arr);
		return NULL;
	}
	memset(mat->refcount, 0, (itc_size_t)total_size);	//��ʼ��Ϊ0
	mat->data.ptr = (itc_uchar*)(mat->refcount + 1);
	mat->data.ptr = (itc_uchar*)(((itc_size_t)mat->data.ptr + ITC_MALLOC_ALIGN - 1) &~(itc_size_t)(ITC_MALLOC_ALIGN - 1));//���뵽ITC_MALLOC_ALIGN����λ������˵��ַ��110��ITC_MALLOC_ALIGN=16����ô�Ͱѵ�ַ���뵽112�������ַ��120����ô�Ͷ��뵽128��
	*mat->refcount = 1;

	return arr;
}


static void iicvCheckHuge( Itc_Mat_t* pMat )
{
	//�����Ҫ����Ŀռ��Ƿ����
	if ((itc_int64)pMat->step*pMat->rows > INT_MAX)	//�������������С
		pMat->type &= ~ITC_MAT_CONT_FLAG;		//����Ϊ������
}

Itc_Mat_t*	itc_create_matHeader( int rows, int cols, int type )
{
	type = ITC_MAT_TYPE(type);

	if( rows < 0 || cols <= 0 )
		ITC_ERROR_("Non-positive width or height");				//

	int min_step = ITC_ELEM_SIZE(type)*cols;
	if( min_step <= 0 )
		ITC_ERROR_("Invalid matrix type");						//

	Itc_Mat_t* pMat = (Itc_Mat_t*)itcAlloc(sizeof(*pMat));
	if (pMat == NULL)
	{
		return NULL;
	}
	memset(pMat, 0, sizeof(*pMat));								//

	pMat->step = min_step;
	pMat->type = ITC_MAT_MAGIC_VAL | type | ITC_MAT_CONT_FLAG;	//ITC_MAT_MAGIC_VALΪMat�ṹ�ı�־�������ж��Ƿ���mat�ṹ
	pMat->rows = rows;
	pMat->cols = cols;
	pMat->data.ptr = NULL;
	pMat->refcount = NULL;
	pMat->hdr_refcount = 1;

	iicvCheckHuge(pMat);
	return pMat;
}

Itc_Mat_t*	itc_init_matHeader( Itc_Mat_t* arr, int rows, int cols, int type, void* data, int step )
{
	if( !arr )
		ITC_ERROR_("����ͷΪ�գ�");

	if( (unsigned)ITC_MAT_DEPTH(type) > ITC_DEPTH_MAX )
		ITC_ERROR_("������ʹ���");

	if( rows < 0 || cols <= 0 )
		ITC_ERROR_("Non-positive cols or rows" );// "Non-positive cols or rows" 

	type = ITC_MAT_TYPE( type );
	arr->type = type | ITC_MAT_MAGIC_VAL;
	arr->rows = rows;
	arr->cols = cols;
	arr->data.ptr = (itc_uchar*)data;
	arr->refcount = 0;
	arr->hdr_refcount = 0;

	int pix_size = ITC_ELEM_SIZE(type);
	int min_step = arr->cols*pix_size;

	if( step != ITC_AUTOSTEP && step != 0 )
	{
		if( step < min_step )
			ITC_ERROR_("���벽������");
		arr->step = step;
	}
	else
	{
		arr->step = min_step;
	}

	arr->type = ITC_MAT_MAGIC_VAL | type |
		(arr->rows == 1 || arr->step == min_step ? ITC_MAT_CONT_FLAG : 0);

	iicvCheckHuge( arr );
	return arr;
}

void itc_release_mat(Itc_Mat_t** arr)
{
	Itc_Mat_t *mat=*arr;
	if (mat!=NULL)
	{
		mat->data.ptr = NULL;
		if (mat->refcount != NULL && --*mat->refcount == 0)//���ü���Ϊ0ʱ���ͷ������ڴ�
			itcFree_(mat->refcount);
		mat->refcount = NULL;
		itcFree_(mat);
		mat = NULL;
	}
}


#define ICV_DEF_BIN_ARI_OP_2D( __op__, name, type, worktype, cast_macro )   \
	static void  name													    \
	(itc_uchar* src1, int step1, itc_uchar* src2, int step2, \
	itc_uchar* dst, int dstep, Track_Size_t size)								\
{                                                                           \
	int i = 0;																\
	int j = 0;																\
	type *srct1;															\
	type *srct2;															\
	type *dstt;																\
	for (i = 0; i < size.height; i++)                                       \
	{																		\
		srct1 = (type*)src1;												\
		srct2 = (type*)src2;												\
		dstt = (type*)dst;													\
		for (j = 0; j < size.width; j++)									\
		{                                                                   \
			worktype t0 = __op__((srct1)[j], (srct2)[j]);					\
			(dstt)[j] = cast_macro(t0);										\
		}																	\
		src1 += step1;														\
		src2 += step2;														\
		dst += dstep;														\
	}		                                                        		\
}

//__op__�ǲ������ͣ�
#define ICV_DEF_BIN_ARI_ALL( __op__, name )											\
	ICV_DEF_BIN_ARI_OP_2D(__op__, icv##name##_8u_C1R, itc_uchar, int, ITC_CAST_8U)	\
	ICV_DEF_BIN_ARI_OP_2D(__op__, icv##name##_8s_C1R, char, int, ITC_CAST_8S)		\
	ICV_DEF_BIN_ARI_OP_2D(__op__, icv##name##_16u_C1R, itc_ushort, int, ITC_CAST_16U)\
	ICV_DEF_BIN_ARI_OP_2D(__op__, icv##name##_16s_C1R, short, int, ITC_CAST_16S)	\
	ICV_DEF_BIN_ARI_OP_2D(__op__, icv##name##_32s_C1R, int, int, ITC_CAST_32S)		\
	ICV_DEF_BIN_ARI_OP_2D(__op__, icv##name##_32f_C1R, float, float, ITC_CAST_32F)	\
	ICV_DEF_BIN_ARI_OP_2D(__op__, icv##name##_64f_C1R, double, double, ITC_CAST_64F)

#undef ITC_SUB_R
#define ITC_SUB_R(a,b) ((a) - (b))						//����sub����
ICV_DEF_BIN_ARI_ALL(ITC_SUB_R, Sub)						//����sub�����ĺ���

void itc_sub_mat(Itc_Mat_t* src1, Itc_Mat_t* src2, Itc_Mat_t* dst)
{
	if( !ITC_ARE_TYPES_EQ( src1, src2 ) || !ITC_ARE_TYPES_EQ( src1, dst ))//��������Ƿ�һ��
		ITC_ERROR_("�������Ͳ�һ��");

	if( !ITC_ARE_SIZES_EQ( src1, src2 ) || !ITC_ARE_SIZES_EQ( src1, dst ))//����С�Ƿ�һ��
		ITC_ERROR_("�����С��һ��");

	int type = ITC_MAT_TYPE(src1->type);	//����
	int depth = ITC_MAT_DEPTH(type);		//���
	int cn = ITC_MAT_CN(type);				//ͨ����

	Track_Size_t sizeMat;
	sizeMat.width=src1->cols * cn;
	sizeMat.height=src1->rows;

	switch (depth)
	{
	case ITC_8U:
		icvSub_8u_C1R(src1->data.ptr,src1->step,src2->data.ptr,src2->step,dst->data.ptr,dst->step,sizeMat);
		break;
	case ITC_8S:
		icvSub_8s_C1R(src1->data.ptr,src1->step,src2->data.ptr,src2->step,dst->data.ptr,dst->step,sizeMat);
		break;
	case ITC_16U:
		icvSub_16u_C1R(src1->data.ptr,src1->step,src2->data.ptr,src2->step,dst->data.ptr,dst->step,sizeMat);
		break;
	case ITC_16S:
		icvSub_16s_C1R(src1->data.ptr,src1->step,src2->data.ptr,src2->step,dst->data.ptr,dst->step,sizeMat);
		break;
	case ITC_32S:
		icvSub_32s_C1R(src1->data.ptr,src1->step,src2->data.ptr,src2->step,dst->data.ptr,dst->step,sizeMat);
		break;
	case ITC_32F:
		icvSub_32f_C1R(src1->data.ptr,src1->step,src2->data.ptr,src2->step,dst->data.ptr,dst->step,sizeMat);
		break;
	case ITC_64F:
		icvSub_64f_C1R(src1->data.ptr,src1->step,src2->data.ptr,src2->step,dst->data.ptr,dst->step,sizeMat);
		break;
	default:
		break;
	}
}

#define MHT_TRACH_RESERVE_THRESHOLD 227  //ֻ�������ڸ�ֵ�ĵ㣬������Ϊ0
void track_update_MHI(Itc_Mat_t* src1, Itc_Mat_t* src2, Itc_Mat_t* mhi, int diffThreshold, Itc_Mat_t* maskT, int Threshold)
{
	if (!ITC_ARE_TYPES_EQ(src1, src2) || !ITC_ARE_TYPES_EQ(src1, mhi))//��������Ƿ�һ��
		ITC_ERROR_("�������Ͳ�һ��");

	if (!ITC_ARE_SIZES_EQ(src1, src2) || !ITC_ARE_SIZES_EQ(src1, mhi))//����С�Ƿ�һ��
		ITC_ERROR_("�����С��һ��");

	int type = ITC_MAT_TYPE(src1->type);	//����
	int depth = ITC_MAT_DEPTH(type);		//���
	if (depth != ITC_8U)
		ITC_ERROR_("������Ȳ���uchar");
	if( ITC_MAT_CN(type)!=1 )				//ͨ����
		ITC_ERROR_("ͨ������Ϊ1");

	Track_Size_t sizeMat;
	sizeMat.width = src1->cols;
	sizeMat.height = src1->rows;

	int i = 0;
	int j = 0;
	itc_uchar *qsrc1 = src1->data.ptr;
	itc_uchar *qsrc2 = src2->data.ptr;
	itc_uchar *qmhi = mhi->data.ptr;
	if (maskT == NULL)
	{
		for (i = 0; i < sizeMat.height; i++)
		{
			for (j = 0; j < sizeMat.width; j++)
			{
				int k = abs((int)(qsrc1[j] - qsrc2[j]));
				if ( k > diffThreshold )
				{
					qmhi[j] = 255;
				}
				else
				{
					//mhi����ȡС��0��ֵ
					qmhi[j] = qmhi[j] > MHT_TRACH_RESERVE_THRESHOLD ? qmhi[j] : 1;
					//qmhi[j] = ITC_IMAX(qmhi[j], 1);
					qmhi[j]--;
				}
			}
			qsrc1 += src1->step;
			qsrc2 += src2->step;
			qmhi += mhi->step;
		}
	}
	else
	{
		itc_uchar *qmask = maskT->data.ptr;
		//��������Ҫʹ���ܱ߽綼Ϊ0�����ڽ����������
		qsrc1 += src1->step;
		qsrc2 += src2->step;
		qmhi += mhi->step;
		qmask += maskT->step;
		if (!ITC_ARE_TYPES_EQ(src1, maskT))//��������Ƿ�һ��
			ITC_ERROR_("�������Ͳ�һ��");
		if (!ITC_ARE_SIZES_EQ(src1, maskT))//����С�Ƿ�һ��
			ITC_ERROR_("�����С��һ��");
		//��Ե������
		sizeMat.height--;
		sizeMat.width--;
		for (i = 1; i < sizeMat.height; i++)
		{
			for (j = 1; j < sizeMat.width; j++)
			{
				int k = abs((int)(qsrc1[j] - qsrc2[j]));
				if (k > diffThreshold)
				{
					qmask[j] = 1;//����һ����ֵ������
					qmhi[j] = 255;
				}
				else
				{
					//mhi����ȡС��0��ֵ
					qmask[j] = (qmhi[j] > Threshold) & 1;//����һ����ֵ������
					qmhi[j] = qmhi[j] > MHT_TRACH_RESERVE_THRESHOLD ? qmhi[j] : 1;
					//qmhi[j] = ITC_IMAX(qmhi[j], 1);
					qmhi[j]--;
				}
			}
			qsrc1 += src1->step;
			qsrc2 += src2->step;
			qmhi += mhi->step;
			qmask += maskT->step;
		}
	}
}

/* initializes 8-element array for fast access to 3x3 neighborhood of a pixel */
#define  ITC_INIT_3X3_DELTAS( deltas, step, nch )				\
	((deltas)[0] = (nch), (deltas)[1] = -(step)+(nch),			\
	(deltas)[2] = -(step), (deltas)[3] = -(step)-(nch),			\
	(deltas)[4] = -(nch), (deltas)[5] = (step)-(nch),			\
	(deltas)[6] = (step), (deltas)[7] = (step)+(nch))

static const Track_Point_t icvCodeDeltas[8] =
{ { 1, 0 }, { 1, -1 }, { 0, -1 }, { -1, -1 }, { -1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 } };

static int itcFetchContourEx(char*		ptr,
int							step,
Track_Point_t				pt,
Track_Seq_t*				contour,
int							nbd)
{
	int         deltas[16];
	char        *i0 = ptr, *i1, *i3, *i4;
	Track_Rect_t rect;
	int         prev_s = -1, s, s_end;
	Track_SeqWriter_t writer;

	assert(1 < nbd && nbd < 128);

	/* initialize local state */
	ITC_INIT_3X3_DELTAS(deltas, step, 1);
	memcpy(deltas + 8, deltas, 8 * sizeof(deltas[0]));

	/* initialize writer */
	itcStartAppendToSeq((Track_Seq_t*)contour, &writer);

	rect.x = rect.width = pt.x;
	rect.y = rect.height = pt.y;

	s_end = s = (contour->flags) ? 0 : 4;	//�ж��Ƿ��ǿ�,�ǿ׾ʹ��ұ߿�ʼɨ�����������ʹ���߿�ʼɨ

	do
	{
		s = (s - 1) & 7;				//˳ʱ���ұ�Ե
		i1 = i0 + deltas[s];
		if (*i1 != 0)
			break;
	} while (s != s_end);

	if (s == s_end)						//ɨ��һȦû���ҵ�������Ե��˵���ǵ�һһ����
	{
		*i0 = (char)(nbd | 0x80);		//��char�������λ��Ϊ1,��Ϊ����Ҳ��һ���ұ�Ե��
		//ITC_WRITE_SEQ_ELEM(pt, writer);//�����
	}
	else
	{
		i3 = i0;
		prev_s = s ^ 4;

		//���ٱ�Ե
		for (;;)
		{
			s_end = s;
			for (;;)
			{
				i4 = i3 + deltas[++s];	//��ʱ��ɨ��8����
				if (*i4 != 0)
					break;
			}
			s &= 7;

			//����ұ�Ե
			if ((unsigned)(s - 1) < (unsigned)s_end)
			{
				*i3 = (char)(nbd | 0x80);//���ұ�Ե���ֵ��λ����Ϊ1����Ϊ-128 = 0x80 =1000 0000
			}
			else if (*i3 == 1)
			{
				*i3 = (char)nbd;//���ұ�Եֵ��λ����Ϊ0
			}

			if (s != prev_s)//ѹ��ͬ����ĵ�
			{
				ITC_WRITE_SEQ_ELEM(pt, writer);//�����
			}

			if (s != prev_s)
			{
				//���±߽�
				if (pt.x < rect.x)
					rect.x = pt.x;
				else if (pt.x > rect.width)
					rect.width = pt.x;

				if (pt.y < rect.y)
					rect.y = pt.y;
				else if (pt.y > rect.height)
					rect.height = pt.y;
			}

			prev_s = s;
			pt.x += icvCodeDeltas[s].x;//icvCodeDeltas��Ԥ�����úõ�ƫ���������ݷ���s
			pt.y += icvCodeDeltas[s].y;

			if (i4 == i0 && i3 == i1)  break;

			i3 = i4;
			s = (s + 4) & 7;
		}                       /* end of border following loop */
	}

	rect.width -= rect.x - 1;
	rect.height -= rect.y - 1;
	((Track_Contour_t*)(contour))->rect = rect;
	itcEndWriteSeq(&writer);

	return 1;
}

int track_find_contours(Itc_Mat_t* src, Track_Contour_t** pContour, Track_MemStorage_t*  storage)
{
	if (src == NULL || pContour == NULL || storage == NULL)
	{
		return 0;
	}
	int step = src->step;
	//char *img0 = (char*)(src->data.ptr);
	char *img = (char*)(src->data.ptr + step);
	
	int width = src->cols - 1;
	int height = src->rows - 1;

	//Track_Point_t lnbd = itcPoint(0, 1);	//��¼��һ��ɨ�赽��Ե���λ��,�������������ṹ�ģ���ʱ�ò���
	int x = 1;						//ɨ����ʼλ��
	int y = 1;
	int prev = img[x - 1];

	int count = 0;
	for (; y < height; y++, img += step)		//�У���������
	{
		for (x = 1; x < width; x++)				//�У���������
		{
			int p = img[x];
			if (p != prev)						//�ҵ�һ����Ե��
			{
				int is_hole = 0;
				Track_Point_t origin;				//ɨ�����
				if (!(prev == 0 && p == 1))		//���û���ҵ���������0->1��ע�����������ͱ�Ե����Ϊ���������ұ�ԵҲ��1->0��Ҫ��׽������֣�
				{
					//����Ƿ��ǿ�
					if (p != 0 || prev < 1)		//��Ӧ����(p=0 && prev>=1)
						goto resume_scan;		//����ɨ����
					is_hole = 1;				//���ÿױ�־
				}
				
				Track_Seq_t* contour = itcCreateSeq(0, sizeof(Track_Contour_t), sizeof(Track_Point_t), storage);
				contour->flags = is_hole;
				//���ٱ�Ե�����
				origin.y = y;
				origin.x = x - is_hole;			//���������������ǿף���Ե��ɨ����㣩��ȡ��Ϊ0������
				itcFetchContourEx(img + x - is_hole, step, itcPoint(origin.x, origin.y), contour, 126);
				//lnbd.x = x - is_hole;			//��ǰɨ�赽��Ե���λ�ã������´�ɨ���ж��Ƿ��а�����ϵ

				if (((Track_Contour_t*)(contour))->rect.width > 1 &&
					((Track_Contour_t*)(contour))->rect.height > 1)
				{
					if ((*pContour) == NULL)
					{
						(*pContour) = (Track_Contour_t*)contour;
						(*pContour)->h_prev = (*pContour)->h_next = contour;
					}
					else
					{
						//����
						contour->h_next = (*pContour)->h_next;
						(*pContour)->h_next = contour;
						contour->h_prev = (*pContour)->h_prev;
						count++;
					}
				}
			resume_scan:
				prev = img[x];		//����ֱ�ӵ���p,��ΪitcFetchContourEx��ı䵱ǰɨ����ĵ�
				//if (prev & -2)		//ֻ������֪�ı�Ե
				//{
				//	lnbd.x = x;		//��¼��ǰɨ�赽��Ե���λ�ã�������һɨ��ʹ�ã��жϰ�����ϵ
				//}
			}
		}
		//lnbd.x = 0;
		//lnbd.y = y + 1;
		prev = 0;
	}
	return count;
}

int track_intersect_rect(Track_Rect_t *rectA, Track_Rect_t *rectB, int expand_dis)
{
	if (rectA == NULL || rectB == NULL)
	{
		return 0;
	}

	int x1_A = rectA->x;
	int y1_A = rectA->y;
	int x2_A = rectA->x + rectA->width;
	int y2_A = rectA->y + rectA->height;

	int x1_B = rectB->x;
	int y1_B = rectB->y;
	int x2_B = rectB->x + rectB->width;
	int y2_B = rectB->y + rectB->height;

	int x_left = ITC_IMIN(x1_A, x1_B);
	int y_top = ITC_IMIN(y1_A, y1_B);
	int x_right = ITC_IMAX(x2_A, x2_B);
	int y_bottom = ITC_IMAX(y2_A, y2_B);
	//�ϲ���Ĵ�С
	int width = x_right - x_left;	
	int height = y_bottom - y_top;

	if (expand_dis>0)
	{
		//expand_dis����0��ʾֻҪ�������εı߽����С��expand_dis��������������1
		if ((rectA->width + rectB->width + expand_dis < width)
			|| (rectA->height + rectB->height + expand_dis < height))
		{
			return 0;
		}
	}
	else
	{
		if (x1_A > x2_B)
			return 0;
		if (y1_A > y2_B)
			return 0;
		if (x2_A < x1_B)
			return 0;
		if (y2_A < y1_B)
			return 0;

		if (expand_dis<0)
		{
			//��expand_dis���д���
			expand_dis = -expand_dis;
			expand_dis = ITC_IMIN(expand_dis, rectA->width - (rectA->width >> 2));
			expand_dis = ITC_IMIN(expand_dis, rectA->height - (rectA->height >> 2));
			expand_dis = ITC_IMIN(expand_dis, rectB->width - (rectB->width >> 2));
			expand_dis = ITC_IMIN(expand_dis, rectB->height - (rectB->height >> 2));
			//���ཻ���ֵĴ�С
			int width_int = ITC_IMIN(x2_A, x2_B) - ITC_IMAX(x1_A, x1_B);
			int height_int = ITC_IMIN(y2_A, y2_B) - ITC_IMAX(y1_A, y1_B);
			//���ཻ����С��expand_dis���򷵻�0
			if (expand_dis > width_int || expand_dis > height_int)
				return 0;
		}
	}
	//�ϲ���rectA
	rectA->x = x_left;
	rectA->y = y_top;
	rectA->width = width;
	rectA->height = height;
	return 1;
}

int track_filtrate_contours(Track_Contour_t** pContour, int size_Threshold, Track_Rect_t *rect_arr)
{
	if (rect_arr == NULL || *pContour == NULL)
		return 0;

	int count_rect = 0;

	Track_Contour_t *Contour = *pContour;
	do
	{
		Track_Rect_t rect = Contour->rect;
		if (rect.width > size_Threshold &&
			rect.height > size_Threshold)					//ɸѡ
		{
			*(rect_arr + count_rect) = rect;
			count_rect++;
		}
		Contour = (Track_Contour_t*)Contour->h_next;
	} while (Contour != *pContour);

	int i = 0, j = 0;
	for (i = 0; i < count_rect; i++)
	{
		for (j = 0; j < count_rect; j++)
		{
			if (i!=j)
			{
				if (track_intersect_rect(rect_arr + i, rect_arr + j, 1))		//�ж��Ƿ��ཻ������ཻ��ֱ�Ӻϲ�
				{
					count_rect--;
					*(rect_arr + j) = *(rect_arr + count_rect);
					j--;
				}
			}
		}
	}

	return count_rect;
}

//************************************
// ��������: track_caluclateDirect_ROI
// ����˵���������������˶������ٶȽ�С
// ��    �ߣ�XueYB
// �������ڣ�2015/10/14
// �� �� ֵ: 
// ��    ��: 
//************************************
int track_calculateDirect_ROI(Itc_Mat_t* mhi, Track_Rect_t roi, int *direct)
{
	//int sum_gradientV = 0;		//��ֱ�����ݶ�
	//int sum_gradientH = 0;		//ˮƽ����
	//int count_change = 0;
	//int count_changeX = 0;		//ͳ����ƫ�Ƶĵ�����
	//int count_changeY = 0;		//
	//int x1 = roi.x;
	//int y1 = roi.y;
	//int x2 = roi.x + roi.width;
	//int y2 = roi.y + roi.height;
	//int step = src->step;
	//uchar *img0 = (uchar*)(src->data.ptr + step*y1);
	//uchar *img1 = (uchar*)(src->data.ptr + step*(y1 + 1));
	//int i = 0, j = 0;
	//for (i = y1; i < y2-1; i++)
	//{
	//	uchar last_Value = 0;
	//	int startX = x1 - 1;
	//	for (j = x1; j < x2-1; j++)
	//	{
	//		if (img1[j] != 0)
	//		{
	//			int gradientX = img0[j + 1] - img0[j];
	//			int gradientY = img1[j] - img0[j];
	//			if (gradientX <= 1 && gradientX >= -1)
	//			{
	//				sum_gradientH += gradientX;
	//				count_changeX++;
	//			}
	//			if (gradientY <= 1 && gradientY >= -1)
	//			{
	//				sum_gradientV += gradientY;
	//				count_changeY++;
	//			}
	//			count_change++;
	//		}
	//	}
	//	img0 = img1;
	//	img1 += step;
	//}
	//int threshold = (roi.width*roi.height) >> 2;
	//if (count_change>threshold)
	//{
	//	if (abs(sum_gradientV) > abs(sum_gradientH))
	//	{
	//		int threshold1 = count_changeY >> 4;
	//		if (sum_gradientV > threshold1)
	//		{
	//			//printf("λ�ã�%d,%d,��С��%d,%d ��ֱ�ݶȣ�%d,ˮƽ�ݶȣ�%d\n", x1, y1, roi.width, roi.height, sum_gradientV, sum_gradientH);
	//			direct = 1;
	//			return 1;
	//		}
	//		else if (sum_gradientV < -threshold1)
	//		{
	//			direct = 2;
	//		}
	//	}
	//	else
	//	{
	//		int threshold2 = count_changeX >> 4;
	//		if (sum_gradientH > threshold2)
	//		{
	//			direct = 3;
	//		}
	//		else if (sum_gradientH < -threshold2)
	//		{
	//			direct = 4;
	//		}
	//	}		
	//}
	
	if (mhi == NULL || direct == NULL)
	{
		return 0;
	}

	int sum_gradientV = 0;		//��ֱ�����ݶ�
	int sum_gradientH = 0;		//ˮƽ����
	int count_change = 0;
	int count_changeX = 0;		//ͳ����ƫ�Ƶĵ�����
	int count_changeY = 0;		//

	int x1 = roi.x;
	int y1 = roi.y;
	int x2 = roi.x + roi.width;
	int y2 = roi.y + roi.height;
	int step = mhi->step;

	int i = 0, j = 0;

	int flag_signLase = 0;			//���ڱ�ǵ�ǰ�����������½�
	int flag_signCurr = 0;
	itc_uchar last_Value = 0;
	int startY = y1 - 1;
	int startX = x1 - 1;
	itc_uchar *img0 = (itc_uchar*)(mhi->data.ptr + step*y1);
	itc_uchar *img1 = img0;
	
	int sign_Value = 0;		//���ڱ�ʾ���η����Ƿ����
	int k_int_enhance = 1 << (ITC_FIXEDPOINT_ALIGN - 2);	//������߳�������
	int k = 0;
	//����ˮƽ�����ٶ�
	for (i = y1; i < y2; i++)
	{
		flag_signLase = 0;				//���ڱ�ǵ�ǰ�����������½�
		flag_signCurr = 0;
		last_Value = 0;
		startX = (x1 - 1)*k_int_enhance;
		k = startX;
		for (j = x1; j < x2; j++, k += k_int_enhance)
		{
			if (img1[j] != last_Value)	//��⵽һ���仯��
			{
				if (last_Value != 0)	//last_Value=0,˵����ǰ��һ����㣬ֻ����startX=j����
				{
					flag_signCurr = img1[j] - last_Value;			//���㵱ǰ����
					if (flag_signLase != 0)
					{
						///�Ѿ�ȷ����ǰ�ݶȵķ���
						if (img1[j] != 0 )
						{
							sign_Value = flag_signCurr*flag_signLase;
							if (sign_Value > 0)	//����һ�µĲ���Ч
							{
								sum_gradientH += (k - startX) / flag_signCurr;
								count_changeX++;
							}
							else
							{
								if (flag_signCurr > 0)				//flag_signCurr > 0˵��flag_signLase<0��
								{									//��ǰ������ģ�����Ҫ����Ч����㣬��һ���ǽ�����ô��ǰ��Ҳ��һ����Ч�Ľ�����,������+2
									sum_gradientH += ((k - startX)*(flag_signLase + flag_signCurr) / sign_Value);	//((k - startX) / flag_signCurr)+((k - startX) / flag_signLase)
									count_changeX += 2;
								}
							}
						}
						else
						{
							if (flag_signLase < 0)					//����λ������ǽ���ģ���ô����Ч��
							{
								sum_gradientH += (k - startX) / flag_signLase;
								count_changeX++;
							}
							flag_signCurr = 0;
						}
						//if (flag_signLase>0)
						//{
						//	if (flag_signCurr > 0 && img1[j] != 0)				//����һ�µĲ���Ч
						//	{
						//		sum_gradientH += (j - startX) / flag_signCurr;
						//		count_changeX++;
						//	}
						//}
						//else
						//{
						//	if (flag_signCurr < 0 && img1[j] != 0)
						//	{
						//		sum_gradientH += (j - startX) / flag_signCurr;
						//		count_changeX++;
						//	}
						//	else if (img1[j] == 0)				//����λ������ǽ���ģ���ô����Ч��
						//	{
						//		sum_gradientH += (j - startX) / flag_signLase;
						//		count_changeX++;
						//	}
						//}
					}
					else
					{
						//��δȷ����ǰ�ݶȵķ���
						if (flag_signCurr > 0)						//��ǰλ�����������ģ���ô����Ч��,��Ϊ�Ǵ���ʼɨ��
						{
							sum_gradientH += (k - startX) / flag_signCurr;
							count_changeX++;
						}
						else if (img1[j] == 0)						//������������ҵ�ǰֵΪ0���Ǿ�Ӧ������flag_signCurr״̬
						{
							flag_signCurr = 0;
						}
					}
					flag_signLase = flag_signCurr;
				}
				startX = k;
				last_Value = img1[j];
				count_change++;
			}
		}
		k -= k_int_enhance;
		if (img1[j - 1] != 0)//�߽紦��
		{
			if (flag_signLase < 0)
			{
				sum_gradientH += (k - startX) / flag_signLase;
				count_changeX++;
			}
		}
		img1 += step;
	}
	
	for (j = x1; j < x2; j++)
	{
		flag_signLase = 0;				//���ڱ�ǵ�ǰ�����������½�
		flag_signCurr = 0;
		last_Value = 0;
		startY = (y1 - 1)*k_int_enhance;
		k = startY;
		img1 = img0;
		for (i = y1; i < y2; i++, k += k_int_enhance)
		{
			if (img1[j] != last_Value)
			{
				if (last_Value != 0)	//last_Value=0,˵����ǰ��һ����㣬ֻ����startX=j����
				{
					flag_signCurr = img1[j] - last_Value;
					if (flag_signLase != 0)
					{
						//�Ѿ�ȷ����ǰ�ݶȵķ���
						if (img1[j] != 0)
						{
							sign_Value = flag_signCurr*flag_signLase;
							if (sign_Value > 0)
							{
								sum_gradientV += (k - startY) / flag_signCurr;
								count_changeY++;
							}
							else 
							{
								if (flag_signCurr > 0)				//flag_signCurr > 0˵��flag_signLase<0
								{
									sum_gradientV += ((k - startY)*(flag_signLase + flag_signCurr) / sign_Value);	//((k - startX) / flag_signCurr)+((k - startX) / flag_signLase)
									count_changeY += 2;
								}
							}
						}
						else
						{
							if (flag_signLase < 0)
							{
								sum_gradientV += (k - startY) / flag_signLase;
								count_changeY++;
							}
							flag_signCurr = 0;
						}
						//if (flag_signLase > 0)
						//{
						//	if (flag_signCurr > 0 && img1[j] != 0)			//����һ�µĲ���Ч
						//	{
						//		sum_gradientV += (i - startY) / flag_signCurr;
						//		count_changeY++;
						//	}
						//}
						//else
						//{
						//	if (flag_signCurr < 0 && img1[j] != 0)
						//	{
						//		sum_gradientV += (i - startY) / flag_signCurr;
						//		count_changeY++;
						//	}
						//	else if (img1[j] == 0)
						//	{
						//		sum_gradientV += (i - startY) / flag_signLase;
						//		count_changeY++;
						//	}
						//}
					}
					else
					{
						//��δȷ����ǰ�ݶȵķ���
						if (flag_signCurr > 0)
						{
							sum_gradientV += (k - startY) / flag_signCurr;
							count_changeY++;
						}
						else if (img1[j] == 0)
						{
							flag_signCurr = 0;
						}
					}
					flag_signLase = flag_signCurr;
				}
				startY = k;
				last_Value = img1[j];
				count_change++;
			}
			img1 += step;
		}

		img1 -= step;
		k -= k_int_enhance;
		if (img1[j] != 0)
		{
			if (flag_signLase < 0)
			{
				sum_gradientV += (k - startY) / flag_signLase;
				count_changeY++;
			}
		}
	}

	int threshold = (roi.width*roi.height) >> 3;
	count_change = count_change >> 1;
	sum_gradientH = sum_gradientH << ITC_FIXEDPOINT_ALIGN;
	sum_gradientV = sum_gradientV << ITC_FIXEDPOINT_ALIGN;
	int gradientH = 0, gradientV = 0;
	if (count_changeX > 0)
		gradientH = sum_gradientH / (count_changeX*k_int_enhance);
	if (count_changeY)
		gradientV = sum_gradientV / (count_changeY*k_int_enhance);
	int angle = (int)(atan2(gradientV, gradientH) * ITC_RADIAN_TO_ANGLE);
	angle = angle < 0 ? angle + ITC_360DEGREE : angle;
	*direct = angle;
	if (count_change>threshold)
	{
		int gradientV_abs = abs(gradientV);
		int gradientH_abs = abs(gradientH);
		if (gradientV_abs>gradientH_abs)
		{
			if (gradientV_abs > (1 << (ITC_FIXEDPOINT_ALIGN - 1)))
				return 1;
		}
		else if (gradientH_abs > (1 << (ITC_FIXEDPOINT_ALIGN - 1)))
		{
			return 2;
		}
	}
	return 0;
}

void track_update_midValueBK(Itc_Mat_t* mat, Itc_Mat_t* matBK)
{
	if (!ITC_ARE_TYPES_EQ(mat, matBK))		//��������Ƿ�һ��
		ITC_ERROR_("�������Ͳ�һ��");

	if (!ITC_ARE_SIZES_EQ(mat, matBK))		//����С�Ƿ�һ��
		ITC_ERROR_("�����С��һ��");

	int type = ITC_MAT_TYPE(mat->type);		//����
	int depth = ITC_MAT_DEPTH(type);		//���
	if (depth != ITC_8U)
		ITC_ERROR_("������Ȳ���uchar");
	if (ITC_MAT_CN(type) != 1)				//ͨ����
		ITC_ERROR_("ͨ������Ϊ1");

	Track_Size_t sizeMat;
	sizeMat.width = mat->cols;
	sizeMat.height = mat->rows;

	int i = 0;
	int j = 0;
	itc_uchar *qmat = mat->data.ptr;
	itc_uchar *qmBK = matBK->data.ptr;

	for (i = 0; i < sizeMat.height; i++)
	{
		for (j = 0; j < sizeMat.width; j++)
		{
			int matValue = qmBK[j];
			int diff = abs(matValue - qmat[j]);
			if (diff<250)
			{
				if (qmat[j]>qmBK[j])
				{
					qmBK[j] = ITC_CAST_8U(++matValue);
				}
				else if (qmat[j] < qmBK[j])
				{
					qmBK[j] = ITC_CAST_8U(--matValue);
				}
			}
			else
			{
				qmBK[j] = qmat[j];
			}
		}
		qmat += mat->step;
		qmBK += matBK->step;
	}
}

int track_copyImage_ROI(Itc_Mat_t* src, Itc_Mat_t* dst, Track_Rect_t roi)
{
	//memset(dst, 0, dst->step*dst->rows);
	if (!src->data.ptr)
	{
		return -1;
	}
	int i = 0;
	int x1 = roi.x;
	int y1 = roi.y;
	int y2 = y1 + roi.height;

	int step = roi.width;

	itc_uchar *img = (itc_uchar*)(src->data.ptr + y1*src->step + x1);
	itc_uchar *img0 = (itc_uchar*)(dst->data.ptr);

	for (i = y1; i < y2; i++)
	{
		memcpy(img0, img, step);
		img += src->step;
		img0 += step;
	}
	if (!dst->data.ptr)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

BOOL track_resize_matData(itc_uchar* srcData, Track_Size_t *ssize, char* dstData, Track_Size_t *dsize)
{
	if (srcData == NULL || ssize==NULL
		|| dstData == NULL || dsize == NULL)
	{
		return FALSE;
	}

	int* x_ofs = (int*)itcAlloc(dsize->width * sizeof(x_ofs[0]));
	if (x_ofs == NULL)
	{
		return FALSE;
	}

	int x, y, t;
	int swidth2 = ssize->width << 1;
	int sheight2 = ssize->height << 1;
	int dwidth2 = dsize->width << 1;
	int dheight2 = dsize->height << 1;
	int minWidth = ITC_IMIN(ssize->width, dsize->width) - 1;
	int minHeight = ITC_IMIN(ssize->height, dsize->height) - 1;
	for (x = 0; x < dsize->width; x++)
	{
		t = (swidth2*x + minWidth) / dwidth2;
		t -= t >= ssize->width;
		x_ofs[x] = t;
	}

	int x_count = dsize->width - 2;
	for (y = 0; y < dsize->height; y++, dstData += dsize->width)
	{
		const itc_uchar* tsrc;
		t = (sheight2*y + minHeight) / dheight2;
		t -= t >= ssize->height;
		tsrc = srcData + ssize->width*t;
		for (x = 0; x <= x_count; x += 2)
		{
			itc_uchar t0 = tsrc[x_ofs[x]];
			itc_uchar t1 = tsrc[x_ofs[x + 1]];

			dstData[x] = t0;
			dstData[x + 1] = t1;
		}
	}
	itcFree_(x_ofs);
	x_ofs = NULL;
	return TRUE;
}
