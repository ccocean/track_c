#include "stuTrack_track_img.h"
#include "itcTrack_draw_img.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <string.h>

#define TRACK_NULL_STATE		0
#define TRACK_STANDUP_STATE		1
#define TRACK_SITDOWN_STATE		(-1)
#define TRACK_MOVE_STATE		1
#define TRACK_BIG_STATE			2

#define TRACK_INTERSECT_EXPAND (-3)
static void stuTrack_filtrate_contours(StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p,Track_Contour_t** pContour)
{
	//轮廓筛选
	if (*pContour == NULL || inst == NULL || interior_params_p == NULL)
		return;

	int count_rect = 0;
	int *stuTrack_size_threshold = interior_params_p->stuTrack_size_threshold;
	Track_Rect_t *stuTrack_rect_arr = interior_params_p->stuTrack_rect_arr;
	Track_Contour_t	*Contour = *pContour;
	do
	{
		Track_Rect_t rect = Contour->rect;
		int centre_y = rect.y + rect.height;
		if (rect.width > stuTrack_size_threshold[centre_y] &&
			rect.height > stuTrack_size_threshold[centre_y] &&
			count_rect < MALLOC_ELEMENT_COUNT)					//筛选
		{
			*(stuTrack_rect_arr + count_rect) = rect;
			count_rect++;
		}
		Contour = (Track_Contour_t*)Contour->h_next;
	} while (Contour != *pContour);

	int i = 0, j = 0;
	for (i = 0; i < count_rect; i++)
	{
		for (j = i + 1; j < count_rect; j++)
		{
			if (track_intersect_rect(stuTrack_rect_arr + i, stuTrack_rect_arr + j, TRACK_INTERSECT_EXPAND))	//判断是否相交，如果相交则直接合并
			{
				count_rect--;
				*(stuTrack_rect_arr + j) = *(stuTrack_rect_arr + count_rect);
				j--;
			}
		}
	}
	interior_params_p->count_stuTrack_rect = count_rect;
}

