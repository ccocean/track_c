#include "stuTrack_track_img.h"
#include "itcTrack_draw_img.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define STATE_STUTRACK_NULL_FLAG	0
#define STATE_STUTRACK_STANDUP_FLAG	1
#define STATE_STUTRACK_SITDOWN_FLAG	(-1)
#define STATE_STUTRACK_MOVE_FLAG	1
#define STATE_STUTRACK_BIG_FLAG		2

#define EXPAND_STUTRACK_INTERSECT_RECT (-3)
static void stuTrack_filtrate_contours(StuITRACK_InteriorParams* interior_params_p, Track_Contour_t** pContour)
{
	ITC_FUNCNAME("FUNCNAME:stuTrack_filtrate_contours\n");
	//轮廓筛选
	if (*pContour == NULL || interior_params_p == NULL)
	{	
		return;
	}

	Track_Rect_t *stuTrack_rect_arr = interior_params_p->stuTrack_rect_arr;
	Track_Contour_t	*Contour = *pContour;
	int count_rect = 0;
	int *stuTrack_size_threshold = interior_params_p->stuTrack_size_threshold;
	do
	{
		Track_Rect_t rect = Contour->rect;
		int centre_y = rect.y + rect.height;
		if (rect.width > stuTrack_size_threshold[centre_y] &&
			rect.height > stuTrack_size_threshold[centre_y] &&
			count_rect < COUNT_STUTRACK_MALLOC_ELEMENT)					//筛选
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
			if (track_intersect_rect(stuTrack_rect_arr + i, stuTrack_rect_arr + j, EXPAND_STUTRACK_INTERSECT_RECT))	//判断是否相交，如果相交则直接合并
			{
				count_rect--;
				*(stuTrack_rect_arr + j) = *(stuTrack_rect_arr + count_rect);
				j--;
			}
		}
	}
	interior_params_p->count_stuTrack_rect = count_rect;
}

#define EXPADN_STURECK_ADDSATND_DIRECT_RANGE	10
static int stuTrack_matchingSatnd_ROI(StuITRACK_InteriorParams* interior_params_p, Track_Rect_t roi)
{
	ITC_FUNCNAME("FUNCNAME:stuTrack_matchingSatnd_ROI\n");
	//匹配roi
	int *stuTrack_size_threshold = interior_params_p->stuTrack_size_threshold;
	int *stuTrack_direct_threshold = interior_params_p->stuTrack_direct_threshold;
	int stuTrack_direct_range = interior_params_p->stuTrack_direct_range;
	double stuTrack_move_threshold = interior_params_p->stuTrack_move_threshold;
	StuTrack_Stand_t* stuTrack_stand = interior_params_p->stuTrack_stand;
	StuTrack_BigMoveObj_t* stuTrack_bigMOveObj = interior_params_p->stuTrack_bigMOveObj;

	int standard_direct = stuTrack_direct_threshold[roi.x + (roi.width >> 1)];
	int direct = 0;
	//中心点
	int x = roi.x + (roi.width >> 1);
	int y = roi.y + (roi.height >> 1);
	int i = 0;
	float dx = 0, dy = 0;
	int flag_ROI = track_calculateDirect_ROI((Itc_Mat_t *)interior_params_p->mhiMat, roi, &direct,&dx,&dy);

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
				diff_x = x - stuTrack_stand[i].centre.x;
				diff_y = y - stuTrack_stand[i].centre.y;
				distance = diff_x * diff_x + diff_y * diff_y;
				if (min_distance>distance)
				{
					min_distance = distance;
					min_ID = i;
				}
			}

			int threshold = (stuTrack_stand[min_ID].roi.width * stuTrack_stand[min_ID].roi.height) >> 3;
			Track_Rect_t _roi = roi;
			if (min_distance < threshold)
			{
				track_intersect_rect(&_roi, &(interior_params_p->stuTrack_stand[min_ID].roi), EXPAND_STUTRACK_INTERSECT_RECT);
				//_PRINTF("m角度：原角度:%d,当前角度:%d，范围:%d\n", stuTrack_stand[min_ID].direction, direct, stuTrack_direct_range);
				if ((abs(stuTrack_stand[min_ID].direction - direct) <= stuTrack_direct_range))
				{
					stuTrack_stand[min_ID].count_up++;
				}
				else
				{
					stuTrack_stand[min_ID].count_up--;
				}
				stuTrack_stand[min_ID].count_teack++;
				stuTrack_stand[min_ID].flag_matching = TRUE;
				stuTrack_stand[min_ID].direction = (stuTrack_stand[min_ID].direction + direct) >> 1;
				stuTrack_stand[min_ID].direction = ITC_IMAX(stuTrack_stand[min_ID].direction, standard_direct - (stuTrack_direct_range >> 1));
				stuTrack_stand[min_ID].direction = ITC_IMIN(stuTrack_stand[min_ID].direction, standard_direct + (stuTrack_direct_range >> 1));
				stuTrack_stand[min_ID].roi = _roi;
				stuTrack_stand[min_ID].centre = itcPoint(_roi.x + (_roi.width >> 1), _roi.y + (_roi.height >> 1));
				stuTrack_stand[min_ID].current_tClock = gettime();
				return 1;
			}
		}

		if (abs(standard_direct - direct) < (stuTrack_direct_range + EXPADN_STURECK_ADDSATND_DIRECT_RANGE) && interior_params_p->count_trackObj_stand < COUNT_STUTRACK_MALLOC_ELEMENT)
		{
			//add
			//_PRINTF("add stand：origin:%d,%d,size:%d,%d\n", x, y, roi.width, roi.height);
			direct = ITC_IMAX(direct, standard_direct - (stuTrack_direct_range >> 1));
			stuTrack_stand[interior_params_p->count_trackObj_stand].direction = ITC_IMIN(direct, standard_direct + (stuTrack_direct_range >> 1));
			stuTrack_stand[interior_params_p->count_trackObj_stand].count_teack = 1;
			stuTrack_stand[interior_params_p->count_trackObj_stand].count_up = 1;
			stuTrack_stand[interior_params_p->count_trackObj_stand].count_down = 0;
			stuTrack_stand[interior_params_p->count_trackObj_stand].flag_Stand = STATE_STUTRACK_NULL_FLAG;
			stuTrack_stand[interior_params_p->count_trackObj_stand].flag_matching = TRUE;
			stuTrack_stand[interior_params_p->count_trackObj_stand].centre = itcPoint(x, y);
			stuTrack_stand[interior_params_p->count_trackObj_stand].roi = roi;
			stuTrack_stand[interior_params_p->count_trackObj_stand].start_tClock = stuTrack_stand[interior_params_p->count_trackObj_stand].current_tClock = gettime();
			interior_params_p->count_trackObj_stand++;
			return 1;
		}
	}

	int intersect_flag = 0;
	for (i = 0; i < interior_params_p->count_trackObj_stand; i++)
	{
		intersect_flag = track_intersect_rect(&roi, &stuTrack_stand[i].roi, -(roi.width >> 1));
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
					//此处待优化
					if (track_intersect_rect(&_roi, &stuTrack_bigMOveObj[i].roi, -(_roi.width >> 1)))
					{
						k = i;
						break;
					}
				}
				if (k >= 0)
				{
					stuTrack_bigMOveObj[k].count_track++;
					stuTrack_bigMOveObj[k].roi = roi;
					stuTrack_bigMOveObj[k].current_position = itcPoint(_roi.x + (_roi.width >> 1), _roi.y + (_roi.height >> 1));
					stuTrack_bigMOveObj[k].current_tClock = gettime();
					return 2;
				}
			}
			if (interior_params_p->count_trackObj_bigMove < COUNT_STUTRACK_MALLOC_ELEMENT)
			{
				//_PRINTF("add bigMove：origin:%d,%d,size:%d,%d\n", x, y, roi.width, roi.height);
				stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].count_track = 1;
				stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].flag_bigMove = STATE_STUTRACK_NULL_FLAG;
				stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].dis_threshold = (int)(ITC_IMIN(roi.width, roi.height) * stuTrack_move_threshold);
				stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].roi = roi;
				stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].origin_position = stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].current_position = itcPoint(x, y);
				stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].start_tClock = stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove].current_tClock = gettime();
				interior_params_p->count_trackObj_bigMove++;
				return 2;
			}
		}
	}
	return 0;
}

