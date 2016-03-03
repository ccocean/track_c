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

//������ʦ����״̬
#define RETURN_TRACK_TCH_NULL 1				//��ʦû��״̬
#define RETURN_TRACK_TCH_MOVEINVIEW 2		//��ʦ����д��ͷ��
#define RETURN_TRACK_TCH_MOVEOUTVIEW 3		//��ʦ�߳���д��ͷ
#define RETURN_TRACK_TCH_OUTSIDE 4			//��ʦ�뿪��������
#define RETURN_TRACK_TCH_BLACKBOARD 5		//��ʦ��д����״̬
#define RETURN_TRACK_TCH_MULITY 6			//��̨��Ŀ��״̬

//����״̬
#define STATUS_CHANGE 1
#define STATUS_NONE 0

//����Ĭ�ϲ���
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

//վ��ʱ�����ֵ
#define TRACK_STAND_THRESHOLD 3500
#define TRACK_TARGETAREA_THRESHOLD 6000 //7200
#define TRACK_TCHOUTSIDE_THRESHOLD TRACK_DEFAULT_TCH_H*0.65

#define TRACK_OUTSIDE_ANGLE_UP 150
#define TRACK_OUTSIDE_ANGLE_DN 30
#define TRACK_FRAMES_ANALYSIS 25

//��ʱ��
typedef struct TrackTimer
{
	unsigned long start;
	unsigned long finish;
	double deltaTime;
}Tch_Timer_t;

//Ԥ��λ��
typedef struct CamPosition
{
	int index;
	//pixels
	int left_pixel;
	int right_pixel;
}Tch_CamPosition_t;

//Ԥ��λ����
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
	//�ж�����
	int g_isMulti;
	int g_isOnStage;
	int g_count;

	int g_posIndex;
	int g_prevPosIndex;
	/*int g_rectCnt;*/
	int tch_lastStatus;

	//Ԥ��λ
	int track_pos_width;
	int numOfPos;
	int numOfSlide;
	int lastRectNum;

	Track_Point_t center;
	//Track_Point_t center1, center2;
	Track_Point_t lastCenter;

	Track_Size_t g_frameSize; //ѹ����Ĵ�С
	Track_Size_t src_size;	//ԭʼͼ���С
	Track_Rect_t g_tchWin;  //�����ʦ��ͼƬ��С
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

	//��ʱ������
	Tch_Timer_t slideTimer;
	Tch_Timer_t tch_timer;
	Tch_Timer_t outsideTimer;

	//Ԥ��λ���鶨��
	Tch_CamPosSlide_t pos_slide;

	//��ʼ��Ԥ��λ��
	//Tch_CamPosition_t tempCams[TRACK_NUMOF_POSITION];
	Tch_CamPosition_t *cam_pos;

	Tch_SysData_t sysData;

	//Tch_Queue_t *tch_queue;
	
	//���ڻ��Ƶ���ɫ
	Track_Colour_t pink_colour;
	Track_Colour_t blue_colour;
	Track_Colour_t lilac_colour;
	Track_Colour_t green_colour;
	Track_Colour_t red_colour;
	Track_Colour_t dullred_colour;
	Track_Colour_t yellow_colour;

}Tch_Data_t;

//#define _PRINTF ((callbackmsg)(data->callbackmsg_func))

int tch_Init(TeaITRACK_Params *params, Tch_Data_t *data);//�ȵ������

int tch_track(itc_uchar *src, itc_uchar* pUV, TeaITRACK_Params *params, Tch_Data_t *data, Tch_Result_t *res);//��ʼ���ٵ������

//int tch_track(Itc_Mat_t *src, TeaITRACK_Params *params, Tch_Data_t *data, Tch_Result_t *res);//��ʼ���ٵ������

void tch_trackDestroy(Tch_Data_t *data);//һ֡�����������

int tch_trackInit(Tch_Data_t *data);//���ù�

int tch_calculateDirect_TCH(Itc_Mat_t* src, Track_Rect_t roi);//���ù�

#ifdef  __cplusplus  
}
#endif  /* end of __cplusplus */ 

#endif