static int stuTrack_matchingSatnd_ROI(StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p, Track_Rect_t roi)
{
	//匹配roi
	int *stuTrack_size_threshold = interior_params_p->stuTrack_size_threshold;
	int *stuTrack_direct_threshold = interior_params_p->stuTrack_direct_threshold;

	int stuTrack_direct_range = inst->stuTrack_direct_range;
	float stuTrack_move_threshold = inst->stuTrack_move_threshold;

	int standard_direct = stuTrack_direct_threshold[roi.x + (roi.width >> 1)];
	int direct = 0;
	//中心点
	int x = roi.x + (roi.width >> 1);
	int y = roi.y + (roi.height >> 1);
	unsigned int i = 0;
	int flag_ROI = track_calculateDirect_ROI((Itc_Mat_t *)interior_params_p->mhiMat, roi, &direct);
	if (flag_ROI == 1 && roi.height>roi.width)
	{
		if (interior_params_p->count_trackObj_stand > 0)
		{
			int min_ID = 0;
			int min_distance = INT_MAX;
			int distance = 0;
			int diff_x = 0;
			int diff_y = 0;
			for (i = 0; i < interior_params_p->count_trackObj_stand; i++)
			{
				diff_x = x - interior_params_p->stuTrack_stand[i].centre.x;
				diff_y = y - interior_params_p->stuTrack_stand[i].centre.y;
				distance = diff_x*diff_x + diff_y + diff_y;
				if (min_distance>distance)
				{
					min_distance = distance;
					min_ID = i;
				}
			}

			int threshold = (interior_params_p->stuTrack_stand[min_ID].roi.width * interior_params_p->stuTrack_stand[min_ID].roi.height) >> 3;
			Track_Rect_t _roi = roi;
			if (min_distance < threshold)
			{
				track_intersect_rect(&_roi, &(interior_params_p->stuTrack_stand[min_ID].roi), TRACK_INTERSECT_EXPAND);
				//_PRINTF("角度：原角度:%d,当前角度:%d，范围:%d\n", interior_params_p->stuTrack_stand[min_ID].direction, direct, stuTrack_direct_range);
				if ((abs(interior_params_p->stuTrack_stand[min_ID].direction - direct) <= stuTrack_direct_range))
				{
					interior_params_p->stuTrack_stand[min_ID].count_up++;
				}
				else
				{
					interior_params_p->stuTrack_stand[min_ID].count_up--;
				}
				interior_params_p->stuTrack_stand[min_ID].count_teack++;
				interior_params_p->stuTrack_stand[min_ID].flag_matching = TRUE;
				interior_params_p->stuTrack_stand[min_ID].direction = (interior_params_p->stuTrack_stand[min_ID].direction + direct) >> 1;
				interior_params_p->stuTrack_stand[min_ID].direction = ITC_IMAX(interior_params_p->stuTrack_stand[min_ID].direction, standard_direct - (stuTrack_direct_range >> 1));
				interior_params_p->stuTrack_stand[min_ID].direction = ITC_IMIN(interior_params_p->stuTrack_stand[min_ID].direction, standard_direct + (stuTrack_direct_range >> 1));
				interior_params_p->stuTrack_stand[min_ID].roi = _roi;
				interior_params_p->stuTrack_stand[min_ID].centre = itcPoint(_roi.x + (_roi.width >> 1), _roi.y + (_roi.height >> 1));
				interior_params_p->stuTrack_stand[min_ID].current_tClock = clock();
				return 1;
			}
		}

		if (abs(standard_direct - direct) < (stuTrack_direct_range + 5) && interior_params_p->count_trackObj_stand < MALLOC_ELEMENT_COUNT)
		{
			//add
			//_PRINTF("add stand：origin:%d,%d,size:%d,%d\n", x, y, roi.width, roi.height);
			direct = ITC_IMAX(direct, standard_direct - (stuTrack_direct_range >> 1));
			interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand].direction = ITC_IMIN(direct, standard_direct + (stuTrack_direct_range >> 1));
			interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand].count_teack = 1;
			interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand].count_up = 1;
			interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand].count_down = 0;
			interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand].flag_Stand = TRACK_NULL_STATE;
			interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand].flag_matching = FALSE;
			interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand].centre = itcPoint(x, y);
			interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand].roi = roi;
			interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand].start_tClock = interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand].current_tClock = clock();
			interior_params_p->count_trackObj_stand++;
			return 1;
		}
	}

	int intersect_flag = 0;
	for (i = 0; i < interior_params_p->count_trackObj_stand; i++)
	{
		intersect_flag = track_intersect_rect(&roi, &interior_params_p->stuTrack_stand[i].roi, -(roi.width >> 1));
	}
	if (intersect_flag == 0)
	{
		int centre_y = roi.y + (roi.height >> 1);
		int size_threshold1 = stuTrack_size_threshold[centre_y] + (stuTrack_size_threshold[centre_y] >> 2);
		int size_threshold2 = stuTrack_size_threshold[centre_y] + (stuTrack_size_threshold[centre_y] >> 1);
		if ((roi.width >  size_threshold1 && roi.height > size_threshold2))
		{
			if (interior_params_p->count_trackObj_bigMove > 0)
			{
				int k = -1;
				Track_Rect_t _roi = roi;
				for (i = 0; i < interior_params_p->count_trackObj_bigMove; i++)
				{
					if (track_intersect_rect(&_roi, &interior_params_p->stuTrack_bigMOveObj[i].roi, -(_roi.width >> 1)))
					{
						k = i;
						break;
					}
				}
				if (k >= 0)
				{
					interior_params_p->stuTrack_bigMOveObj[k].count_track++;
					interior_params_p->stuTrack_bigMOveObj[k].roi = roi;
					interior_params_p->stuTrack_bigMOveObj[k].current_position = itcPoint(_roi.x + (_roi.width >> 1), _roi.y + (_roi.height >> 1));
					interior_params_p->stuTrack_bigMOveObj[k].current_tClock = clock();
					return 2;
				}
			}
			if (interior_params_p->count_trackObj_bigMove < MALLOC_ELEMENT_COUNT)
			{
				//_PRINTF("add bigMove：origin:%d,%d,size:%d,%d\n", x, y, roi.width, roi.height);
				interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].count_track = 1;
				interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].flag_bigMove = TRACK_NULL_STATE;
				interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].dis_threshold = (int)(ITC_IMIN(roi.width, roi.height) * stuTrack_move_threshold);
				interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].roi = roi;
				interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].origin_position = interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].current_position = itcPoint(x, y);
				interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].start_tClock = interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].current_tClock = clock();
				interior_params_p->count_trackObj_bigMove++;
				return 2;
			}
		}
	}
	return 0;
}