#define EXPADN_STURECK_STAND_COUNTUP_THRESHOLD	10
#define EXPADN_STURECK_STAND_COUNTTAK_THRESHOLD	(-2)
#define THRESHOLD_STURECK_RATIO_HENIGHTWIDTH	(2.1)
static BOOL stuTrack_judgeStand_ROI(StuITRACK_InteriorParams* interior_params_p, StuTrack_Stand_t track_stand)
{
	//判断是否起立
	int stuTrack_standCount_threshold = interior_params_p->stuTrack_standCount_threshold;
	double ratio_lengthWidth = (((double)track_stand.roi.height) / track_stand.roi.width);
	if (((track_stand.count_up > stuTrack_standCount_threshold && track_stand.count_teack > (stuTrack_standCount_threshold + EXPADN_STURECK_STAND_COUNTTAK_THRESHOLD))
		|| track_stand.count_up > (stuTrack_standCount_threshold + EXPADN_STURECK_STAND_COUNTUP_THRESHOLD))
		&& (ratio_lengthWidth - THRESHOLD_STURECK_RATIO_HENIGHTWIDTH) <= ITC_DBL_EPSILON)
	{
		return TRUE;
	}
	return FALSE;
}

#define THRESHOLD_STURECK_MOVETIME_DELETE_TIME	1000
#define THRESHOLD_STURECK_STANDTIME_DELETE_TIME	300
#define EXPADN_STURECK_SITDOWN_DIRECT	30
static void stuTrack_analyze_ROI(StuITRACK_InteriorParams* interior_params_p)
{
	ITC_FUNCNAME("FUNCNAME:stuTrack_analyze_ROI\n");
	//分析候选区域
	int *stuTrack_size_threshold = interior_params_p->stuTrack_size_threshold;
	int *stuTrack_direct_threshold = interior_params_p->stuTrack_direct_threshold;
	int stuTrack_direct_range = interior_params_p->stuTrack_direct_range;
	int stuTrack_sitdownCount_threshold = interior_params_p->stuTrack_sitdownCount_threshold;
	unsigned int stuTrack_moveDelayed_threshold = (unsigned int)interior_params_p->stuTrack_moveDelayed_threshold;
	StuTrack_Stand_t* stuTrack_stand = interior_params_p->stuTrack_stand;
	StuTrack_BigMoveObj_t* stuTrack_bigMOveObj = interior_params_p->stuTrack_bigMOveObj;
	Itc_Mat_t *mhi = (Itc_Mat_t *)interior_params_p->mhiMat;

	int direct = 0;
	int standard_direct = 0;
	float dx = 0, dy = 0;
	int flag_ROI = 0;
	int i = 0;
	for (i = 0; i < interior_params_p->count_trackObj_stand; i++)
	{
		if (stuTrack_stand[i].flag_Stand != STATE_STUTRACK_STANDUP_FLAG)
		{
			//检测有没有起立
			if (stuTrack_stand[i].flag_matching == FALSE)
			{
				standard_direct = stuTrack_direct_threshold[interior_params_p->stuTrack_stand[i].centre.x];
				flag_ROI = track_calculateDirect_ROI(mhi, stuTrack_stand[i].roi, &direct,&dx,&dy);
				//_PRINTF("a角度：原角度:%d,当前角度:%d，范围:%d\n", stuTrack_stand[i].direction, direct, stuTrack_direct_range);
				if ((flag_ROI == 1) && ((abs(stuTrack_stand[i].direction - direct)) <= stuTrack_direct_range))
				{
					stuTrack_stand[i].count_up++;
				}
			}
			//_PRINTF("判断：%d, %d\n", stuTrack_stand[i].count_teack, stuTrack_stand[i].count_up);
			if (stuTrack_judgeStand_ROI(interior_params_p, interior_params_p->stuTrack_stand[i]))	//确定是否站立
			{
				_PRINTF("stand up：origin:%d,%d,size:%d,%d\n", stuTrack_stand[i].centre.x, stuTrack_stand[i].centre.y, stuTrack_stand[i].roi.width, stuTrack_stand[i].roi.height);
				//设置起立的标记
				interior_params_p->result_flag |= RESULT_STUTRACK_STANDUP_FLAG;
				stuTrack_stand[i].flag_Stand = STATE_STUTRACK_STANDUP_FLAG;
			}
		}
		else
		{
			//检测有没有坐下
			standard_direct = stuTrack_direct_threshold[stuTrack_stand[i].centre.x];
			standard_direct = (standard_direct > ITC_180DEGREE) ? (standard_direct - ITC_180DEGREE) : (standard_direct + ITC_180DEGREE);		//计算与起立方向相反的角度
			flag_ROI = track_calculateDirect_ROI(mhi, stuTrack_stand[i].roi, &direct,&dx,&dy);
			if ((flag_ROI == 1) && ((abs(standard_direct - direct))<= stuTrack_direct_range + EXPADN_STURECK_SITDOWN_DIRECT))
			{
				stuTrack_stand[i].count_down++;
				if (interior_params_p->stuTrack_stand[i].count_down>stuTrack_sitdownCount_threshold)
				{
					_PRINTF("sit down：origin:%d,%d,size:%d,%d\n", stuTrack_stand[i].centre.x, stuTrack_stand[i].centre.y, stuTrack_stand[i].roi.width, stuTrack_stand[i].roi.height);
					//设置坐下的标记
					interior_params_p->result_flag |= RESULT_STUTRACK_SITDOWN_FLAG;
					stuTrack_stand[i].flag_Stand = STATE_STUTRACK_SITDOWN_FLAG;
					stuTrack_stand[i].count_teack = 0;
					stuTrack_stand[i].count_up = 0;
					stuTrack_stand[i].count_down = 0;
					stuTrack_stand[i].flag_matching = FALSE;
					continue;
				}
			}
		}
		
		if (stuTrack_stand[i].flag_Stand != STATE_STUTRACK_STANDUP_FLAG)
		{
			unsigned int _time = gettime() - stuTrack_stand[i].current_tClock;
			if (_time > THRESHOLD_STURECK_STANDTIME_DELETE_TIME)				//删除非站立roi
			{
				stuTrack_stand[i] = stuTrack_stand[--(interior_params_p->count_trackObj_stand)];
				i--;
				continue;
			}
		}
		stuTrack_stand[i].flag_matching = FALSE;
	}

	//分析移动的目标
	for (i = 0; i < interior_params_p->count_trackObj_bigMove; i++)
	{
		unsigned int _time = gettime() - stuTrack_bigMOveObj[i].current_tClock;
		if (_time > THRESHOLD_STURECK_MOVETIME_DELETE_TIME)
		{
			//_PRINTF("delete bigMove:origin:%d,%d,current:%d,%d\n", stuTrack_bigMOveObj[i].origin_position.x, stuTrack_bigMOveObj[i].origin_position.y, stuTrack_bigMOveObj[i].current_position.x, stuTrack_bigMOveObj[i].current_position.y);
			if (stuTrack_bigMOveObj[i].flag_bigMove != STATE_STUTRACK_NULL_FLAG)
			{	
				//设置停止运动的标记
				interior_params_p->result_flag |= RESULT_STUTRACK_STOPMOVE_FLAG;
			}
			//删除长时间不运动的目标
			stuTrack_bigMOveObj[i] = stuTrack_bigMOveObj[--(interior_params_p->count_trackObj_bigMove)];
			i--;
			continue;
		}
		if (interior_params_p->stuTrack_bigMOveObj[i].flag_bigMove == STATE_STUTRACK_NULL_FLAG)
		{
			int diff_x = abs(stuTrack_bigMOveObj[i].origin_position.x - stuTrack_bigMOveObj[i].current_position.x);
			int diff_y = abs(stuTrack_bigMOveObj[i].origin_position.y - stuTrack_bigMOveObj[i].current_position.y);
			if (diff_x>stuTrack_bigMOveObj[i].dis_threshold || diff_y>stuTrack_bigMOveObj[i].dis_threshold)
			{
				_time = stuTrack_bigMOveObj[i].current_tClock - stuTrack_bigMOveObj[i].start_tClock;
				if (_time > stuTrack_moveDelayed_threshold)
				{
					_PRINTF("find Move：origin:%d,%d,size:%d,%d\n", stuTrack_bigMOveObj[i].origin_position.x, stuTrack_bigMOveObj[i].origin_position.y, stuTrack_bigMOveObj[i].roi.width, stuTrack_bigMOveObj[i].roi.height);
					//设置移动目标的标记
					interior_params_p->result_flag |= RESULT_STUTRACK_MOVE_FLAG;
					stuTrack_bigMOveObj[i].flag_bigMove = STATE_STUTRACK_MOVE_FLAG;
				}
			}
			else 
			{
				int centre_y = stuTrack_bigMOveObj[i].roi.y + stuTrack_bigMOveObj[i].roi.width;
				int size_threshold = stuTrack_size_threshold[centre_y] + stuTrack_size_threshold[centre_y];
				int size_threshold2 = size_threshold + stuTrack_size_threshold[centre_y];
				if ((stuTrack_bigMOveObj[i].roi.width > size_threshold && stuTrack_bigMOveObj[i].roi.height > size_threshold)
					|| stuTrack_bigMOveObj[i].roi.height > size_threshold2)
				{
					_PRINTF("find big：origin:%d,%d,size:%d,%d\n", stuTrack_bigMOveObj[i].origin_position.x, stuTrack_bigMOveObj[i].origin_position.y, stuTrack_bigMOveObj[i].roi.width, stuTrack_bigMOveObj[i].roi.height);
					stuTrack_bigMOveObj[i].flag_bigMove = STATE_STUTRACK_BIG_FLAG;
					interior_params_p->result_flag |= RESULT_STUTRACK_MOVE_FLAG;//设置移动目标的标记
				}
			}
		}
	}
}

