#ifndef stuTrack_track_img_h__
#define stuTrack_track_img_h__

#include "itctype.h"
#include "itcerror.h"
#include "itcdatastructs.h"
#include "itcCore.h"
#include "stuTrack_settings_parameter.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define STUTRACK_IMG_HEIGHT 264
#define STUTRACK_IMG_WIDTH	480

#define MALLOC_ELEMENT_COUNT 10

#define _PRINTF											\
if(interior_params_p->callbackmsg_func==NULL)			\
{	interior_params_p->callbackmsg_func = printf;}		\
((callbackmsg)(interior_params_p->callbackmsg_func))

#define ITC_RETURN
#define JUDEGE_POINTER_NULL(p,r)				\
if ((p) == NULL)								\
{												\
	stuTrack_stopTrack(inst, interior_params_p);\
	return r;									\
}

#include <time.h>
typedef struct StuTrack_Stand_t
{
	int direction;
	int count_teack;	//
	int count_up;		//
	int count_down;		//
	int flag_Stand;		//起立标志
	int flag_matching;	//匹配标志
	Track_Point_t centre;
	Track_Rect_t roi;
	clock_t start_tClock;
	clock_t current_tClock;
}StuTrack_Stand_t;

typedef struct StuTrack_BigMoveObj_t
{
	int count_track;
	int flag_bigMove;		//标志
	int dis_threshold;		//认为是移动目标的阈值
	Track_Rect_t roi;
	clock_t start_tClock;
	clock_t current_tClock;
	Track_Point_t origin_position;
	Track_Point_t current_position;
}StuTrack_BigMoveObj_t;

typedef struct _StuITRACK_InteriorParams
{
	BOOL initialize_flag;
	unsigned int _count;	//统计帧数
	size_t img_size;		//图像大小w*h

	int result_flag;							//当前帧变化状态

	unsigned int count_trackObj_stand;			//起立区域计数
	StuTrack_Stand_t* stuTrack_stand;

	unsigned int count_trackObj_bigMove;		//移动目标计数
	StuTrack_BigMoveObj_t* stuTrack_bigMOveObj;

	unsigned int count_stuTrack_rect;			//运动区域计数
	Track_Rect_t *stuTrack_rect_arr;

	Track_MemStorage_t* stuTrack_storage;

	int *stuTrack_size_threshold;				//运动目标大小过滤阈值（根据位置不同阈值不同）
	int *stuTrack_direct_threshold;				//起立的标准角度,大小为width
	Itc_Mat_t *tempMat;
	Itc_Mat_t *currMat;
	Itc_Mat_t *lastMat;
	Itc_Mat_t *mhiMat;
	Itc_Mat_t *maskMat;

	callbackmsg callbackmsg_func;						//用于信息输出的函数指针
}StuITRACK_InteriorParams;

void stuTrack_initializeTrack(StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p);
void stuTrack_process(StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p, StuITRACK_OutParams_t* return_params, char* imageData);
void stuTrack_stopTrack(StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p);

#ifdef  __cplusplus  
}
#endif  /* end of __cplusplus */ 

#endif