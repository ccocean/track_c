#ifndef stuTrack_settings_parameter_h__
#define stuTrack_settings_parameter_h__

#ifdef  __cplusplus
extern "C" {
#endif

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

typedef struct 	_StuITRACK_Params
{
	int flag_setting;	//�����Ƿ�����
	int height;			//ͼ��߶�
	int width;			//ͼ����

	TrackPrarms_Point_t stuTrack_vertex[4];		//ѧ�������ĸ�����λ��
	int stuTrack_direct_standard[4];			//�ĸ�����λ����ֱ������ͼ���еĽǶ�
	int stuTrack_stuWidth_standard[4];			//�ĸ�����λ��ѧ����ͼ������ռ�Ŀ��
	int stuTrack_direct_range;					//����ʱ����ĽǶ�ƫ�뷶Χ
	float stuTrack_move_threshold;				//�ж����ƶ�Ŀ���ƫ����ֵ����ֵ��
	int stuTrack_standCount_threshold;			//�ж�Ϊ������֡����ֵ
	int stuTrack_sitdownCount_threshold;		//�ж�Ϊ���µ�֡����ֵ
	int stuTrack_moveDelayed_threshold;		//�ƶ�Ŀ�걣�ָ��ٵ���ʱ���������ʱ�����˶������������(��λ������)
}StuITRACK_Params;

//
#define STUTRACK_NEWCHANGE_FLAG		(1<<30)
#define STUTRACK_IF_NEWCHANGE(n)	((n & STUTRACK_NEWCHANGE_FLAG)== STUTRACK_NEWCHANGE_FLAG)	//�ж��Ƿ��б仯

//�仯״̬��
#define STUTRACK_RETURN_NULL		0
#define	STUTRACK_RETURN_STANDUP		1
#define	STUTRACK_RETURN_SITDOWN		2
#define	STUTRACK_RETURN_MOVE		4
#define STUTRACK_RETURN_STOPMOVE	8

#define STUTRACK_IF_STANDUP(n)		((n & STUTRACK_RETURN_STANDUP)== STUTRACK_RETURN_STANDUP)	//�ж��Ƿ�������
#define STUTRACK_IF_SITDOWN(n)		((n & STUTRACK_RETURN_SITDOWN)== STUTRACK_RETURN_SITDOWN)	//�ж��Ƿ�������
#define STUTRACK_IF_MOVE(n)			((n & STUTRACK_RETURN_MOVE)== STUTRACK_RETURN_MOVE)			//�ж��Ƿ����ƶ�Ŀ��
#define STUTRACK_IF_STOPMOVE(n)		((n & STUTRACK_RETURN_STOPMOVE)== STUTRACK_RETURN_STOPMOVE)	//�ж��Ƿ����ƶ�Ŀ��ֹͣ�˶�

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

#ifdef  __cplusplus  
}
#endif  /* end of __cplusplus */ 

#endif