static void stuTrack_proStandDown_ROI(StuITRACK_InteriorParams* interior_params_p)
{
	ITC_FUNCNAME("FUNCNAME:stuTrack_proStandDown_ROI\n");
	//匹配和分析候选区域
	int i = 0;
	for (i = 0; i < interior_params_p->count_stuTrack_rect; i++)
	{
		stuTrack_matchingSatnd_ROI(interior_params_p, interior_params_p->stuTrack_rect_arr[i]);
	}
	stuTrack_analyze_ROI(interior_params_p);			//分析候选roi
}

static void stuTrack_drawShow_imgData(StuITRACK_InteriorParams* interior_params_p, itc_uchar* imageData, itc_uchar* bufferuv)
{
	ITC_FUNCNAME("FUNCNAME:stuTrack_drawShow_imgData\n");
	//if (interior_params_p->stuTrack_Draw_flag == FALSE)
	//{	
	//	return;
	//}

	//画出结果
	int i = 0;
	Track_Size_t *srcimg_size = &interior_params_p->srcimg_size;	//原始图像大小
	StuTrack_Stand_t* stuTrack_stand = interior_params_p->stuTrack_stand;
	StuTrack_BigMoveObj_t* stuTrack_bigMOveObj = interior_params_p->stuTrack_bigMOveObj;

	Track_Rect_t *rect;
	Track_Point_t *current_position;
	Track_Point_t *origin_position;

#ifdef _WIN32
	int YUV420_type = TRACK_DRAW_YUV420P;
#endif
#ifndef _WIN32
	int YUV420_type = TRACK_DRAW_YUV420SP;
#endif

	//for (i = 0; i < interior_params_p->count_stuTrack_rect; i++)
	//{
	//	rect = &stuTrack_rect_arr[i];
	//	track_draw_rectangle(imageData, bufferuv, srcimg_size, rect, &interior_params_p->yellow_colour, YUV420_type);
	//}

	for (i = 0; i < interior_params_p->count_trackObj_bigMove; i++)
	{
		rect = &stuTrack_bigMOveObj[i].roi;
		current_position = &stuTrack_bigMOveObj[i].current_position;
		origin_position = &stuTrack_bigMOveObj[i].origin_position;
		if (interior_params_p->stuTrack_bigMOveObj[i].flag_bigMove != STATE_STUTRACK_NULL_FLAG)
		{
			if (interior_params_p->stuTrack_bigMOveObj[i].flag_bigMove == STATE_STUTRACK_MOVE_FLAG)
			{
				track_draw_rectangle(imageData, bufferuv, srcimg_size, rect, &interior_params_p->pink_colour, YUV420_type);
			}
			else if (interior_params_p->stuTrack_bigMOveObj[i].flag_bigMove == STATE_STUTRACK_BIG_FLAG)
			{
				track_draw_rectangle(imageData, bufferuv, srcimg_size, rect, &interior_params_p->blue_colour, YUV420_type);
			}
			track_draw_line(imageData, bufferuv, srcimg_size, current_position, origin_position, &interior_params_p->green_colour, YUV420_type);//
		}
		else
		{
			track_draw_rectangle(imageData, bufferuv, srcimg_size, rect, &interior_params_p->lilac_colour, YUV420_type);
		}
	}

	for (i = 0; i < interior_params_p->count_trackObj_stand; i++)
	{
		rect = &stuTrack_stand[i].roi;
		if (interior_params_p->stuTrack_stand[i].flag_Stand != STATE_STUTRACK_NULL_FLAG)
		{
			if (interior_params_p->stuTrack_stand[i].flag_Stand == STATE_STUTRACK_STANDUP_FLAG)
			{
				track_draw_rectangle(imageData, bufferuv, srcimg_size, rect, &interior_params_p->red_colour, YUV420_type);
			}
			else if (interior_params_p->stuTrack_stand[i].flag_Stand == STATE_STUTRACK_SITDOWN_FLAG)
			{
				track_draw_rectangle(imageData, bufferuv, srcimg_size, rect, &interior_params_p->dullred_colour, YUV420_type);
			}
		}
		else
		{
			track_draw_rectangle(imageData, bufferuv, srcimg_size, rect, &interior_params_p->yellow_colour, YUV420_type);
		}
	}

	//Track_Rect_t *stuTrack_rect_arr = interior_params_p->stuTrack_rect_arr;
	//for (i = 0; i < interior_params_p->count_stuTrack_rect; i++)
	//{
	//	int direct;
	//	track_calculateDirect_ROI((Itc_Mat_t *)interior_params_p->mhiMat, interior_params_p->stuTrack_rect_arr[i], &direct);
	//	_PRINTF("角度：%d\n", direct);
	//	int x1 = 20 * cos(direct*ITC_PI / ITC_180DEGREE);
	//	int y1 = 20 * sin(direct*ITC_PI / ITC_180DEGREE);
	//	Track_Point_t pt1 = { 0, 0 };
	//	pt1.x = interior_params_p->stuTrack_rect_arr[i].x + interior_params_p->stuTrack_rect_arr[i].width / 2;
	//	pt1.y = interior_params_p->stuTrack_rect_arr[i].y + interior_params_p->stuTrack_rect_arr[i].height / 2;
	//	Track_Point_t pt2 = { 0, 0 };
	//	pt2.x = pt1.x + x1;
	//	pt2.y = pt1.y + y1;
	//	track_draw_line(imageData, bufferuv, srcimg_size, &pt1, &pt2, &interior_params_p->green_colour, YUV420_type);
	//	track_draw_point(imageData, bufferuv, srcimg_size, &pt1, &interior_params_p->red_colour, YUV420_type);
	//}
}

