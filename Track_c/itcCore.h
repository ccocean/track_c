/************************************************************************** 
    *  @Copyright (c) 2015, XueYB, All rights reserved. 
 
    *  @file     : itcCore.h 
    *  @version  : ver 1.0 
 
    *  @author   : XueYB 
    *  @date     : 2015/10/09 11:19 
    *  @brief    : Mat�ṹ��ͼ���������
**************************************************************************/
#ifndef itcCore_h__
#define itcCore_h__
#include "itctype.h"
#include "itcerror.h"
#include "itcdatastructs.h"

#ifdef  __cplusplus
extern "C" {
#endif

#include <time.h>
#define CONVERSION_STUTRACK_UNITStoMS (1000)

#ifdef _WIN32
#include <windows.h>
#define  gettime GetTickCount
#else
#include <mcfw/src_bios6/utils/utils.h>
#include <mcfw/interfaces/ti_media_std.h>
#define  gettime Utils_getCurTimeInMsec
#endif

typedef struct Itc_Mat
{
	int type;
	int step;

	/* for internal use only */
	int* refcount;
	int hdr_refcount;

	union
	{
		uchar* ptr;
		short* s;
		int* i;
		float* fl;
		double* db;
	} data;

	union
	{
		int rows;
		int height;
	};

	union
	{
		int cols;
		int width;
	};
}
Itc_Mat_t;

Itc_Mat_t	itc_mat(int rows, int cols, int type, void* data);											//�ֶ��������ݴ���Mat,ע�ⲻ����itcReleaseMat�ͷ�
Itc_Mat_t*	itc_create_mat( int height, int width, int type );											//����Mat
Itc_Mat_t*	itc_create_matHeader( int rows, int cols, int type );										//����Matͷ
Itc_Mat_t*	itc_init_matHeader( Itc_Mat_t* arr, int rows, int cols, int type, void* data, int step );	//��Matͷ���ʼ��
void	itc_release_mat( Itc_Mat_t** mat );															//�ͷ�Mat,����ͷ������


void track_sub_mat(Itc_Mat_t* src1, Itc_Mat_t* src2, Itc_Mat_t* dst);			//dst=src1-src2

//����������֡ͼ������mhiͼ
void track_update_MHI(Itc_Mat_t* src1,//��ǰ֡
	Itc_Mat_t* src2,				//��һ֡
	Itc_Mat_t* mhi,					//�˶���ʷͼ��
	int diffThreshold,				//���ڹ���֡���С
	Itc_Mat_t* maskT,				//����ͼ��������������
	int Threshold);					//������������,mhi���ڸ�ֵ�ŰѶ�Ӧ��maskT��Ϊ1

//�������
int track_find_contours(Itc_Mat_t* src1,	//�����ֵͼ��0��1����4�ܱ߽����Ϊ0�������Խ��
	Track_Contour_t** pContour,					//���������
	Track_MemStorage_t*  storage);				//�洢��


int track_filtrate_contours(Track_Contour_t** pContour,int size_Threshold, Track_Rect_t *rect_arr);

int track_intersect_rect(Track_Rect_t *rectA, Track_Rect_t *rectB, int expand_dis);	//�ж��������ο��Ƿ��ཻ

int track_calculateDirect_ROI(Itc_Mat_t* mhi, Track_Rect_t roi, int *direct);	//����roi�����ڵ�Ŀ�������˶�����

void track_update_midValueBK(Itc_Mat_t* mat, Itc_Mat_t* matBK);					//����ֵ�����±���

int track_copyImage_ROI(Itc_Mat_t* src, Itc_Mat_t* dst, Track_Rect_t roi);

BOOL  track_resize_matData(uchar* srcData, Track_Size_t* ssize, char* dstData, Track_Size_t* dsize);
#ifdef  __cplusplus  
}
#endif  /* end of __cplusplus */ 

#endif // itcCore_h__