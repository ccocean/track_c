#include "tch_track.h"

//static Track_Point_t pos_1 = { -1 };
//static Track_Point_t pos_2 = { -1 };

static void tchTrack_Copy_matData(Tch_Data_t* datas, itc_uchar* srcData)
{
	//ITC_FUNCNAME("FUNCNAME:stuTrack_resizeCopy_matData\n");
	int y = 0;
	int tch_step = datas->g_tchWin.width;
	int blk_step = datas->g_blkWin.width;
	int src_step = datas->sysData.width;
	if (tch_step > src_step)
	{
		//_PRINTF("The image cache size error!\n");
		datas->sysData.callbackmsg_func("The image cache size error!\n");
		return;
	}

	if (blk_step > src_step)
	{
		//_PRINTF("The image cache size error!\n");
		datas->sysData.callbackmsg_func("The image cache size error!\n");
		return;
	}

	itc_uchar* dst_blk = datas->currMatBlk->data.ptr;
	itc_uchar* src_blk = srcData + src_step*datas->g_blkWin.y + datas->g_blkWin.x;
	for (y = 0; y < datas->g_blkWin.height; y++)
	{
		memcpy(dst_blk, src_blk, sizeof(itc_uchar)* blk_step);
		dst_blk += blk_step;
		src_blk += src_step;
	}

	itc_uchar* dst_tch = datas->currMatTch->data.ptr;
	itc_uchar* src_tch = srcData + src_step*datas->g_tchWin.y + datas->g_tchWin.x;
	for (y = 0; y < datas->g_tchWin.height; y++)
	{
		memcpy(dst_tch, src_tch, sizeof(itc_uchar)* tch_step);
		dst_tch += tch_step;
		src_tch += src_step;
	}
}

int tch_trackInit(Tch_Data_t *data)
{
	if (!data)
	{
		data->sysData.callbackmsg_func("Tch_Data_t err.");
		return -1;
	}
	data->track_pos_width = data->g_frameSize.width / data->numOfPos;

	data->tch_lastStatus = RETURN_TRACK_TCH_NULL;

	
	data->g_posIndex = -1;
	data->g_prevPosIndex = -1;
	//tch_pos = &g_posIndex;

	data->g_isMulti = 0;
	data->g_isOnStage = 0;
	data->g_count = 0;

	data->lastRectNum = -1;

	data->slideTimer.start = 0;
	data->slideTimer.finish = 0;
	data->slideTimer.deltaTime = 0;

	data->tch_timer.start = 0;
	data->tch_timer.finish = 0;
	data->tch_timer.deltaTime = 0;

	data->outsideTimer.start = 0;
	data->outsideTimer.finish = 0;
	data->outsideTimer.deltaTime = 0;

	data->pos_slide.width = (data->numOfSlide - 1) / 2;
	data->pos_slide.center = -1;
	data->pos_slide.left = -1;
	data->pos_slide.right = -1;

	data->cam_pos = calloc(data->numOfPos, sizeof(Tch_CamPosition_t));
	Tch_CamPosition_t *ptr = data->cam_pos;

	int cnt = 0;
	int i = 0;
	while (cnt<data->numOfPos)
	{
		ptr->index = i / data->track_pos_width;
		ptr->left_pixel = i;
		ptr->right_pixel = i + data->track_pos_width;
		i += data->track_pos_width;
		ptr++;
		cnt++;
	}
	
	ptr = NULL;

	//data->srcMat = itc_create_mat(data->g_frameSize.height, data->g_frameSize.width, ITC_8UC1);

	data->prevMatTch = itc_create_mat(data->g_tchWin.height, data->g_tchWin.width, ITC_8UC1);
	data->prevMatBlk = itc_create_mat(data->g_blkWin.height, data->g_blkWin.width, ITC_8UC1);
	data->tempMatTch = NULL;
	data->tempMatBlk = NULL;

	data->currMatTch = itc_create_mat(data->g_tchWin.height, data->g_tchWin.width, ITC_8UC1);
	data->mhiMatTch = itc_create_mat(data->g_tchWin.height, data->g_tchWin.width, ITC_8UC1);
	data->maskMatTch = itc_create_mat(data->g_tchWin.height, data->g_tchWin.width, ITC_8UC1);
	data->currMatBlk = itc_create_mat(data->g_blkWin.height, data->g_blkWin.width, ITC_8UC1);
	data->mhiMatBlk = itc_create_mat(data->g_blkWin.height, data->g_blkWin.width, ITC_8UC1);
	data->maskMatBlk = itc_create_mat(data->g_blkWin.height, data->g_blkWin.width, ITC_8UC1);

	data->storage = itcCreateMemStorage(0);
	data->storageTch = itcCreateChildMemStorage(data->storage);
	data->storageBlk = itcCreateChildMemStorage(data->storage);

	//data->tch_queue = InitQueue();
	//data->callbackmsg_func = printf;

	return 0;
}

