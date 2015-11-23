/************************************************************************** 
    *  @Copyright (c) 2015, XueYB, All rights reserved. 
 
    *  @file     : itcCore.h 
    *  @version  : ver 1.0 
 
    *  @author   : XueYB 
    *  @date     : 2015/10/09 11:19 
    *  @brief    : Mat结构和图像基本操作
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

Itc_Mat_t	itc_mat(int rows, int cols, int type, void* data);											//手动分配数据创建Mat,注意不能用itcReleaseMat释放
Itc_Mat_t*	itc_create_mat( int height, int width, int type );											//创建Mat
Itc_Mat_t*	itc_create_matHeader( int rows, int cols, int type );										//创建Mat头
Itc_Mat_t*	itc_init_matHeader( Itc_Mat_t* arr, int rows, int cols, int type, void* data, int step );	//有Mat头后初始化
void	itc_release_mat( Itc_Mat_t** mat );															//释放Mat,包括头和数据


void track_sub_mat(Itc_Mat_t* src1, Itc_Mat_t* src2, Itc_Mat_t* dst);			//dst=src1-src2

//输入连续两帧图像，生成mhi图
void track_update_MHI(Itc_Mat_t* src1,//当前帧
	Itc_Mat_t* src2,				//上一帧
	Itc_Mat_t* mhi,					//运动历史图像
	int diffThreshold,				//用于过滤帧差大小
	Itc_Mat_t* maskT,				//掩码图像，用于轮廓分析
	int Threshold);					//用于生成掩码,mhi大于该值才把对应的maskT置为1

//轮廓检测
int track_find_contours(Itc_Mat_t* src1,	//输入二值图像（0，1），4周边界必须为0，否则会越界
	Track_Contour_t** pContour,					//输出的轮廓
	Track_MemStorage_t*  storage);				//存储器


int track_filtrate_contours(Track_Contour_t** pContour,int size_Threshold, Track_Rect_t *rect_arr);

int track_intersect_rect(Track_Rect_t *rectA, Track_Rect_t *rectB, int expand_dis);	//判断两个矩形框是否相交

int track_calculateDirect_ROI(Itc_Mat_t* mhi, Track_Rect_t roi, int *direct);	//返回roi区域内的目标整体运动方向，

void track_update_midValueBK(Itc_Mat_t* mat, Itc_Mat_t* matBK);					//用中值法更新背景

int track_copyImage_ROI(Itc_Mat_t* src, Itc_Mat_t* dst, Track_Rect_t roi);

BOOL  track_resize_matData(uchar* srcData, Track_Size_t* ssize, char* dstData, Track_Size_t* dsize);
#ifdef  __cplusplus  
}
#endif  /* end of __cplusplus */ 

#endif // itcCore_h__