#define RATIO_HENIGHTWIDTH_THRESHOLD (2.1)
static int stuTrack_judgeStand_ROI(StuITRACK_Params *inst, StuTrack_Stand_t track_stand)
{
	//判断是否起立
	int stuTrack_standCount_threshold = inst->stuTrack_standCount_threshold;
	double ratio_lengthWidth = (((double)track_stand.roi.height) / track_stand.roi.width);
	if (((track_stand.count_up > stuTrack_standCount_threshold && track_stand.count_teack > (stuTrack_standCount_threshold - 2))
		|| track_stand.count_up > (stuTrack_standCount_threshold + 10))
		&& (ratio_lengthWidth - RATIO_HENIGHTWIDTH_THRESHOLD) <= 0.0)
	{
		return 1;
	}
	return 0;
}

#define MOVETIME_DELETE_THRESHOLD	1000
#define STANDTIME_DELETE_THRESHOLD	300
static void stuTrack_analyze_ROI(StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p)
{
	//分析候选区域
	int *stuTrack_size_threshold = interior_params_p->stuTrack_size_threshold;
	int *stuTrack_direct_threshold = interior_params_p->stuTrack_direct_threshold;

	int stuTrack_direct_range = inst->stuTrack_direct_range;
	int stuTrack_sitdownCount_threshold = inst->stuTrack_sitdownCount_threshold;
	int stuTrack_moveDelayed_threshold = inst->stuTrack_moveDelayed_threshold;

	Itc_Mat_t *mhi = (Itc_Mat_t *)interior_params_p->mhiMat;

	int direct = 0;
	int standard_direct = 0;
	int flag_ROI = 0;
	unsigned int i = 0;
	for (i = 0; i < interior_params_p->count_trackObj_stand; i++)
	{
		if (interior_params_p->stuTrack_stand[i].flag_Stand != TRACK_STANDUP_STATE)
		{
			//检测有没有起立
			if (interior_params_p->stuTrack_stand[i].flag_matching == FALSE)
			{
				standard_direct = stuTrack_direct_threshold[interior_params_p->stuTrack_stand[i].centre.x];
				flag_ROI = track_calculateDirect_ROI(mhi, interior_params_p->stuTrack_stand[i].roi, &direct);
				if ((abs(interior_params_p->stuTrack_stand[i].direction - direct) < stuTrack_direct_range) && (flag_ROI == 1))
				{
					interior_params_p->stuTrack_stand[i].count_up++;
				}
			}
			//_PRINTF("判断：%d, %d\n", interior_params_p->stuTrack_stand[i].count_teack, interior_params_p->stuTrack_stand[i].count_up);
			if (stuTrack_judgeStand_ROI(inst, interior_params_p->stuTrack_stand[i]))	//确定是否站立
			{
				_PRINTF("stand up：origin:%d,%d,size:%d,%d\n", interior_params_p->stuTrack_stand[i].centre.x, interior_params_p->stuTrack_stand[i].centre.y, interior_params_p->stuTrack_stand[i].roi.width, interior_params_p->stuTrack_stand[i].roi.height);
				//设置起立的标记
				interior_params_p->result_flag |= STUTRACK_RETURN_STANDUP;
				interior_params_p->stuTrack_stand[i].flag_Stand = TRACK_STANDUP_STATE;
			}
		}
		else
		{
			//检测有没有坐下
			standard_direct = stuTrack_direct_threshold[interior_params_p->stuTrack_stand[i].centre.x];
			standard_direct = standard_direct > ITC_DEGREES ? standard_direct - ITC_DEGREES : standard_direct + ITC_DEGREES;		//计算与起立方向相反的角度
			flag_ROI = track_calculateDirect_ROI(mhi, interior_params_p->stuTrack_stand[i].roi, &direct);
			if ((abs(standard_direct - direct)< stuTrack_direct_range + 30) && (flag_ROI == 1))
			{
				interior_params_p->stuTrack_stand[i].count_down++;
				if (interior_params_p->stuTrack_stand[i].count_down>stuTrack_sitdownCount_threshold)
				{
					_PRINTF("sit down：origin:%d,%d,size:%d,%d\n", interior_params_p->stuTrack_stand[i].centre.x, interior_params_p->stuTrack_stand[i].centre.y, interior_params_p->stuTrack_stand[i].roi.width, interior_params_p->stuTrack_stand[i].roi.height);
					//设置坐下的标记
					interior_params_p->result_flag |= STUTRACK_RETURN_SITDOWN;
					interior_params_p->stuTrack_stand[i].flag_Stand = TRACK_SITDOWN_STATE;
					interior_params_p->stuTrack_stand[i].count_teack = 0;
					interior_params_p->stuTrack_stand[i].count_up = 0;
					interior_params_p->stuTrack_stand[i].count_down = 0;
					interior_params_p->stuTrack_stand[i].flag_matching = FALSE;
					continue;
				}
			}
		}
		
		if (interior_params_p->stuTrack_stand[i].flag_Stand != TRACK_STANDUP_STATE)
		{
			clock_t _time = clock() - interior_params_p->stuTrack_stand[i].current_tClock;
			if (_time > STANDTIME_DELETE_THRESHOLD)				//删除非站立roi
			{
				interior_params_p->stuTrack_stand[i] = interior_params_p->stuTrack_stand[--(interior_params_p->count_trackObj_stand)];
				i--;
				continue;
			}
		}
		interior_params_p->stuTrack_stand[i].flag_matching = FALSE;
	}

	//分析移动的目标
	for (i = 0; i < interior_params_p->count_trackObj_bigMove; i++)
	{
		clock_t _time = clock() - interior_params_p->stuTrack_bigMOveObj[i].current_tClock;
		if (_time > MOVETIME_DELETE_THRESHOLD)
		{
			//_PRINTF("delete bigMove:origin:%d,%d,current:%d,%d\n", interior_params_p->stuTrack_bigMOveObj[i].origin_position.x, interior_params_p->stuTrack_bigMOveObj[i].origin_position.y, interior_params_p->stuTrack_bigMOveObj[i].current_position.x, interior_params_p->stuTrack_bigMOveObj[i].current_position.y);
			if (interior_params_p->stuTrack_bigMOveObj[i].flag_bigMove != TRACK_NULL_STATE)
			{	
				//设置停止运动的标记
				interior_params_p->result_flag |= STUTRACK_RETURN_STOPMOVE;
			}
			//删除长时间不运动的目标
			interior_params_p->stuTrack_bigMOveObj[i] = interior_params_p->stuTrack_bigMOveObj[--(interior_params_p->count_trackObj_bigMove)];
			i--;
			continue;
		}
		if (interior_params_p->stuTrack_bigMOveObj[i].flag_bigMove == TRACK_NULL_STATE)
		{
			int diff_x = abs(interior_params_p->stuTrack_bigMOveObj[i].origin_position.x - interior_params_p->stuTrack_bigMOveObj[i].current_position.x);
			int diff_y = abs(interior_params_p->stuTrack_bigMOveObj[i].origin_position.y - interior_params_p->stuTrack_bigMOveObj[i].current_position.y);
			if (diff_x>interior_params_p->stuTrack_bigMOveObj[i].dis_threshold || diff_y>interior_params_p->stuTrack_bigMOveObj[i].dis_threshold)
			{
				_time = interior_params_p->stuTrack_bigMOveObj[i].current_tClock - interior_params_p->stuTrack_bigMOveObj[i].start_tClock;
				if (_time > stuTrack_moveDelayed_threshold)
				{
					//_PRINTF("find Move：origin:%d,%d,size:%d,%d\n", interior_params_p->stuTrack_bigMOveObj[i].origin_position.x, interior_params_p->stuTrack_bigMOveObj[i].origin_position.y, interior_params_p->stuTrack_bigMOveObj[i].roi.width, interior_params_p->stuTrack_bigMOveObj[i].roi.height);
					//设置移动目标的标记
					interior_params_p->result_flag |= STUTRACK_RETURN_MOVE;
					interior_params_p->stuTrack_bigMOveObj[i].flag_bigMove = TRACK_MOVE_STATE;
				}
			}
			else 
			{
				int centre_y = interior_params_p->stuTrack_bigMOveObj[i].roi.y + interior_params_p->stuTrack_bigMOveObj[i].roi.width;
				int size_threshold = stuTrack_size_threshold[centre_y] + stuTrack_size_threshold[centre_y];
				int size_threshold2 = size_threshold + stuTrack_size_threshold[centre_y];
				if ((interior_params_p->stuTrack_bigMOveObj[i].roi.width > size_threshold && interior_params_p->stuTrack_bigMOveObj[i].roi.height > size_threshold)
					|| interior_params_p->stuTrack_bigMOveObj[i].roi.height > size_threshold2)
				{
					//_PRINTF("find big：origin:%d,%d,size:%d,%d\n", interior_params_p->stuTrack_bigMOveObj[i].origin_position.x, interior_params_p->stuTrack_bigMOveObj[i].origin_position.y, interior_params_p->stuTrack_bigMOveObj[i].roi.width, interior_params_p->stuTrack_bigMOveObj[i].roi.height);
					interior_params_p->stuTrack_bigMOveObj[i].flag_bigMove = TRACK_BIG_STATE;
					interior_params_p->result_flag |= STUTRACK_RETURN_MOVE;//设置移动目标的标记
				}
			}
		}
	}
}