static void tchTrack_drawShow_imgData(Tch_Data_t* interior_params, itc_uchar* imageData, itc_uchar* bufferuv, Track_Rect_t *rect, Track_Colour_t *color)
{
	Track_Size_t srcimg_size = { interior_params->sysData.width, interior_params ->sysData.height};	//原始图像大小
#ifdef _WIN32
	int YUV420_type = TRACK_DRAW_YUV420P;
#endif
#ifndef _WIN32
	int YUV420_type = TRACK_DRAW_YUV420SP;
#endif
	track_draw_rectangle(imageData, bufferuv, &srcimg_size, rect, color, YUV420_type);
}


#define TRACK_MAINTAIN_TIME 2000//维护返回值的时间，2秒钟返回一个结果
static int tch_return_maintain(Tch_Timer_t *tch_timer, int in)
{
	int out;
	if (in == TRUE)
	{
		tch_timer->deltaTime = 0;
		tch_timer->start = gettime();
		out = TRUE;
		return out;
	}
	else
	{
		tch_timer->finish = gettime();
		tch_timer->deltaTime = tch_timer->finish - tch_timer->start;
		if (tch_timer->deltaTime>TRACK_MAINTAIN_TIME)
		{
			tch_timer->deltaTime = 0;
			tch_timer->start = gettime();
			out = TRUE;
		}
		else
		{
			out = FALSE;
		}
		return out;
	}
}

static void tch_updatePosition(Tch_Data_t *data)
{
	//更新位置
	data->g_prevPosIndex = data->g_posIndex;
	//pos_1 = data->center;
	if (data->g_posIndex == data->pos_slide.right || data->g_posIndex == data->pos_slide.left)//当人物走到滑框的边缘，就更新滑框，相当于调全景移动云台相机，修改为走到边缘前一个预置位就更新镜头
	{
		if (data->g_posIndex < data->pos_slide.width)//左端
		{
			if (data->g_posIndex == 0)//在最左端时维护时间
			{
				data->slideTimer.finish = gettime();
				data->slideTimer.deltaTime += data->slideTimer.finish - data->slideTimer.start;
				data->slideTimer.start = data->slideTimer.finish;
			}
			else//否则更新时间
			{
				data->slideTimer.finish = 0;
				data->slideTimer.deltaTime = 0;
				data->slideTimer.start = gettime();
			}
			data->pos_slide.center = data->pos_slide.width;
			data->pos_slide.left = data->pos_slide.center - data->pos_slide.width;
			data->pos_slide.right = data->pos_slide.center + data->pos_slide.width;
		}
		else if (data->g_posIndex > data->numOfPos - 1 - data->pos_slide.width)//右端
		{
			if (data->g_posIndex == data->numOfPos - 1)//在最右端时维护时间
			{
				data->slideTimer.finish = gettime();
				data->slideTimer.deltaTime += data->slideTimer.finish - data->slideTimer.start;
				data->slideTimer.start = data->slideTimer.finish;
			}
			else//否则更新时间
			{
				data->slideTimer.finish = 0;
				data->slideTimer.deltaTime = 0;
				data->slideTimer.start = gettime();
			}
			data->pos_slide.center = data->numOfPos - 1 - data->pos_slide.width;
			data->pos_slide.left = data->pos_slide.center - data->pos_slide.width;
			data->pos_slide.right = data->pos_slide.center + data->pos_slide.width;
		}
		else
		{
			data->pos_slide.center = data->g_posIndex;
			data->pos_slide.left = data->pos_slide.center - data->pos_slide.width;
			data->pos_slide.right = data->pos_slide.center + data->pos_slide.width;

			data->slideTimer.finish = 0;
			data->slideTimer.deltaTime = 0;
			data->slideTimer.start = gettime();
		}
	}
	else if (data->pos_slide.left > data->g_posIndex || data->g_posIndex > data->pos_slide.right)
	{
		//当追踪到的目标已经在上一帧滑框的外面则立刻更新滑框
		if (data->g_posIndex < data->pos_slide.width)
		{
			data->pos_slide.center = data->pos_slide.width;
			data->pos_slide.left = data->pos_slide.center - data->pos_slide.width;
			data->pos_slide.right = data->pos_slide.center + data->pos_slide.width;
		}
		else if (data->g_posIndex > data->numOfPos - 1 - data->pos_slide.width)
		{
			data->pos_slide.center = data->numOfPos - 1 - data->pos_slide.width;
			data->pos_slide.left = data->pos_slide.center - data->pos_slide.width;
			data->pos_slide.right = data->pos_slide.center + data->pos_slide.width;
		}
		else
		{
			data->pos_slide.center = data->g_posIndex;
			data->pos_slide.left = data->pos_slide.center - data->pos_slide.width;
			data->pos_slide.right = data->pos_slide.center + data->pos_slide.width;
		}
		data->slideTimer.finish = 0;
		data->slideTimer.deltaTime = 0;
		data->slideTimer.start = gettime();
	}
	else//在特写镜头范围内则维护时间
	{
		data->slideTimer.finish = gettime();
		data->slideTimer.deltaTime += data->slideTimer.finish - data->slideTimer.start;
		data->slideTimer.start = data->slideTimer.finish;
	}
}

