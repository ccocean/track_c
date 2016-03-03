#ifndef _TCH_TRACK_
#define _TCH_TRACK_

#include "itcCore.h"
#include "tch_params.h"
#include "itcTrack_draw_img.h"
#include "Tch_Queue.h"
#include<time.h>

#ifdef  __cplusplus
extern "C" {
#endif

//定义老师跟踪状态
#define RETURN_TRACK_TCH_NULL 1				//老师没有状态
#define RETURN_TRACK_TCH_MOVEINVIEW 2		//老师在特写镜头内
#define RETURN_TRACK_TCH_MOVEOUTVIEW 3		//老师走出特写镜头
#define RETURN_TRACK_TCH_OUTSIDE 4			//老师离开跟踪区域
#define RETURN_TRACK_TCH_BLACKBOARD 5		//老师书写板书状态
#define RETURN_TRACK_TCH_MULITY 6			//讲台多目标状态

//返回状态
#define STATUS_CHANGE 1
#define STATUS_NONE 0

//定义默认参数
#define TRACK_SLIDE_WIDTH 5
#define TRACK_NUMOF_POSITION 10

#define TRACK_DEFAULT_WIDTH 480
#define TRACK_DEFAULT_HEIGHT 264
#define TRACK_DEFAULT_TCH_X 0
#define TRACK_DEFAULT_TCH_Y 75
#define TRACK_DEFAULT_TCH_W 480
#define TRACK_DEFAULT_TCH_H 150
#define TRACK_DEFAULT_BLK_X 0
#define TRACK_DEFAULT_BLK_Y 30
#define TRACK_DEFAULT_BLK_W 480
#define TRACK_DEFAULT_BLK_H 37

//站定时间的阈值
#define TRACK_STAND_THRESHOLD 3500
#define TRACK_TARGETAREA_THRESHOLD 6000 //7200
#define TRACK_TCHOUTSIDE_THRESHOLD TRACK_DEFAULT_TCH_H*0.65

#define TRACK_OUTSIDE_ANGLE_UP 150
#define TRACK_OUTSIDE_ANGLE_DN 30
#define TRACK_FRAMES_ANALYSIS 25

//计时器
typedef struct TrackTimer
{
	unsigned long start;
	unsigned long finish;
	double deltaTime;
}Tch_Timer_t;

//预置位块
typedef struct CamPosition
{
	int index;
	//pixels
	int left_pixel;
	int right_pixel;
}Tch_CamPosition_t;

//预置位滑块
typedef struct CamPositionSlide
{
	//index
	int center;
	int left;
	int right;
	int width;
}Tch_CamPosSlide_t;

typedef struct FromSystem
{
	int width;
	int height;
	callbackmsg callbackmsg_func;
}Tch_SysData_t;


typedef struct Data
{
	//判定旗帜
	int g_isMulti;
	int g_isOnStage;
	int g_count;

	int g_posIndex;
	int g_prevPosIndex;
	/*int g_rectCnt;*/
	int tch_lastStatus;

	//预置位
	int track_pos_width;
	int numOfPos;
	int numOfSlide;
	int lastRectNum;

	Track_Point_t center;
	//Track_Point_t center1, center2;
	Track_Point_t lastCenter;

	Track_Size_t g_frameSize; //压缩后的大小
	Track_Size_t src_size;	//原始图像大小
	Track_Rect_t g_tchWin;  //处理教师的图片大小
	Track_Rect_t g_blkWin;

	//Itc_Mat_t *srcMat;
	Itc_Mat_t *tempMatTch;
	Itc_Mat_t *tempMatBlk;
	Itc_Mat_t *prevMatTch;
	Itc_Mat_t *prevMatBlk;
	Itc_Mat_t *currMatTch;
	Itc_Mat_t *mhiMatTch;
	Itc_Mat_t *maskMatTch;
	Itc_Mat_t *currMatBlk;
	Itc_Mat_t *mhiMatBlk;
	Itc_Mat_t *maskMatBlk;

	Track_MemStorage_t *storage;
	Track_MemStorage_t *storageTch;
	Track_MemStorage_t *storageBlk;

	//计时器定义
	Tch_Timer_t slideTimer;
	Tch_Timer_t tch_timer;
	Tch_Timer_t outsideTimer;

	//预置位滑块定义
	Tch_CamPosSlide_t pos_slide;

	//初始化预置位块
	//Tch_CamPosition_t tempCams[TRACK_NUMOF_POSITION];
	Tch_CamPosition_t *cam_pos;

	Tch_SysData_t sysData;

	//Tch_Queue_t *tch_queue;
	
	//用于绘制的颜色
	Track_Colour_t pink_colour;
	Track_Colour_t blue_colour;
	Track_Colour_t lilac_colour;
	Track_Colour_t green_colour;
	Track_Colour_t red_colour;
	Track_Colour_t dullred_colour;
	Track_Colour_t yellow_colour;

}Tch_Data_t;

//#define _PRINTF ((callbackmsg)(data->callbackmsg_func))

int tch_Init(TeaITRACK_Params *params, Tch_Data_t *data);//先调用这个

int tch_track(itc_uchar *src, itc_uchar* pUV, TeaITRACK_Params *params, Tch_Data_t *data, Tch_Result_t *res);//开始跟踪调用这个

//int tch_track(Itc_Mat_t *src, TeaITRACK_Params *params, Tch_Data_t *data, Tch_Result_t *res);//开始跟踪调用这个

void tch_trackDestroy(Tch_Data_t *data);//一帧结束调用这个

int tch_trackInit(Tch_Data_t *data);//不用管

int tch_calculateDirect_TCH(Itc_Mat_t* src, Track_Rect_t roi);//不用管

#ifdef  __cplusplus  
}
#endif  /* end of __cplusplus */ 

#endif