static void stuTrack_proStandDown_ROI(StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p)
{
	//匹配和分析候选区域
	unsigned int i = 0;
	for (i = 0; i < interior_params_p->count_stuTrack_rect; i++)
	{
		if (stuTrack_matchingSatnd_ROI(inst, interior_params_p, interior_params_p->stuTrack_rect_arr[i]))
		{
			interior_params_p->stuTrack_rect_arr[i] = interior_params_p->stuTrack_rect_arr[--(interior_params_p->count_stuTrack_rect)];
			i--;
		}
	}
	stuTrack_analyze_ROI(inst, interior_params_p);			//分析候选roi
}

#define TRACK_DRAWCOLOUR_SETTING \
Trcak_Colour_t move_colour		= colour_RGB2YUV(255,   0, 255);					\
Trcak_Colour_t big_colour		= colour_RGB2YUV(  0,   0, 255);					\
Trcak_Colour_t noBogMove_colour = colour_RGB2YUV(155, 155, 255);					\
Trcak_Colour_t line_colour		= colour_RGB2YUV(  0, 255,   0);					\
Trcak_Colour_t stand_colour		= colour_RGB2YUV(255,   0,   0);					\
Trcak_Colour_t sit_colour		= colour_RGB2YUV(127,   0,   0);					\
Trcak_Colour_t noStndSit_colour = colour_RGB2YUV(255, 255,   0)