static void stuTrack_reslut(StuITRACK_InteriorParams* interior_params_p, StuITRACK_OutParams_t* return_params)
{
	ITC_FUNCNAME("FUNCNAME:stuTrack_reslut\n");
	//填写返回结果结构体
	if (interior_params_p->result_flag != RESULT_STUTRACK_NULL_FLAG)
	{
		_PRINTF("new change！\n");
		return_params->result_flag = interior_params_p->result_flag | RESULT_STUTRACK_NEWCHANGE_FLAG;		//当前帧的变化,设置有变化的标记
		return_params->count_trackObj_stand = interior_params_p->count_trackObj_stand;		//移动目标个数
		return_params->count_trackObj_bigMove = interior_params_p->count_trackObj_bigMove;	//起立目标个数
		if (RESULT_STUTRACK_IF_MOVE(return_params->result_flag) && interior_params_p->count_trackObj_bigMove>0)
		{
			//发现移动目标，将最新的目标位置返回
			return_params->move_position.x = interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove - 1].current_position.x;
			return_params->move_position.y = interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove - 1].current_position.y;
			return_params->moveObj_size.width = interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove - 1].roi.width;
			return_params->moveObj_size.height = interior_params_p->stuTrack_bigMOveObj[interior_params_p->count_trackObj_bigMove - 1].roi.height;
		}
		if (RESULT_STUTRACK_IF_STANDUP(return_params->result_flag) && interior_params_p->count_trackObj_stand>0)
		{
			//发现起立目标，位置指向最新的站立区域
			return_params->stand_position.x = interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand - 1].centre.x;
			return_params->stand_position.y = interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand - 1].centre.y;
			return_params->standObj_size.width = interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand - 1].roi.width;
			return_params->standObj_size.height = interior_params_p->stuTrack_stand[interior_params_p->count_trackObj_stand - 1].roi.height;
		}
	}
}