//void static tch_updateTchPosition(Tch_Data_t *data)
//{
//	int i;
//	Tch_CamPosition_t *ptr;
//	ptr = data->cam_pos;
//	int pos1, pos2;
//	//获取当前运动框体所在的预置位
//	for (i = 0; i < data->numOfPos; i++)
//	{
//		if (data->g_prevPosIndex>0)
//		{
//			if (ptr->left_pixel <= data->center1.x&&data->center1.x <= ptr->right_pixel)
//			{
//				pos1 = ptr->index;
//			}
//			if (ptr->left_pixel <= data->center2.x&&data->center2.x <= ptr->right_pixel)
//			{
//				pos2 = ptr->index;
//			}
//			ptr++;
//		}
//		else
//		{
//			break;
//		}
//	}
//	ptr = NULL;
//	if (pos1 != pos2)
//	{
//		int a = abs(data->g_posIndex - pos1);
//		int b = abs(data->g_posIndex - pos2);
//	}
//	else
//		return;
//	
//}

//static int tch_direct_analysis(Tch_Data_t *data)
//{
//	int i = 0;
//	int count = 0;
//	PNode pnode = data->tch_queue->front;
//	Item pitem;
//	for (i = 0; i < data->tch_queue->size; i++)
//	{
//		if (TRACK_OUTSIDE_ANGLE_DN < pnode->data&&pnode->data<TRACK_OUTSIDE_ANGLE_UP)
//		{
//			count++;
//		}
//		pnode = pnode->next;
//	}
//	DeQueue(data->tch_queue, &pitem);
//	return (count >(data->tch_queue->size / 2)) ? 1 : 0;
//}