static void stuTrack_drawShow_imgData(StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p, char* imageData)
{
	//画出结果
	unsigned int i = 0;
	Track_Size_t img_size = { inst->width, inst->height };
	TRACK_DRAWCOLOUR_SETTING;
	for (i = 0; i < interior_params_p->count_trackObj_bigMove; i++)
	{

		if (interior_params_p->stuTrack_bigMOveObj[i].flag_bigMove != TRACK_NULL_STATE)
		{
			if (interior_params_p->stuTrack_bigMOveObj[i].flag_bigMove == TRACK_MOVE_STATE)
			{
				track_draw_rectangle(imageData, &img_size, &(interior_params_p->stuTrack_bigMOveObj[i].roi), &move_colour);
			}
			else if (interior_params_p->stuTrack_bigMOveObj[i].flag_bigMove == TRACK_BIG_STATE)
			{
				track_draw_rectangle(imageData, &img_size, &(interior_params_p->stuTrack_bigMOveObj[i].roi), &big_colour);
			}
			track_draw_line(imageData, &img_size, &(interior_params_p->stuTrack_bigMOveObj[i].current_position), &(interior_params_p->stuTrack_bigMOveObj[i].origin_position), &line_colour);
		}
		else
		{
			track_draw_rectangle(imageData, &img_size, &(interior_params_p->stuTrack_bigMOveObj[i].roi), &noBogMove_colour);
		}
	}

	for (i = 0; i < interior_params_p->count_trackObj_stand; i++)
	{
		if (interior_params_p->stuTrack_stand[i].flag_Stand != TRACK_NULL_STATE)
		{
			if (interior_params_p->stuTrack_stand[i].flag_Stand == TRACK_STANDUP_STATE)
			{
				track_draw_rectangle(imageData, &img_size, &(interior_params_p->stuTrack_stand[i].roi), &stand_colour);
			}
			else if (interior_params_p->stuTrack_stand[i].flag_Stand == TRACK_SITDOWN_STATE)
			{
				track_draw_rectangle(imageData, &img_size, &(interior_params_p->stuTrack_stand[i].roi), &sit_colour);
			}
		}
		else
		{
			track_draw_rectangle(imageData, &img_size, &(interior_params_p->stuTrack_stand[i].roi), &noStndSit_colour);
		}
	}
}