static void stuTrack_Copy_matData(StuITRACK_InteriorParams* interior_params_p, itc_uchar* srcData)
{
	ITC_FUNCNAME("FUNCNAME:stuTrack_resizeCopy_matData\n");
	int y = 0;
	int height = interior_params_p->img_size.height;
	int dst_step = interior_params_p->img_size.width;
	int src_step = interior_params_p->srcimg_size.width;
	if (dst_step > src_step)
	{
		_PRINTF("The image cache size error!\n");
		return;
	}

	itc_uchar* dst_p = interior_params_p->currMat->data.ptr;
	for (y = 0; y < height; y++)
	{
		memcpy(dst_p, srcData, sizeof(itc_uchar)* dst_step);
		dst_p += dst_step;
		srcData += src_step;
	}
}
#define CHECH_STURRACK_RESULT_OK 0
#define CHECH_STURRACK_RESULT_ERROR1 1
#define CHECH_STURRACK_RESULT_ERROR2 2
#define CHECH_STURRACK_RESULT_ERROR3 3
#define CHECH_STURRACK_RESULT_ERROR4 4
#define CHECH_STURRACK_RESULT_ERROR5 5
#define CHECH_STURRACK_RESULT_ERROR6 6
#define CHECH_STURRACK_RESULT_ERROR7 7
static int stuTrack_check_clientParams(const StuITRACK_ClientParams_t* clientParams_p)
{

	if (clientParams_p->stuTrack_vertex[0].x < 0 ||
		clientParams_p->stuTrack_vertex[1].x < 0 ||
		clientParams_p->stuTrack_vertex[2].x < 0 ||
		clientParams_p->stuTrack_vertex[3].x < 0 ||
		clientParams_p->stuTrack_vertex[0].y < 0 ||
		clientParams_p->stuTrack_vertex[1].y < 0 ||
		clientParams_p->stuTrack_vertex[2].y < 0 ||
		clientParams_p->stuTrack_vertex[3].y < 0)
	{
		return CHECH_STURRACK_RESULT_ERROR1;
	}

	if (clientParams_p->stuTrack_move_threshold < 0.01)
	{
		return CHECH_STURRACK_RESULT_ERROR2;
	}

	if (clientParams_p->stuTrack_standCount_threshold < 0)
	{
		return CHECH_STURRACK_RESULT_ERROR3;
	}

	if (clientParams_p->stuTrack_sitdownCount_threshold < 0)
	{
		return CHECH_STURRACK_RESULT_ERROR4;
	}

	if (clientParams_p->stuTrack_moveDelayed_threshold < 0)
	{
		return CHECH_STURRACK_RESULT_ERROR5;
	}

	if (clientParams_p->stuTrack_direct_range < 0)
	{
		return CHECH_STURRACK_RESULT_ERROR6;
	}

	if (clientParams_p->height <= 0 || clientParams_p->width <= 0)
	{
		return CHECH_STURRACK_RESULT_ERROR7;
	}

	return CHECH_STURRACK_RESULT_OK;
}

