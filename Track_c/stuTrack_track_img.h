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

typedef int(*_callbackmsg)(const char *format, ...);//�������������Ϣ�ĺ���ָ��

typedef struct StuTrack_Stand_t
{
	int direction;
	int count_teack;	//
	int count_up;		//
	int count_down;	//
	int flag_Stand;		//������־
	int flag_matching;	//ƥ���־
	Track_Point_t centre;
	Track_Rect_t roi;
	unsigned int start_tClock;
	unsigned int current_tClock;
}StuTrack_Stand_t;

typedef struct StuTrack_BigMoveObj_t
{
	int count_track;
	int flag_bigMove;		//��־
	int dis_threshold;		//��Ϊ���ƶ�Ŀ�����ֵ
	Track_Rect_t roi;
	unsigned int start_tClock;
	unsigned int current_tClock;
	Track_Point_t origin_position;
	Track_Point_t current_position;
}StuTrack_BigMoveObj_t;

typedef struct _StuITRACK_InteriorParams
{
	BOOL initialize_flag;
	int _count;	//ͳ��֡��
	Track_Size_t img_size;		//����ͼ���С
	Track_Size_t srcimg_size;	//ԭʼͼ���С

	int result_flag;					//��ǰ֡�仯״̬

	int count_trackObj_stand;			//�����������
	StuTrack_Stand_t* stuTrack_stand;
	int count_trackObj_bigMove;			//�ƶ�Ŀ�����
	StuTrack_BigMoveObj_t* stuTrack_bigMOveObj;
	int count_stuTrack_rect;			//�˶��������
	Track_Rect_t *stuTrack_rect_arr;

	int stuTrack_debugMsg_flag;					//������Ϣ����ȼ�
	int stuTrack_Draw_flag;						//�Ƿ���ƽ��
	int stuTrack_direct_range;					//����ʱ����ĽǶ�ƫ�뷶Χ
	int stuTrack_standCount_threshold;			//�ж�Ϊ������֡����ֵ
	int stuTrack_sitdownCount_threshold;		//�ж�Ϊ���µ�֡����ֵ
	int stuTrack_moveDelayed_threshold;			//�ƶ�Ŀ�걣�ָ��ٵ���ʱ���������ʱ�����˶������������(��λ������)
	double stuTrack_move_threshold;				//�ж����ƶ�Ŀ���ƫ����ֵ����ֵ��
	int *stuTrack_size_threshold;				//�˶�Ŀ���С������ֵ������λ�ò�ͬ��ֵ��ͬ��
	int *stuTrack_direct_threshold;				//�����ı�׼�Ƕ�,��СΪwidth

	Itc_Mat_t *tempMat;
	Itc_Mat_t *currMat;
	Itc_Mat_t *lastMat;
	Itc_Mat_t *mhiMat;
	Itc_Mat_t *maskMat;
	Track_MemStorage_t* stuTrack_storage;

	//���ڻ��Ƶ���ɫ
	Track_Colour_t pink_colour;
	Track_Colour_t blue_colour;
	Track_Colour_t lilac_colour;
	Track_Colour_t green_colour;
	Track_Colour_t red_colour;
	Track_Colour_t dullred_colour;
	Track_Colour_t yellow_colour;

	_callbackmsg callbackmsg_func;					//������Ϣ����ĺ���ָ��
}StuITRACK_InteriorParams;

typedef struct _StuITRACK_SystemParams
{
	int nsrcHeight;			//Դͼ��߶�
	int nsrcWidth;			//Դͼ����
	_callbackmsg callbackmsg_func;					//������Ϣ����ĺ���ָ��
}StuITRACK_SystemParams_t;

typedef struct 	_StuITRACK_Params
{
	StuITRACK_SystemParams_t systemParams;
	StuITRACK_ClientParams_t clientParams;
}StuITRACK_Params;

//Ĭ���������ֵ
#define SCALE_STURACK_DEFAULT_ZOOM						0.333333333333333333
#define THRESHOLD_STUTRACK_MOVE_DEFALUT_PARAMS			1.2
#define THRESHOLD_STUTRACK_STANDCOUNT_DEFALUT_PARAMS	5
#define THRESHOLD_STUTRACK_SITDOWNCOUNT_DEFALUT_PARAMS	5
#define THRESHOLD_STUTRACK_MOVEDELAYED_DEFALUT_PARAMS	500
#define RANGE_STUTRACK_STANDDIRECT_DEFALUT_PARAMS		9

//���ڼ���ɸѡ��ֵ�����Է��̲���
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