static void stuTrack_reslut(StuITRACK_InteriorParams* interior_params_p, StuITRACK_OutParams_t* return_params)
{
	//填写返回结果结构体
	if (interior_params_p->result_flag != STUTRACK_RETURN_NULL)
	{
		_PRINTF("new change！\n");
		return_params->result_flag |= STUTRACK_NEWCHANGE_FLAG;
		return_params->result_flag = interior_params_p->result_flag;						//当前帧的变化
		return_params->count_trackObj_stand = interior_params_p->count_trackObj_stand;		//移动目标个数
		return_params->count_trackObj_bigMove = interior_params_p->count_trackObj_bigMove;	//起立目标个数
		if ((return_params->result_flag & STUTRACK_RETURN_MOVE) == STUTRACK_RETURN_MOVE)
		{
			//发现移动目标，将最新的目标位置返回
			return_params->move_position.x = interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove - 1].current_position.x;
			return_params->move_position.y = interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove - 1].current_position.y;
			return_params->moveObj_size.width = interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove - 1].roi.width;
			return_params->moveObj_size.height = interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove - 1].roi.height;
		}
		if ((return_params->result_flag & STUTRACK_RETURN_STANDUP) == STUTRACK_RETURN_STANDUP)
		{
			//发现起立目标，位置指向最新的站立区域
			return_params->stand_position.x = interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand - 1].centre.x;
			return_params->stand_position.y = interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand - 1].centre.y;
			return_params->standObj_size.width = interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand - 1].roi.width;
			return_params->standObj_size.height = interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand - 1].roi.height;
		}
	}
}