#define PRESET_ALIGN 10
#define TRACK_OUTSIDE_TIME 1000
int tch_track(itc_uchar *src, itc_uchar* pUV, TeaITRACK_Params *params, Tch_Data_t *data, Tch_Result_t *res)
//int tch_track(IplImage *src, TeaITRACK_Params *params, Tch_Data_t *data, Tch_Result_t *res)
{

	if (src==NULL || pUV==NULL|| params==NULL || data==NULL || res==NULL)
	{
		return -2;
	}
	res->pos = -1;
	res->status = RETURN_TRACK_TCH_NULL;
	int i = 0, j = 0;


	if (data->g_count>0)
	{
		ITC_SWAP(data->currMatTch, data->prevMatTch, data->tempMatTch);
		ITC_SWAP(data->currMatBlk, data->prevMatBlk, data->tempMatBlk);
	}
	tchTrack_Copy_matData(data, src);

#ifdef _WIN32
	tchTrack_drawShow_imgData(data, src, pUV, &data->g_tchWin, &data->red_colour);
	tchTrack_drawShow_imgData(data, src, pUV, &data->g_blkWin, &data->red_colour);
#endif

	int s_contourRectTch = 0;//老师的轮廓数目
	int s_contourRectBlk = 0;//板书的轮廓数目
	Track_Rect_t s_rectsTch[100];
	Track_Rect_t s_rectsBlk[100];
	Track_Rect_t s_bigRects[100];//筛选出来的大面积运动物体
	int s_maxdist = -1;//比较多个面积
	int s_rectCnt = 0;
	int isChange = -1;

	if (data->g_count>0)
	{
		data->g_count++;
		if (data->g_count>1000000)
		{
			data->g_count = 1;
		}
		//g_time = (double)cvGetTickCount();
		track_update_MHI(data->currMatTch, data->prevMatTch, data->mhiMatTch, 10, data->maskMatTch, 242);
		Track_Contour_t *contoursTch = NULL;
		itcClearMemStorage(data->storageTch);
		track_find_contours(data->maskMatTch, &contoursTch, data->storageTch);
		s_contourRectTch = track_filtrate_contours(&contoursTch, 20, s_rectsTch);

		track_update_MHI(data->currMatBlk, data->prevMatBlk, data->mhiMatBlk, 10, data->maskMatBlk, 235);
		Track_Contour_t *contoursBlk = NULL;
		itcClearMemStorage(data->storageBlk);
		track_find_contours(data->maskMatBlk, &contoursBlk, data->storageBlk);
		s_contourRectBlk = track_filtrate_contours(&contoursBlk, 20, s_rectsBlk);
		Track_Rect_t drawRect;

		if (s_contourRectBlk > 0)
		{
			for (i = 0; i < s_contourRectBlk;i++)
			{
				drawRect.x = s_rectsBlk[i].x + data->g_blkWin.x;
				drawRect.y = s_rectsBlk[i].y + data->g_blkWin.y;
				drawRect.width = s_rectsBlk[i].width;
				drawRect.height = s_rectsBlk[i].height;
				tchTrack_drawShow_imgData(data, src, pUV, &drawRect, &data->green_colour);
			}
			res->status = RETURN_TRACK_TCH_BLACKBOARD;
			isChange = (res->status != data->tch_lastStatus);
			data->tch_lastStatus = RETURN_TRACK_TCH_BLACKBOARD;
			res->pos = data->pos_slide.center+PRESET_ALIGN;
			isChange = tch_return_maintain(&data->tch_timer, isChange);

			for (i = 0; i < s_contourRectTch; i++)
			{
				if (params->threshold.targetArea < s_rectsTch[i].width*s_rectsTch[i].height)
				{
					s_bigRects[s_rectCnt] = s_rectsTch[i];
					s_rectCnt++;
				}
			}

			if (1 == s_rectCnt)
			{
				//获取运动方向
				int direct = -1;
				direct = tch_calculateDirect_TCH(data->mhiMatTch, s_bigRects[0]);
				if (direct > -1)
				{
					data->center = itcPoint(s_bigRects[0].x + (s_bigRects[0].width >> 1), s_bigRects[0].y + (s_bigRects[0].height >> 1));
					drawRect.x = s_bigRects[0].x + data->g_tchWin.x;
					drawRect.y = s_bigRects[0].y + data->g_tchWin.y;
					drawRect.width = s_bigRects[0].width;
					drawRect.height = s_bigRects[0].height;
					tchTrack_drawShow_imgData(data, src, pUV, &drawRect, &data->yellow_colour);

					Tch_CamPosition_t *ptr;
					ptr = data->cam_pos;
					//获取当前运动框体所在的预置位
					for (i = 0; i < data->numOfPos; i++)
					{
						if (ptr->left_pixel <= data->center.x&&data->center.x <= ptr->right_pixel)
						{
							if (data->g_prevPosIndex == -1)
							{
								data->g_prevPosIndex = ptr->index;
							}
							data->g_posIndex = ptr->index;
							break;
						}
						ptr++;
					}
					ptr = NULL;
				}
				if ((abs(data->g_prevPosIndex - data->g_posIndex) > data->pos_slide.width + 1) || (data->center.y > params->threshold.outside))
				{
					data->g_posIndex = data->g_prevPosIndex;
				}
				else
				{
					tch_updatePosition(data);
					res->pos = data->pos_slide.center + PRESET_ALIGN;
				}
			}
			return isChange;
		}

		//比较多个Rect之间x坐标的距离
		for (i = 0; i < s_contourRectTch; i++)
		{
			if (params->threshold.targetArea<s_rectsTch[i].width*s_rectsTch[i].height)
			{
				s_bigRects[s_rectCnt] = s_rectsTch[i];
				s_rectCnt++;
			}
		}
		if (0 == s_rectCnt)
		{
			if (data->tch_lastStatus == RETURN_TRACK_TCH_MOVEOUTVIEW)
			{
				data->slideTimer.finish = gettime();
				data->slideTimer.deltaTime += data->slideTimer.finish - data->slideTimer.start;
				data->slideTimer.start = data->slideTimer.finish;
				if ((data->slideTimer.deltaTime) > params->threshold.stand)
				{
					res->pos = data->pos_slide.center + PRESET_ALIGN;
					res->status = RETURN_TRACK_TCH_MOVEINVIEW;
					isChange = (res->status != data->tch_lastStatus);
					data->tch_lastStatus = RETURN_TRACK_TCH_MOVEINVIEW;
					isChange = tch_return_maintain(&data->tch_timer, isChange);
					data->lastRectNum = s_rectCnt;
					return isChange;
				}
				else
				{
					res->pos = data->pos_slide.center + PRESET_ALIGN;
					res->status = data->tch_lastStatus;
					isChange = (res->status != data->tch_lastStatus);
					isChange = tch_return_maintain(&data->tch_timer, isChange);
					data->lastRectNum = s_rectCnt;
					return isChange;
				}
			}
			else if (data->tch_lastStatus == RETURN_TRACK_TCH_MULITY)
			{
				data->slideTimer.start = gettime();
				data->slideTimer.deltaTime = 0;
				res->pos = data->pos_slide.center + PRESET_ALIGN;
				res->status = data->tch_lastStatus;
				isChange = (res->status != data->tch_lastStatus);
				isChange = tch_return_maintain(&data->tch_timer, isChange);
				data->lastRectNum = s_rectCnt;
				return isChange;
			}
			else
			{
				res->pos = data->pos_slide.center + PRESET_ALIGN;
				res->status = data->tch_lastStatus;
				isChange = (res->status != data->tch_lastStatus);
				isChange = tch_return_maintain(&data->tch_timer, isChange);
				data->lastRectNum = s_rectCnt;
				return isChange;
			}
			
		}
		else if (s_rectCnt>1)
		{
			s_maxdist = -1;
			for (i = 0; i < s_rectCnt; i++)
			{
				drawRect.x = s_bigRects[i].x + data->g_tchWin.x;
				drawRect.y = s_bigRects[i].y + data->g_tchWin.y;
				drawRect.width = s_bigRects[i].width;
				drawRect.height = s_bigRects[i].height;
				tchTrack_drawShow_imgData(data, src, pUV, &drawRect, &data->yellow_colour);
				for (j = i + 1; j < s_rectCnt; j++)
				{
					if (abs(s_bigRects[i].x - s_bigRects[j].x)>s_maxdist)
					{
						s_maxdist = abs(s_bigRects[i].x - s_bigRects[j].x);
					}
					else
						continue;
				}
			}
			if (s_maxdist > data->track_pos_width)
			{
				//printf("=========");
				//data->g_isMulti = 1;
 				/*data->center1 = itcPoint(s_bigRects[0].x + (s_bigRects[0].width >> 1), s_bigRects[0].y + (s_bigRects[0].height >> 1));
 				data->center2 = itcPoint(s_bigRects[1].x + (s_bigRects[1].width >> 1), s_bigRects[1].y + (s_bigRects[1].height >> 1));*/
				res->status = RETURN_TRACK_TCH_MULITY;
				isChange = (res->status != data->tch_lastStatus);
				data->tch_lastStatus = RETURN_TRACK_TCH_MULITY;
				res->pos = -1;
				data->slideTimer.start = gettime();
				data->slideTimer.deltaTime = 0;
				isChange = tch_return_maintain(&data->tch_timer, isChange);
				data->lastRectNum = s_rectCnt;
				return isChange;
			}
		}
		else
		{
			//获取运动方向
			int direct = -1;
			float dx = 0, dy = 0;
			//direct = tch_calculateDirect_TCH(data->mhiMatTch, s_bigRects[0]);
			track_calculateDirect_ROI(data->mhiMatTch, s_bigRects[0], &direct,&dx,&dy);
			//printf("角度： %d\r\n", direct);
			if (direct>-1)
			{
				data->center = itcPoint(s_bigRects[0].x + (s_bigRects[0].width >> 1), s_bigRects[0].y + (s_bigRects[0].height >> 1));
				drawRect.x = s_bigRects[0].x + data->g_tchWin.x;
				drawRect.y = s_bigRects[0].y + data->g_tchWin.y;
				drawRect.width = s_bigRects[0].width;
				drawRect.height = s_bigRects[0].height;
				tchTrack_drawShow_imgData(data, src, pUV, &drawRect, &data->yellow_colour);
				
				Tch_CamPosition_t *ptr;
				ptr = data->cam_pos;
				//获取当前运动框体所在的预置位
				for (i = 0; i < data->numOfPos; i++)
				{
					if (ptr->left_pixel <= data->center.x&&data->center.x <= ptr->right_pixel)
					{
						if (data->g_prevPosIndex==-1)
						{
							data->g_prevPosIndex = ptr->index;
							data->g_posIndex = ptr->index;
							if (data->tch_lastStatus == RETURN_TRACK_TCH_OUTSIDE)
							{
								data->outsideTimer.finish = gettime();
								data->outsideTimer.deltaTime = data->outsideTimer.finish - data->outsideTimer.start;
								if (data->outsideTimer.deltaTime>TRACK_OUTSIDE_TIME&&data->lastRectNum==0)
								{
									break;
								}
								else
								{
									data->outsideTimer.start = 0;
									data->outsideTimer.finish = 0;
									data->outsideTimer.deltaTime = 0;

									tch_updatePosition(data);
									data->slideTimer.deltaTime += params->threshold.stand + 100;
									res->status = RETURN_TRACK_TCH_MOVEINVIEW;
									res->pos = data->pos_slide.center + PRESET_ALIGN;
									isChange = 0;
									data->tch_lastStatus = RETURN_TRACK_TCH_MOVEINVIEW;
									data->lastRectNum = s_rectCnt;
									return  isChange;
								}
							}
							else
								break;
						}
						else
						{
							data->g_posIndex = ptr->index;
							break;
						}
					}
					ptr++;
				}
				ptr = NULL;

				
				//printf("x: %d,  y:  %d\r\n", pos_1.x,pos_1.y);
				//如果获取的位置与上一次位置相差很大，则认为是多目标
				//printf("last position:%d,   current position:%d\r\n", data->g_prevPosIndex, data->g_posIndex);
				if (abs(data->g_prevPosIndex - data->g_posIndex) > data->pos_slide.width + 1)
				{
					//data->g_isMulti = 1;
					data->g_posIndex = data->g_prevPosIndex;//maintain the last position
					//data->g_prevPosIndex = data->g_posIndex;
					res->status = RETURN_TRACK_TCH_MULITY;
					isChange = (res->status != data->tch_lastStatus);
					data->tch_lastStatus = RETURN_TRACK_TCH_MULITY;
					res->pos = data->pos_slide.center + PRESET_ALIGN;
					data->slideTimer.deltaTime = 0;
					data->slideTimer.start = gettime();
					isChange = tch_return_maintain(&data->tch_timer, isChange);
					data->lastRectNum = s_rectCnt;
					return isChange;
				}

				//如果运动框体低于一定阈值，则认为走下讲台
				if (data->center.y > params->threshold.outside)
				{
					//如果上一次结果为多目标则认为是讲台上还剩一个人
					if (data->tch_lastStatus == RETURN_TRACK_TCH_MULITY)
					{
						res->status = RETURN_TRACK_TCH_MOVEOUTVIEW;
						data->g_posIndex = -1;
						data->g_prevPosIndex = -1;
						isChange = (res->status != data->tch_lastStatus);
						data->tch_lastStatus = RETURN_TRACK_TCH_MOVEOUTVIEW;
						isChange = tch_return_maintain(&data->tch_timer, isChange);
						data->lastRectNum = s_rectCnt;
						return isChange;
					}
					else
					{
						//if (data->tch_queue->size < TRACK_FRAMES_ANALYSIS - 1)
						//{
						//	EnQueue(data->tch_queue, direct);
						//	res->status = data->tch_lastStatus;
						//	isChange = (res->status != data->tch_lastStatus);
						//	isChange = tch_return_maintain(&data->tch_timer, isChange);
						//	return isChange;
						//}
						//else
						//{
						//	EnQueue(data->tch_queue, direct);
						//	if (tch_direct_analysis(data))
						//	{
						//		res->status = RETURN_TRACK_TCH_OUTSIDE;
						//		isChange = (res->status != data->tch_lastStatus);
						//		data->tch_lastStatus = RETURN_TRACK_TCH_OUTSIDE;
						//		res->pos = -1;
						//		data->g_posIndex = -1;
						//		data->g_prevPosIndex = -1;
						//		data->tch_lastStatus = RETURN_TRACK_TCH_OUTSIDE;
						//		isChange = tch_return_maintain(&data->tch_timer, isChange);
						//		return isChange;
						//	}
						//	//else
						//	//{
						//	//	res->status = data->tch_lastStatus;
						//	//	/*isChange = (res->status != data->tch_lastStatus);
						//	//	isChange = tch_return_maintain(&data->tch_timer, isChange);
						//	//	return isChange;*/
						//	//}
						//}
						if (data->outsideTimer.deltaTime>0)
						{
							//data->slideTimer.deltaTime = 0;
							data->slideTimer.start = gettime();
							//data->slideTimer.finish = 0;
						}
						else
						{
							res->status = RETURN_TRACK_TCH_OUTSIDE;
							isChange = (res->status != data->tch_lastStatus);
							data->tch_lastStatus = RETURN_TRACK_TCH_OUTSIDE;

							if (data->outsideTimer.deltaTime == 0)
							{
								data->outsideTimer.start = gettime();
							}

							data->slideTimer.deltaTime = 0;
							data->slideTimer.start = 0;
							data->pos_slide.center = -1;
							data->pos_slide.left = -1;
							data->pos_slide.right = -1;
							res->pos = -1;
							data->g_posIndex = -1;
							data->g_prevPosIndex = -1;
							isChange = tch_return_maintain(&data->tch_timer, isChange);
							data->lastRectNum = s_rectCnt;
							return isChange;
						}
						
					}
				}

				/*if (data->tch_queue->size > 0)
				{
				ClearQueue(data->tch_queue);
				}*/

				tch_updatePosition(data);

				if ((data->slideTimer.deltaTime) > params->threshold.stand)
				{
					if (data->outsideTimer.deltaTime>0)
					{
						data->outsideTimer.deltaTime = 0;
						data->outsideTimer.start = 0;
						data->outsideTimer.finish = 0;
					}
					res->status = RETURN_TRACK_TCH_MOVEINVIEW;
					isChange = (res->status != data->tch_lastStatus);
					data->tch_lastStatus = RETURN_TRACK_TCH_MOVEINVIEW;
					res->pos = data->pos_slide.center + PRESET_ALIGN;
					isChange = tch_return_maintain(&data->tch_timer, isChange);
					data->lastRectNum = s_rectCnt;
					return isChange;
				}
				else
				{
					res->status = RETURN_TRACK_TCH_MOVEOUTVIEW;
					isChange = (res->status != data->tch_lastStatus);
					data->tch_lastStatus = RETURN_TRACK_TCH_MOVEOUTVIEW;
					res->pos = data->pos_slide.center + PRESET_ALIGN;
					isChange = tch_return_maintain(&data->tch_timer, isChange);
					data->lastRectNum = s_rectCnt;
					return isChange;
				}
			}
			else
			{
				isChange = (res->status != data->tch_lastStatus);
				data->tch_lastStatus = res->status;
				isChange = tch_return_maintain(&data->tch_timer, isChange);
				data->lastRectNum = s_rectCnt;
				return isChange;
			}
		}
	}
	else
	{
		if (!data->currMatTch->data.ptr||!data->currMatBlk->data.ptr)
		{
			return -2;
		}
		if (data->g_count==0)
		{
			data->tch_timer.start = gettime();
		}
		data->g_count++;
		isChange = (res->status != data->tch_lastStatus);
		data->tch_lastStatus = res->status;
		isChange = tch_return_maintain(&data->tch_timer, isChange);
		return isChange;
	}
	isChange = (res->status != data->tch_lastStatus);
	data->tch_lastStatus = res->status;
	isChange = tch_return_maintain(&data->tch_timer, isChange);
	return isChange;
}

