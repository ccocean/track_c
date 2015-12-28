#ifndef stuTrack_settings_parameter_h__
#define stuTrack_settings_parameter_h__

#ifdef  __cplusplus
extern "C" {
#endif

#define HEIGHT_STUTRACK_IMG_ 264
#define WIDTH_STUTRACK_IMG_	480

typedef struct _POINT
{
	int x;
	int y;
}TrackPrarms_Point_t;

typedef struct _SIZE
{
	int width;
	int height;
}TrackPrarms_Size_t;

typedef struct _StuITRACK_ClientParams
{
	int flag_setting;	//�����Ƿ�����
	int height;			//���õ�ͼ��߶�
	int width;			//���õ�ͼ����
	int stuTrack_debugMsg_flag;					//������Ϣ����ȼ�
	int stuTrack_Draw_flag;						//�Ƿ���ƽ��
	int stuTrack_direct_standard[4];			//�ĸ�����λ����ֱ������ͼ���еĽǶ�
	int stuTrack_stuWidth_standard[4];			//�ĸ�����λ��ѧ����ͼ������ռ�Ŀ��
	int stuTrack_direct_range;					//����ʱ����ĽǶ�ƫ�뷶Χ
	int stuTrack_standCount_threshold;			//�ж�Ϊ������֡����ֵ
	int stuTrack_sitdownCount_threshold;		//�ж�Ϊ���µ�֡����ֵ
	int stuTrack_moveDelayed_threshold;			//�ƶ�Ŀ�걣�ָ��ٵ���ʱ���������ʱ�����˶������������(��λ������)
	double stuTrack_move_threshold;				//�ж����ƶ�Ŀ���ƫ����ֵ����ֵ��
	TrackPrarms_Point_t stuTrack_vertex[4];		//ѧ�������ĸ�����λ��
}StuITRACK_ClientParams_t;

//---------------------------------------------------------------------����������
#define RESULT_STUTRACK_NEWCHANGE_FLAG		(1<<30)
#define RESULT_STUTRACK_IF_NEWCHANGE(n)	((n & RESULT_STUTRACK_NEWCHANGE_FLAG)== RESULT_STUTRACK_NEWCHANGE_FLAG)	//�ж��Ƿ��б仯

//�仯״̬��
#define RESULT_STUTRACK_NULL_FLAG		0
#define	RESULT_STUTRACK_STANDUP_FLAG	1
#define	RESULT_STUTRACK_SITDOWN_FLAG	2
#define	RESULT_STUTRACK_MOVE_FLAG		4
#define RESULT_STUTRACK_STOPMOVE_FLAG	8

#define RESULT_STUTRACK_IF_STANDUP(n)		((n & RESULT_STUTRACK_STANDUP_FLAG)== RESULT_STUTRACK_STANDUP_FLAG)	//�ж��Ƿ�������
#define RESULT_STUTRACK_IF_SITDOWN(n)		((n & RESULT_STUTRACK_SITDOWN_FLAG)== RESULT_STUTRACK_SITDOWN_FLAG)	//�ж��Ƿ�������
#define RESULT_STUTRACK_IF_MOVE(n)			((n & RESULT_STUTRACK_MOVE_FLAG)== RESULT_STUTRACK_MOVE_FLAG)			//�ж��Ƿ����ƶ�Ŀ��
#define RESULT_STUTRACK_IF_STOPMOVE(n)		((n & RESULT_STUTRACK_STOPMOVE_FLAG)== RESULT_STUTRACK_STOPMOVE_FLAG)	//�ж��Ƿ����ƶ�Ŀ��ֹͣ�˶�

typedef struct _StuITRACK_OutParams
{
	int result_flag;							//��ǰ֡�仯״̬
	unsigned int count_trackObj_stand;			//����Ŀ�����
	unsigned int count_trackObj_bigMove;		//�ƶ�Ŀ�����
	TrackPrarms_Point_t stand_position;			//����Ŀ��λ��
	TrackPrarms_Point_t move_position;			//�ƶ�Ŀ��λ��
	TrackPrarms_Size_t standObj_size;			//����Ŀ���С
	TrackPrarms_Size_t moveObj_size;			//�ƶ�Ŀ���С
}StuITRACK_OutParams_t;
//-----------------------------------------------------------------------

#ifdef  __cplusplus  
}
#endif  /* end of __cplusplus */ 

#endif
