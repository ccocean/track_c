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

#define COUNT_STUTRACK_MALLOC_ELEMENT 10

#define _PRINTF											\
if (interior_params_p->callbackmsg_func == NULL)		\
{	interior_params_p->callbackmsg_func = printf; }		\
((callbackmsg)(interior_params_p->callbackmsg_func))

#define ITC_RETURN
#define JUDEGE_STUREACK_IF_NULL(p,r)			\
if ((p) == NULL)								\
{												\
	_PRINTF("Memory allocation error!\n");		\
	stuTrack_stopTrack(inst, interior_params_p);\
	return r;									\
}

#ifdef STUTRACK_DEBUG_SWICTH
#define ITC_FUNCNAME(name) _PRINTF(name)
#endif
#ifndef STUTRACK_DEBUG_SWICTH
#define ITC_FUNCNAME(name)
#endif

typedef int(*_callbackmsg)(const char *format, ...);//用于输出调试信息的函数指针

typedef struct StuTrack_Stand_t
{
	int direction;
	int count_teack;	//
	int count_up;		//
	int count_down;	//
	int flag_Stand;		//起立标志
	int flag_matching;	//匹配标志
	Track_Point_t centre;
	Track_Rect_t roi;
	unsigned int start_tClock;
	unsigned int current_tClock;
}StuTrack_Stand_t;

typedef struct StuTrack_BigMoveObj_t
{
	int count_track;
	int flag_bigMove;		//标志
	int dis_threshold;		//认为是移动目标的阈值
	Track_Rect_t roi;
	unsigned int start_tClock;
	unsigned int current_tClock;
	Track_Point_t origin_position;
	Track_Point_t current_position;
}StuTrack_BigMoveObj_t;

typedef struct _StuITRACK_InteriorParams
{
	BOOL initialize_flag;
	int _count;	//统计帧数
	Track_Size_t img_size;		//处理图像大小
	Track_Size_t srcimg_size;	//原始图像大小

	int result_flag;					//当前帧变化状态

	int count_trackObj_stand;			//起立区域计数
	StuTrack_Stand_t* stuTrack_stand;
	int count_trackObj_bigMove;			//移动目标计数
	StuTrack_BigMoveObj_t* stuTrack_bigMOveObj;
	int count_stuTrack_rect;			//运动区域计数
	Track_Rect_t *stuTrack_rect_arr;

	int stuTrack_debugMsg_flag;					//调试信息输出等级
	int stuTrack_Draw_flag;						//是否绘制结果
	int stuTrack_direct_range;					//起立时允许的角度偏离范围
	int stuTrack_standCount_threshold;			//判定为起立的帧数阈值
	int stuTrack_sitdownCount_threshold;		//判定为坐下的帧数阈值
	int stuTrack_moveDelayed_threshold;			//移动目标保持跟踪的延时，超过这个时间无运动，则放弃跟踪(单位：毫秒)
	double stuTrack_move_threshold;				//判定是移动目标的偏离阈值（比值）
	int *stuTrack_size_threshold;				//运动目标大小过滤阈值（根据位置不同阈值不同）
	int *stuTrack_direct_threshold;				//起立的标准角度,大小为width

	Itc_Mat_t *tempMat;
	Itc_Mat_t *currMat;
	Itc_Mat_t *lastMat;
	Itc_Mat_t *mhiMat;
	Itc_Mat_t *maskMat;
	Track_MemStorage_t* stuTrack_storage;

	//用于绘制的颜色
	Track_Colour_t pink_colour;
	Track_Colour_t blue_colour;
	Track_Colour_t lilac_colour;
	Track_Colour_t green_colour;
	Track_Colour_t red_colour;
	Track_Colour_t dullred_colour;
	Track_Colour_t yellow_colour;

	_callbackmsg callbackmsg_func;					//用于信息输出的函数指针
}StuITRACK_InteriorParams;

typedef struct _StuITRACK_SystemParams
{
	int nsrcHeight;			//源图像高度
	int nsrcWidth;			//源图像宽度
	_callbackmsg callbackmsg_func;					//用于信息输出的函数指针
}StuITRACK_SystemParams_t;

typedef struct 	_StuITRACK_Params
{
	StuITRACK_SystemParams_t systemParams;
	StuITRACK_ClientParams_t clientParams;
}StuITRACK_Params;

//默认输入参数值
#define SCALE_STURACK_DEFAULT_ZOOM						0.333333333333333333
#define THRESHOLD_STUTRACK_MOVE_DEFALUT_PARAMS			1.2
#define THRESHOLD_STUTRACK_STANDCOUNT_DEFALUT_PARAMS	5
#define THRESHOLD_STUTRACK_SITDOWNCOUNT_DEFALUT_PARAMS	5
#define THRESHOLD_STUTRACK_MOVEDELAYED_DEFALUT_PARAMS	500
#define RANGE_STUTRACK_STANDDIRECT_DEFALUT_PARAMS		9

//用于计算筛选阈值的线性方程参数
#define A_STUTRACK_SIZE_THRESHOLD_PARAMS		(0.25)
#define B_STUTRACK_SIZE_THRESHOLD_PARAMS		(-6.0)
#define DIRECT_STUTRACK_TRANSMUTABLILITY_RANGE	(40.0)
#define A_STUTRACK_DIRECT_THRESHOLD_PARAMS		(DIRECT_STUTRACK_TRANSMUTABLILITY_RANGE/WIDTH_STUTRACK_IMG_)
#define B_STUTRACK_DIRECT_THRESHOLD_PARAMS		(270-DIRECT_STUTRACK_TRANSMUTABLILITY_RANGE/2)
#define MINTHRESHOLD_STUTRACK_SIZE_THRESHOLD_PARAMS		20
#define MAXTHRESHOLD_STUTRACK_SIZE_THRESHOLD_PARAMS		55
#define MINTHRESHOLD_STUTRACK_DIRECT_THRESHOLD_PARAMS	225
#define MAXTHRESHOLD_STUTRACK_DIRECT_THRESHOLD_PARAMS	315

#define COMPUTER_STUTRACK_SIZE_THRESHOLD_PARAMS(n,a,b)  (ITC_MIN(ITC_MAX(((a *n + b)), MINTHRESHOLD_STUTRACK_SIZE_THRESHOLD_PARAMS), MAXTHRESHOLD_STUTRACK_SIZE_THRESHOLD_PARAMS))
#define COMPUTER_STUTRACK_DIRECT_THRESHOLD_PARAMS(n,a,b)  (ITC_MIN(ITC_MAX(((a *n + b)), MINTHRESHOLD_STUTRACK_DIRECT_THRESHOLD_PARAMS), MAXTHRESHOLD_STUTRACK_DIRECT_THRESHOLD_PARAMS))

BOOL stuTrack_initializeTrack(const StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p);
void stuTrack_process(const StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p, StuITRACK_OutParams_t* return_params, char* imageData, char* bufferuv);
void stuTrack_stopTrack(const StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p);

#ifdef  __cplusplus  
}
#endif  /* end of __cplusplus */ 

#endif