void stuTrack_initializeTrack(StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p)
{
	if (inst == NULL || interior_params_p == NULL)
	{
		interior_params_p->initialize_flag = FALSE;
		return;
	}
	stuTrack_stopTrack(inst, interior_params_p);
	interior_params_p->initialize_flag = FALSE;
	if (inst->height != STUTRACK_IMG_HEIGHT || inst->width != STUTRACK_IMG_WIDTH)
	{
		return;
	}
	//分配内存
	interior_params_p->currMat = itc_create_mat(inst->height, inst->width, ITC_8UC1);
	JUDEGE_POINTER_NULL(interior_params_p->currMat, ITC_RETURN);

	interior_params_p->lastMat = itc_create_mat(inst->height, inst->width, ITC_8UC1);
	JUDEGE_POINTER_NULL(interior_params_p->lastMat, ITC_RETURN);

	interior_params_p->mhiMat = itc_create_mat(inst->height, inst->width, ITC_8UC1);
	JUDEGE_POINTER_NULL(interior_params_p->mhiMat, ITC_RETURN);

	interior_params_p->maskMat = itc_create_mat(inst->height, inst->width, ITC_8UC1);
	JUDEGE_POINTER_NULL(interior_params_p->maskMat, ITC_RETURN);

	interior_params_p->stuTrack_storage = itcCreateMemStorage(0);
	JUDEGE_POINTER_NULL(interior_params_p->stuTrack_storage, ITC_RETURN);

	interior_params_p->stuTrack_stand = (StuTrack_Stand_t*)malloc(sizeof(StuTrack_Stand_t)* MALLOC_ELEMENT_COUNT);
	JUDEGE_POINTER_NULL(interior_params_p->stuTrack_stand, ITC_RETURN);

	interior_params_p->stuTrack_bigMOveObj = (StuTrack_BigMoveObj_t*)malloc(sizeof(StuTrack_BigMoveObj_t)* MALLOC_ELEMENT_COUNT);
	JUDEGE_POINTER_NULL(interior_params_p->stuTrack_bigMOveObj, ITC_RETURN);

	interior_params_p->stuTrack_rect_arr = (Track_Rect_t*)malloc(sizeof(Track_Rect_t)* MALLOC_ELEMENT_COUNT);
	JUDEGE_POINTER_NULL(interior_params_p->stuTrack_rect_arr, ITC_RETURN);

	interior_params_p->stuTrack_size_threshold = (int *)malloc(sizeof(int)* inst->height);
	JUDEGE_POINTER_NULL(interior_params_p->stuTrack_size_threshold, ITC_RETURN);

	interior_params_p->stuTrack_direct_threshold = (int *)malloc(sizeof(int)* inst->width);
	JUDEGE_POINTER_NULL(interior_params_p->stuTrack_direct_threshold, ITC_RETURN);

	//初始化自有的内部参数
	interior_params_p->_count = 0;
	interior_params_p->img_size = inst->height*inst->width;
	interior_params_p->count_trackObj_stand = 0;
	interior_params_p->count_trackObj_bigMove = 0;
	interior_params_p->count_stuTrack_rect = 0;
	memset(interior_params_p->stuTrack_stand, 0, sizeof(StuTrack_Stand_t)* MALLOC_ELEMENT_COUNT);
	memset(interior_params_p->stuTrack_bigMOveObj, 0, sizeof(StuTrack_BigMoveObj_t)* MALLOC_ELEMENT_COUNT);
	memset(interior_params_p->stuTrack_rect_arr, 0, sizeof(Track_Rect_t)* MALLOC_ELEMENT_COUNT);
	memset(interior_params_p->stuTrack_size_threshold, 0, sizeof(int)* inst->height);
	memset(interior_params_p->stuTrack_direct_threshold, 0, sizeof(int)* inst->width);
	
	int i = 0;
	if (inst->flag_setting == FALSE)
	{
		interior_params_p->img_size = STUTRACK_IMG_HEIGHT*STUTRACK_IMG_WIDTH;
		//默认的参数
		inst->stuTrack_move_threshold = 1.2;			//判定是移动目标的偏离阈值（比值）
		inst->stuTrack_standCount_threshold = 5;		//判定为起立的帧数阈值
		inst->stuTrack_sitdownCount_threshold = 5;		//判定为坐下的帧数阈值
		inst->stuTrack_moveDelayed_threshold = 500;
		inst->stuTrack_direct_range = 10;
		
		for (i = 0; i < inst->height; i++)
		{
			interior_params_p->stuTrack_size_threshold[i] = ITC_IMIN(ITC_IMAX(((-6 + (i >> 2))), 21), 78);
		}
		for (i = 0; i < inst->width / 2; i++)
		{
			int direct = (inst->width - i * 2) >> 5;
			interior_params_p->stuTrack_direct_threshold[i] = 270 - direct;
			interior_params_p->stuTrack_direct_threshold[inst->width - 1 - i] = 270 + direct;
		}
	}
	else
	{
		//
	}
	interior_params_p->initialize_flag = TRUE;
}

