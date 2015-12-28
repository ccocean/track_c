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
	int flag_setting;	//参数是否被设置
	int height;			//设置的图像高度
	int width;			//设置的图像宽度
	int stuTrack_debugMsg_flag;					//调试信息输出等级
	int stuTrack_Draw_flag;						//是否绘制结果
	int stuTrack_direct_standard[4];			//四个顶点位置竖直方向在图像中的角度
	int stuTrack_stuWidth_standard[4];			//四个顶点位置学生在图像中所占的宽度
	int stuTrack_direct_range;					//起立时允许的角度偏离范围
	int stuTrack_standCount_threshold;			//判定为起立的帧数阈值
	int stuTrack_sitdownCount_threshold;		//判定为坐下的帧数阈值
	int stuTrack_moveDelayed_threshold;			//移动目标保持跟踪的延时，超过这个时间无运动，则放弃跟踪(单位：毫秒)
	double stuTrack_move_threshold;				//判定是移动目标的偏离阈值（比值）
	TrackPrarms_Point_t stuTrack_vertex[4];		//学生区域四个顶点位置
}StuITRACK_ClientParams_t;

//---------------------------------------------------------------------输出参数相关
#define RESULT_STUTRACK_NEWCHANGE_FLAG		(1<<30)
#define RESULT_STUTRACK_IF_NEWCHANGE(n)	((n & RESULT_STUTRACK_NEWCHANGE_FLAG)== RESULT_STUTRACK_NEWCHANGE_FLAG)	//判断是否有变化

//变化状态宏
#define RESULT_STUTRACK_NULL_FLAG		0
#define	RESULT_STUTRACK_STANDUP_FLAG	1
#define	RESULT_STUTRACK_SITDOWN_FLAG	2
#define	RESULT_STUTRACK_MOVE_FLAG		4
#define RESULT_STUTRACK_STOPMOVE_FLAG	8

#define RESULT_STUTRACK_IF_STANDUP(n)		((n & RESULT_STUTRACK_STANDUP_FLAG)== RESULT_STUTRACK_STANDUP_FLAG)	//判断是否有起立
#define RESULT_STUTRACK_IF_SITDOWN(n)		((n & RESULT_STUTRACK_SITDOWN_FLAG)== RESULT_STUTRACK_SITDOWN_FLAG)	//判断是否有坐下
#define RESULT_STUTRACK_IF_MOVE(n)			((n & RESULT_STUTRACK_MOVE_FLAG)== RESULT_STUTRACK_MOVE_FLAG)			//判断是否有移动目标
#define RESULT_STUTRACK_IF_STOPMOVE(n)		((n & RESULT_STUTRACK_STOPMOVE_FLAG)== RESULT_STUTRACK_STOPMOVE_FLAG)	//判断是否有移动目标停止运动

typedef struct _StuITRACK_OutParams
{
	int result_flag;							//当前帧变化状态
	unsigned int count_trackObj_stand;			//起立目标个数
	unsigned int count_trackObj_bigMove;		//移动目标个数
	TrackPrarms_Point_t stand_position;			//起立目标位置
	TrackPrarms_Point_t move_position;			//移动目标位置
	TrackPrarms_Size_t standObj_size;			//起立目标大小
	TrackPrarms_Size_t moveObj_size;			//移动目标大小
}StuITRACK_OutParams_t;
//-----------------------------------------------------------------------

#ifdef  __cplusplus  
}
#endif  /* end of __cplusplus */ 

#endif
