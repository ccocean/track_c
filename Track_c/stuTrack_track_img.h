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
	int flag_Stand;		//������־
	int flag_matching;	//ƥ���־
	Track_Point_t centre;
	Track_Rect_t roi;
	clock_t start_tClock;
	clock_t current_tClock;
}StuTrack_Stand_t;

typedef struct StuTrack_BigMoveObj_t
{
	int count_track;
	int flag_bigMove;		//��־
	int dis_threshold;		//��Ϊ���ƶ�Ŀ�����ֵ
	Track_Rect_t roi;
	clock_t start_tClock;
	clock_t current_tClock;
	Track_Point_t origin_position;
	Track_Point_t current_position;
}StuTrack_BigMoveObj_t;

typedef struct _StuITRACK_InteriorParams
{
	BOOL initialize_flag;
	unsigned int _count;	//ͳ��֡��
	size_t img_size;		//ͼ���Сw*h

	int result_flag;							//��ǰ֡�仯״̬

	unsigned int count_trackObj_stand;			//�����������
	StuTrack_Stand_t* stuTrack_stand;

	unsigned int count_trackObj_bigMove;		//�ƶ�Ŀ�����
	StuTrack_BigMoveObj_t* stuTrack_bigMOveObj;

	unsigned int count_stuTrack_rect;			//�˶��������
	Track_Rect_t *stuTrack_rect_arr;

	Track_MemStorage_t* stuTrack_storage;

	int *stuTrack_size_threshold;				//�˶�Ŀ���С������ֵ������λ�ò�ͬ��ֵ��ͬ��
	int *stuTrack_direct_threshold;				//�����ı�׼�Ƕ�,��СΪwidth
	Itc_Mat_t *tempMat;
	Itc_Mat_t *currMat;
	Itc_Mat_t *lastMat;
	Itc_Mat_t *mhiMat;
	Itc_Mat_t *maskMat;

	callbackmsg callbackmsg_func;						//������Ϣ����ĺ���ָ��
}StuITRACK_InteriorParams;

void stuTrack_initializeTrack(StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p);
void stuTrack_process(StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p, StuITRACK_OutParams_t* return_params, char* imageData);
void stuTrack_stopTrack(StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p);

#ifdef  __cplusplus  
}
#endif  /* end of __cplusplus */ 

#endif