#define FRAME_DIFF_THRESHOLD	12
#define HMI_MASK_THERSHOLD		248
void stuTrack_process(StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p, StuITRACK_OutParams_t* return_params, char* imageData)
{
	if (imageData == NULL || return_params == NULL || interior_params_p == NULL || inst == NULL 
		|| interior_params_p->initialize_flag == FALSE)
	{	
		return;
	}
	memcpy(interior_params_p->currMat->data.ptr, imageData, interior_params_p->img_size);

	interior_params_p->result_flag = STUTRACK_RETURN_NULL;	//清空变化状态
	Track_Contour_t* firstContour = NULL;
	if (interior_params_p->_count>1)
	{
		itcClearMemStorage(interior_params_p->stuTrack_storage);
		track_update_MHI(interior_params_p->currMat, interior_params_p->lastMat, interior_params_p->mhiMat, FRAME_DIFF_THRESHOLD, interior_params_p->maskMat, HMI_MASK_THERSHOLD);
		track_find_contours(interior_params_p->maskMat, &firstContour, interior_params_p->stuTrack_storage);
		stuTrack_filtrate_contours(inst, interior_params_p,&firstContour);
		stuTrack_proStandDown_ROI(inst, interior_params_p);
	}
	interior_params_p->tempMat = interior_params_p->currMat;
	interior_params_p->currMat = interior_params_p->lastMat;
	interior_params_p->lastMat = interior_params_p->tempMat;
	interior_params_p->_count++;

	stuTrack_drawShow_imgData(inst,interior_params_p,imageData);	//绘制处理效果
	stuTrack_reslut(interior_params_p, return_params);				//填写返回结果
}

void stuTrack_stopTrack(StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p)
{
	if (inst == NULL || interior_params_p == NULL)
	{
		return;
	}

	if (interior_params_p->stuTrack_size_threshold != NULL)
	{
		free(interior_params_p->stuTrack_size_threshold);
		interior_params_p->stuTrack_size_threshold = NULL;
	}

	if (interior_params_p->stuTrack_direct_threshold != NULL)
	{
		free(interior_params_p->stuTrack_direct_threshold);
		interior_params_p->stuTrack_direct_threshold = NULL;
	}

	if (interior_params_p->stuTrack_stand != NULL)
	{
		free(interior_params_p->stuTrack_stand);
		interior_params_p->stuTrack_stand = NULL;
	}

	if (interior_params_p->stuTrack_bigMOveObj != NULL)
	{
		free(interior_params_p->stuTrack_bigMOveObj);
		interior_params_p->stuTrack_bigMOveObj = NULL;
	}

	if (interior_params_p->stuTrack_rect_arr != NULL)
	{
		free(interior_params_p->stuTrack_rect_arr);
		interior_params_p->stuTrack_rect_arr = NULL;
	}
	interior_params_p->count_trackObj_stand = 0;
	interior_params_p->count_trackObj_bigMove = 0;
	interior_params_p->count_stuTrack_rect = 0;

	itcReleaseMemStorage(&interior_params_p->stuTrack_storage);
	itc_release_mat(&interior_params_p->currMat);
	itc_release_mat(&interior_params_p->lastMat);
	itc_release_mat(&interior_params_p->mhiMat);
	itc_release_mat(&interior_params_p->maskMat);
}