BOOL stuTrack_initializeTrack(const StuITRACK_Params * inst, StuITRACK_InteriorParams* interior_params_p)
{
	if (inst == NULL || interior_params_p == NULL)
	{
		return FALSE;
	}
	stuTrack_stopTrack(inst, interior_params_p);

	if (inst->systemParams.callbackmsg_func != NULL)
	{	interior_params_p->callbackmsg_func = inst->systemParams.callbackmsg_func;}
	else
	{	interior_params_p->callbackmsg_func = printf;}
	interior_params_p->srcimg_size.width = 0;
	interior_params_p->srcimg_size.height = 0;
	interior_params_p->initialize_flag = FALSE;

	double size_threshold_a, size_threshold_b;
	double direct_threshold_a, direct_threshold_b;
	if ((inst->clientParams.flag_setting == TRUE) && (stuTrack_check_clientParams(&inst->clientParams) == CHECH_STURRACK_RESULT_OK))
	{
		//非默认参数
		int y1 = (inst->clientParams.stuTrack_vertex[0].y + inst->clientParams.stuTrack_vertex[1].y) / 2;
		int y2 = (inst->clientParams.stuTrack_vertex[2].y + inst->clientParams.stuTrack_vertex[3].y) / 2;
		if (y1 == y2)
		{
			_PRINTF("The input parameter error:y1 == y2!\n");
			return FALSE;
		}
		int width1 = (inst->clientParams.stuTrack_stuWidth_standard[0] + inst->clientParams.stuTrack_stuWidth_standard[1]) / 2;
		int width2 = (inst->clientParams.stuTrack_stuWidth_standard[2] + inst->clientParams.stuTrack_stuWidth_standard[3]) / 2;
		size_threshold_a = ((double)(width1 - width2)) / (y1 - y2);
		size_threshold_b = width1 - size_threshold_a*y1;

		int x1 = (inst->clientParams.stuTrack_vertex[0].x + inst->clientParams.stuTrack_vertex[2].x) / 2;
		int x2 = (inst->clientParams.stuTrack_vertex[1].x + inst->clientParams.stuTrack_vertex[3].x) / 2;
		if (x1 == x2)
		{
			_PRINTF("The input parameter error:x1 == x2!\n");
			return FALSE;
		}
		int direct1 = (ITC_NORM_ANGLE360(inst->clientParams.stuTrack_direct_standard[0]) + ITC_NORM_ANGLE360(inst->clientParams.stuTrack_direct_standard[2])) / 2;
		int direct2 = (ITC_NORM_ANGLE360(inst->clientParams.stuTrack_direct_standard[1]) + ITC_NORM_ANGLE360(inst->clientParams.stuTrack_direct_standard[3])) / 2;
		direct_threshold_a = ((double)(direct1 - direct2)) / (x1 - x2);
		direct_threshold_b = direct1 - direct_threshold_a*direct_threshold_a;

		interior_params_p->stuTrack_debugMsg_flag			= inst->clientParams.stuTrack_debugMsg_flag;
		interior_params_p->stuTrack_Draw_flag				= inst->clientParams.stuTrack_Draw_flag;
		interior_params_p->stuTrack_move_threshold			= inst->clientParams.stuTrack_move_threshold;			//判定是移动目标的偏离阈值（相对于自身宽度的比值）
		interior_params_p->stuTrack_standCount_threshold	= inst->clientParams.stuTrack_standCount_threshold;		//判定为起立的帧数阈值
		interior_params_p->stuTrack_sitdownCount_threshold	= inst->clientParams.stuTrack_sitdownCount_threshold;	//判定为坐下的帧数阈值
		interior_params_p->stuTrack_moveDelayed_threshold	= inst->clientParams.stuTrack_moveDelayed_threshold;
		interior_params_p->stuTrack_direct_range			= inst->clientParams.stuTrack_direct_range;
		interior_params_p->img_size.width	= (inst->clientParams.width  + ITC_IMAGE_ALIGN - 1)&~(ITC_IMAGE_ALIGN - 1);		//对齐到8位
		interior_params_p->img_size.height	= (inst->clientParams.height + ITC_IMAGE_ALIGN - 1)&~(ITC_IMAGE_ALIGN - 1);
	}
	else
	{
		//默认的参数
		size_threshold_a	= A_STUTRACK_SIZE_THRESHOLD_PARAMS;
		size_threshold_b	= B_STUTRACK_SIZE_THRESHOLD_PARAMS;
		direct_threshold_a	= A_STUTRACK_DIRECT_THRESHOLD_PARAMS;
		direct_threshold_b	= B_STUTRACK_DIRECT_THRESHOLD_PARAMS;
		interior_params_p->stuTrack_debugMsg_flag			= 1;
		interior_params_p->stuTrack_Draw_flag				= TRUE;
		interior_params_p->stuTrack_move_threshold			= THRESHOLD_STUTRACK_MOVE_DEFALUT_PARAMS;			//判定是移动目标的偏离阈值（相对于自身宽度的比值）
		interior_params_p->stuTrack_standCount_threshold	= THRESHOLD_STUTRACK_STANDCOUNT_DEFALUT_PARAMS;		//判定为起立的帧数阈值
		interior_params_p->stuTrack_sitdownCount_threshold	= THRESHOLD_STUTRACK_SITDOWNCOUNT_DEFALUT_PARAMS;	//判定为坐下的帧数阈值
		interior_params_p->stuTrack_moveDelayed_threshold	= THRESHOLD_STUTRACK_MOVEDELAYED_DEFALUT_PARAMS;
		interior_params_p->stuTrack_direct_range			= RANGE_STUTRACK_STANDDIRECT_DEFALUT_PARAMS;
		interior_params_p->img_size.width					= WIDTH_STUTRACK_IMG_;
		interior_params_p->img_size.height					= HEIGHT_STUTRACK_IMG_;
	}

	//初始化自有的内部统计参数
	interior_params_p->_count = 0;
	interior_params_p->count_trackObj_stand = 0;
	interior_params_p->count_trackObj_bigMove = 0;
	interior_params_p->count_stuTrack_rect = 0;

	//分配内存
	interior_params_p->currMat = itc_create_mat(interior_params_p->img_size.height, interior_params_p->img_size.width, ITC_8UC1);
	JUDEGE_STUREACK_IF_NULL(interior_params_p->currMat, FALSE);

	interior_params_p->lastMat = itc_create_mat(interior_params_p->img_size.height, interior_params_p->img_size.width, ITC_8UC1);
	JUDEGE_STUREACK_IF_NULL(interior_params_p->lastMat, FALSE);

	interior_params_p->mhiMat = itc_create_mat(interior_params_p->img_size.height, interior_params_p->img_size.width, ITC_8UC1);
	JUDEGE_STUREACK_IF_NULL(interior_params_p->mhiMat, FALSE);

	interior_params_p->maskMat = itc_create_mat(interior_params_p->img_size.height, interior_params_p->img_size.width, ITC_8UC1);
	JUDEGE_STUREACK_IF_NULL(interior_params_p->maskMat, FALSE);

	interior_params_p->stuTrack_storage = itcCreateMemStorage(0);
	JUDEGE_STUREACK_IF_NULL(interior_params_p->stuTrack_storage, FALSE);

	interior_params_p->stuTrack_stand = (StuTrack_Stand_t*)itcAlloc(sizeof(StuTrack_Stand_t)* COUNT_STUTRACK_MALLOC_ELEMENT);
	JUDEGE_STUREACK_IF_NULL(interior_params_p->stuTrack_stand, FALSE);

	interior_params_p->stuTrack_bigMOveObj = (StuTrack_BigMoveObj_t*)itcAlloc(sizeof(StuTrack_BigMoveObj_t)* COUNT_STUTRACK_MALLOC_ELEMENT);
	JUDEGE_STUREACK_IF_NULL(interior_params_p->stuTrack_bigMOveObj, FALSE);

	interior_params_p->stuTrack_rect_arr = (Track_Rect_t*)itcAlloc(sizeof(Track_Rect_t)* COUNT_STUTRACK_MALLOC_ELEMENT);
	JUDEGE_STUREACK_IF_NULL(interior_params_p->stuTrack_rect_arr, FALSE);

	interior_params_p->stuTrack_size_threshold = (int *)itcAlloc(sizeof(int)* interior_params_p->img_size.height);
	JUDEGE_STUREACK_IF_NULL(interior_params_p->stuTrack_size_threshold, FALSE);

	interior_params_p->stuTrack_direct_threshold = (int *)itcAlloc(sizeof(int)* interior_params_p->img_size.width);
	JUDEGE_STUREACK_IF_NULL(interior_params_p->stuTrack_direct_threshold, FALSE);

	memset(interior_params_p->stuTrack_stand, 0, sizeof(StuTrack_Stand_t)* COUNT_STUTRACK_MALLOC_ELEMENT);
	memset(interior_params_p->stuTrack_bigMOveObj, 0, sizeof(StuTrack_BigMoveObj_t)* COUNT_STUTRACK_MALLOC_ELEMENT);
	memset(interior_params_p->stuTrack_rect_arr, 0, sizeof(Track_Rect_t)* COUNT_STUTRACK_MALLOC_ELEMENT);
	memset(interior_params_p->stuTrack_size_threshold, 0, sizeof(int)* interior_params_p->img_size.height);
	memset(interior_params_p->stuTrack_direct_threshold, 0, sizeof(int)* interior_params_p->img_size.width);

	int x = 0;
	for (x = 0; x < interior_params_p->img_size.width; x++)
	{
		//初始化标准起立角度
		interior_params_p->stuTrack_direct_threshold[x] = (int)(COMPUTER_STUTRACK_DIRECT_THRESHOLD_PARAMS(x, direct_threshold_a, direct_threshold_b) + 0.5);
	}
	int y = 0;
	for (y = 0; y < interior_params_p->img_size.height; y++)
	{
		//初始化筛选尺寸阈值
		interior_params_p->stuTrack_size_threshold[y] = (int)(COMPUTER_STUTRACK_SIZE_THRESHOLD_PARAMS(y, size_threshold_a, size_threshold_b) + 0.5);
	}
	
	//初始化绘制颜色
	interior_params_p->pink_colour = colour_RGB2YUV(255, 0, 255);	/*粉红*/
	interior_params_p->blue_colour = colour_RGB2YUV(0, 0, 255);		/*纯蓝*/
	interior_params_p->lilac_colour = colour_RGB2YUV(155, 155, 255);/*淡紫*/
	interior_params_p->green_colour = colour_RGB2YUV(0, 255, 0);	/*纯绿*/
	interior_params_p->red_colour = colour_RGB2YUV(255, 0, 0);		/*纯红*/
	interior_params_p->dullred_colour = colour_RGB2YUV(127, 0, 0);	/*暗红*/
	interior_params_p->yellow_colour = colour_RGB2YUV(255, 255, 0);	/*纯黄*/

	interior_params_p->initialize_flag = TRUE;
	return TRUE;
}