int tch_calculateDirect_TCH(Itc_Mat_t* src, Track_Rect_t roi)
{
	int i = 0;
	int j = 0;
	int direct = 0;;
	int sum_gradientV = 0;		//垂直方向梯度
	int sum_gradientH = 0;		//水平方向

	int x1 = roi.x;
	int y1 = roi.y;
	int x2 = roi.x + roi.width;
	int y2 = roi.y + roi.height;

	int step = src->step;
	char *img0 = (char*)(src->data.ptr + step*y1);
	char *img = (char*)(src->data.ptr + step*(y1 + 1));

	for (i = y1; i < y2 - 1; i++)
	{
		for (j = x1; j < x2 - 1; j++)
		{
			if (img0[j] != 0)
			{
				if (img[j] != 0)
					sum_gradientV += img0[j] - img[j];
				if (img0[j + 1] != 0)
					sum_gradientH += img0[j] - img0[j + 1];
			}
		}
		img0 = img;
		img += step;
	}

	int threshold = roi.width*roi.height / 10;
	if (abs(sum_gradientV) > abs(sum_gradientH))
	{
		if (sum_gradientV > threshold)
		{
			direct = 2;
		}
		else if (sum_gradientV < -threshold)
		{
			direct = 1;
		}
	}
	else
	{
		if (sum_gradientH > threshold)
		{
			direct = 3;
		}
		else if (sum_gradientH < -threshold)
		{
			direct = 4;
		}
	}
	return direct;
	//printf("位置：%d,%d,大小：%d,%d 垂直梯度：%d,水平梯度：%d\n", x1, y1, roi.width, roi.height,sum_gradientV, sum_gradientH);
}