#define THRESHOLD_STUTRACK_FRAME_DIFF	12
#define THERSHOLD_STUTRAKC_HMI_MASK		248
void stuTrack_process(const StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p, StuITRACK_OutParams_t* return_params, char* imageData, char* bufferuv)
{
	if (imageData == NULL || return_params == NULL || interior_params_p == NULL || inst == NULL )
	{
		return;
	}
	else if (interior_params_p->initialize_flag == FALSE)
	{
		_PRINTF("Failed to initialize!\n");
		return;
	}
	ITC_FUNCNAME("FUNCNAME:stuTrack_process\n");
	if (interior_params_p->srcimg_size.width == 0 || interior_params_p->srcimg_size.height == 0)																									 
	{																																																 
		if (inst->systemParams.nsrcWidth == 0 || inst->systemParams.nsrcHeight == 0)																												 
		{																																															 
			_PRINTF("The original image size error!\n");																																			 
			return;																																													 
		}																																															 
		interior_params_p->srcimg_size.width = inst->systemParams.nsrcWidth;																														 
		interior_params_p->srcimg_size.height = inst->systemParams.nsrcHeight;									
	}

	stuTrack_Copy_matData(interior_params_p, (itc_uchar*)imageData);
	interior_params_p->result_flag = RESULT_STUTRACK_NULL_FLAG;	//清空变化状态
	Track_Contour_t* firstContour = NULL;
	if (interior_params_p->_count>1)
	{
		itcClearMemStorage(interior_params_p->stuTrack_storage);
		track_update_MHI(interior_params_p->currMat, interior_params_p->lastMat, interior_params_p->mhiMat, THRESHOLD_STUTRACK_FRAME_DIFF, interior_params_p->maskMat, THERSHOLD_STUTRAKC_HMI_MASK);
		track_find_contours(interior_params_p->maskMat, &firstContour, interior_params_p->stuTrack_storage);
		stuTrack_filtrate_contours(interior_params_p,&firstContour);
		stuTrack_proStandDown_ROI(interior_params_p);
	}

	ITC_SWAP(interior_params_p->currMat, interior_params_p->lastMat, interior_params_p->tempMat);
	interior_params_p->_count++;

	stuTrack_drawShow_imgData(interior_params_p, (itc_uchar*)imageData, (itc_uchar*)bufferuv);	//绘制处理效果
	stuTrack_reslut(interior_params_p, return_params);									//填写返回结果
}

void stuTrack_stopTrack(const StuITRACK_Params *inst, StuITRACK_InteriorParams* interior_params_p)
{
	if (inst == NULL || interior_params_p == NULL)
	{
		return;
	}
	ITC_FUNCNAME("FUNCNAME:stuTrack_stopTrack\n");

	if (interior_params_p->stuTrack_size_threshold != NULL)
	{
		itcFree_(interior_params_p->stuTrack_size_threshold);
		interior_params_p->stuTrack_size_threshold = NULL;
	}

	if (interior_params_p->stuTrack_direct_threshold != NULL)
	{
		itcFree_(interior_params_p->stuTrack_direct_threshold);
		interior_params_p->stuTrack_direct_threshold = NULL;
	}

	if (interior_params_p->stuTrack_stand != NULL)
	{
		itcFree_(interior_params_p->stuTrack_stand);
		interior_params_p->stuTrack_stand = NULL;
	}

	if (interior_params_p->stuTrack_bigMOveObj != NULL)
	{
		itcFree_(interior_params_p->stuTrack_bigMOveObj);
		interior_params_p->stuTrack_bigMOveObj = NULL;
	}

	if (interior_params_p->stuTrack_rect_arr != NULL)
	{
		itcFree_(interior_params_p->stuTrack_rect_arr);
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
	interior_params_p->initialize_flag = FALSE;
}