void tch_trackDestroy(Tch_Data_t *data)
{
	if (data->g_count > 0)
	{
		itcReleaseMemStorage(&data->storageTch);
		itcReleaseMemStorage(&data->storageBlk);
		itcReleaseMemStorage(&data->storage);

		itc_release_mat(&data->currMatTch);
		itc_release_mat(&data->prevMatTch);
		itc_release_mat(&data->mhiMatTch);
		itc_release_mat(&data->maskMatTch);

		itc_release_mat(&data->currMatBlk);
		itc_release_mat(&data->prevMatBlk);
		itc_release_mat(&data->mhiMatBlk);
		itc_release_mat(&data->maskMatBlk);

		//DestroyQueue(data->tch_queue);
	}
	else
	{
		return;
	}
}

int tch_Init(TeaITRACK_Params *params, Tch_Data_t *data)
{
	if (params==NULL||data==NULL)
	{
		data->sysData.callbackmsg_func("params err.");
		return -1;
	}

	tch_trackDestroy(data);

	if (params->isSetParams==0)
	{
		params->frame.width = TRACK_DEFAULT_WIDTH;
		params->frame.height = TRACK_DEFAULT_HEIGHT;

		params->tch.x = TRACK_DEFAULT_TCH_X;
		params->tch.y = TRACK_DEFAULT_TCH_Y;
		params->tch.width = TRACK_DEFAULT_TCH_W;
		params->tch.height = TRACK_DEFAULT_TCH_H;

		params->blk.x = TRACK_DEFAULT_BLK_X;
		params->blk.y = TRACK_DEFAULT_BLK_Y;
		params->blk.width = TRACK_DEFAULT_BLK_W;
		params->blk.height = TRACK_DEFAULT_BLK_H;

		params->threshold.stand = TRACK_STAND_THRESHOLD;
		params->threshold.outside = TRACK_TCHOUTSIDE_THRESHOLD;
		params->threshold.targetArea = TRACK_TARGETAREA_THRESHOLD;

		params->numOfPos = TRACK_NUMOF_POSITION;
		params->numOfSlide = TRACK_SLIDE_WIDTH;

		//return 0;
	}
	//初始化帧的大小
	if (params->frame.width != 480 || params->frame.height != 264)
	{
		data->sysData.callbackmsg_func("frame size err.");
		return -1;
	}
	data->g_frameSize.width = params->frame.width;
	data->g_frameSize.height = params->frame.height;
	//初始化教师框
	if (params->tch.width<0 || params->tch.height<0)
	{
		data->sysData.callbackmsg_func("teacher window size err.");
		return -1;
	}
	else if (params->tch.width>params->frame.width || params->tch.height>params->frame.height)
	{
		data->sysData.callbackmsg_func("teacher window size err.");
		return -1;
	}
	else if (params->tch.x<0 || params->tch.x>params->frame.width || params->tch.y<0 || params->tch.y>params->frame.height)
	{
		data->sysData.callbackmsg_func("teacher window size err.");
		return -1;
	}
	else
	{
		data->g_tchWin.x = params->tch.x;
		data->g_tchWin.y = params->tch.y;
		data->g_tchWin.width = params->tch.width;
		data->g_tchWin.height = params->tch.height;
	}
	//初始化板书框体
	if (params->blk.width < 0 || params->blk.height < 0)
	{
		data->sysData.callbackmsg_func("blackboard window size err.");
		return -1;
	}
	else if (params->blk.width > params->frame.width || params->blk.height > params->frame.height)
	{
		data->sysData.callbackmsg_func("blackboard window size err.");
		return -1;
	}
	else if (params->blk.x < 0 || params->blk.x > params->frame.width || params->blk.y < 0 || params->blk.y > params->frame.height)
	{
		data->sysData.callbackmsg_func("blackboard window size err.");
		return -1;
	}
	else
	{
		data->g_blkWin.x = params->blk.x;
		data->g_blkWin.y = params->blk.y;
		data->g_blkWin.width = params->blk.width;
		data->g_blkWin.height = params->blk.height;
	}
	//初始化阈值
	if (params->threshold.stand <= 0 || params->threshold.targetArea <= 0 || params->threshold.outside <= 0)
	{
		data->sysData.callbackmsg_func("threshold err.");
		return -1;
	}
	if (params->numOfPos<params->numOfSlide||params->numOfPos<=0||params->numOfSlide<=0)
	{
		data->sysData.callbackmsg_func("camera position err.");
		return -1;
	}
	else
	{
		data->numOfPos = params->numOfPos;
		data->numOfSlide = params->numOfSlide;
	}
	/*else
	{
	track_standThreshold = params->threshold.stand;
	track_targetAreaThreshold = params->threshold.targetArea;
	track_tchOutsideThreshold = params->threshold.outside;
	}*/
	if (tch_trackInit(data)<0)
	{
		return -1;
	}

	//初始化绘制颜色
	data->pink_colour = colour_RGB2YUV(255, 0, 255);	/*粉红*/
	data->blue_colour = colour_RGB2YUV(0, 0, 255);		/*纯蓝*/
	data->lilac_colour = colour_RGB2YUV(155, 155, 255);/*淡紫*/
	data->green_colour = colour_RGB2YUV(0, 255, 0);	/*纯绿*/
	data->red_colour = colour_RGB2YUV(255, 0, 0);		/*纯红*/
	data->dullred_colour = colour_RGB2YUV(127, 0, 0);	/*暗红*/
	data->yellow_colour = colour_RGB2YUV(255, 255, 0);	/*纯黄*/
	return 0;
}
