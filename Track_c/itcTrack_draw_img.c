#include "itcTrack_draw_img.h"
#include <math.h>
#include <stdlib.h>

typedef void(*draw_point_func)(itc_uchar *buffer_y, itc_uchar *buffer_u, itc_uchar *buffer_v, Track_Size_t* img_size, int point_x, int point_y, Track_Colour_t *yuv_value);

static void _track_draw_point1(itc_uchar *buffer_y, itc_uchar *buffer_u, itc_uchar *buffer_v, Track_Size_t* img_size, int point_x, int point_y, Track_Colour_t *yuv_value)
{
	//TRACK_DRAW_YUV420P
	if (buffer_y == NULL || img_size == NULL || yuv_value == NULL)
	{
		return;
	}
	//yuv分段存储
	int point_x_off = ((point_x & 1) == 0) ? 1 : -1;
	int point_y_off = ((point_y & 1) == 0) ? img_size->width : -img_size->width;
	buffer_y += (point_y*img_size->width + point_x);
	//因为一个u和v对应4个y，对y操作
	*(itc_uchar *)(buffer_y) = yuv_value->val[0];
	*(itc_uchar *)(buffer_y + point_x_off) = yuv_value->val[0];
	*(itc_uchar *)(buffer_y + point_y_off) = yuv_value->val[0];
	*(itc_uchar *)(buffer_y + point_y_off + point_x_off) = yuv_value->val[0];

	//对u和v操作
	if (buffer_u != NULL && buffer_v != NULL)
	{
		int u2v_offset = ((point_y >> 1)*(img_size->width >> 1) + (point_x >> 1));
		*(itc_uchar *)(buffer_u + u2v_offset) = yuv_value->val[1];
		*(itc_uchar *)(buffer_v + u2v_offset) = yuv_value->val[2];
	}
}

static void _track_draw_point2(itc_uchar *buffer_y, itc_uchar *buffer_u, itc_uchar *buffer_v, Track_Size_t* img_size, int point_x, int point_y, Track_Colour_t *yuv_value)
{
	//TRACK_DRAW_YUV420SP
	if (buffer_y == NULL || img_size == NULL || yuv_value == NULL)
	{
		return;
	}
	//yuv分段存储
	int point_x_off = ((point_x & 1) == 0) ? 1 : -1;
	int point_y_off = ((point_y & 1) == 0) ? img_size->width : -img_size->width;
	buffer_y += (point_y*img_size->width + point_x);
	//因为一个u和v对应4个y，对y操作
	*(itc_uchar *)(buffer_y) = yuv_value->val[0];
	*(itc_uchar *)(buffer_y + point_x_off) = yuv_value->val[0];
	*(itc_uchar *)(buffer_y + point_y_off) = yuv_value->val[0];
	*(itc_uchar *)(buffer_y + point_y_off + point_x_off) = yuv_value->val[0];

	//对u和v操作
	if (buffer_u != NULL && buffer_v != NULL)
	{
		int u2v_offset = ((point_y >> 1)*(img_size->width) + (point_x & (~1)));
		*(itc_uchar *)(buffer_u + u2v_offset) = yuv_value->val[1];//buffer_v-buffer_u=1，u、v交叉存储
		*(itc_uchar *)(buffer_v + u2v_offset) = yuv_value->val[2];
	}
}

Track_Colour_t colour(itc_uchar y, itc_uchar u, itc_uchar v)
{
	Track_Colour_t yuv;
	yuv.val[0] = y;
	yuv.val[1] = u;
	yuv.val[2] = v;
	return yuv;
}

Track_Colour_t colour_RGB2YUV(int R, int G, int B)
{
	itc_uchar Y = (itc_uchar)( 0.2990 * R + 0.5870 * G + 0.1140 * B);
	itc_uchar U = (itc_uchar)(-0.1687 * R - 0.3313 * G + 0.5000 * B + 128);
	itc_uchar V = (itc_uchar)( 0.5000 * R - 0.4187 * G - 0.0813 * B + 128);
	return colour(Y, U, V);
}


void track_draw_point(itc_uchar *buffer, itc_uchar *bufferuv, Track_Size_t *img_size, Track_Point_t* start_point, Track_Colour_t *yuv_value, int type)
{
	if (buffer == NULL || img_size == NULL
		|| start_point == NULL || yuv_value == NULL)
	{
		return;
	}

	draw_point_func _track_draw_point=NULL;
	itc_uchar *buffer_u = NULL;
	itc_uchar *buffer_v = NULL;
	int sizeuv = img_size->width*img_size->height;
	int uvoffset = 0;
	switch (type)
	{
	case TRACK_DRAW_YUV420P:
		uvoffset = (sizeuv >> 2);
		_track_draw_point = _track_draw_point1;
		break;
	case TRACK_DRAW_YUV420SP:
		uvoffset = 1;
		_track_draw_point = _track_draw_point2;
		break;
	default:
		uvoffset = (sizeuv >> 2);
		_track_draw_point = _track_draw_point1;
		break;
	}
	if (bufferuv != NULL)
	{
		buffer_u = bufferuv;
		buffer_v = buffer_u + uvoffset;
	}

	_track_draw_point(buffer, buffer_u, buffer_v, img_size, start_point->x, start_point->y, yuv_value);
}


void track_draw_line(itc_uchar *buffer, itc_uchar *bufferuv, Track_Size_t* img_size, Track_Point_t* start_point, Track_Point_t* end_point, Track_Colour_t *yuv_value, int type)
{
	if (buffer == NULL || img_size == NULL
		|| start_point == NULL || end_point == NULL || yuv_value == NULL)
	{
		return;
	}

	draw_point_func _track_draw_point = NULL;
	itc_uchar *buffer_u = NULL;
	itc_uchar *buffer_v = NULL;
	int sizeuv = img_size->width*img_size->height;
	int uvoffset = 0;
	switch (type)
	{
	case TRACK_DRAW_YUV420P:
		uvoffset = (sizeuv >> 2);
		_track_draw_point = _track_draw_point1;
		break;
	case TRACK_DRAW_YUV420SP:
		uvoffset = 1;
		_track_draw_point = _track_draw_point2;
		break;
	default:
		uvoffset = (sizeuv >> 2);
		_track_draw_point = _track_draw_point1;
		break;
	}
	if (bufferuv != NULL)
	{
		buffer_u = bufferuv;
		buffer_v = buffer_u + uvoffset;
	}

	int dx = end_point->x - start_point->x;
	int	dy = end_point->y - start_point->y;
	int	steps = 0;
	int	k = 0;
	int tx, ty;
	float xIn, yIn;

	float x = (float)start_point->x;
	float y = (float)start_point->y;

	if (abs(dx) > abs(dy))
	{
		steps = abs(dx);
	}
	else
	{
		steps = abs(dy);
	}

	xIn = (float)dx / (float)steps;
	yIn = (float)dy / (float)steps;
	_track_draw_point(buffer, buffer_u, buffer_v, img_size, start_point->x, start_point->y, yuv_value);
	for (k = 0; k < steps; k++)
	{
		x += xIn;
		y += yIn;
		tx = (int)(x + 0.5);
		ty = (int)(y + 0.5);
		_track_draw_point(buffer, buffer_u, buffer_v, img_size, tx, ty, yuv_value);
	}
}

void track_draw_rectangle(itc_uchar *buffer, itc_uchar *bufferuv, Track_Size_t* img_size, Track_Rect_t* rect, Track_Colour_t *yuv_value, int type)
{
	if (buffer == NULL || img_size == NULL
		|| rect == NULL || yuv_value == NULL)
	{
		return;
	}

	draw_point_func _track_draw_point = NULL;
	itc_uchar *buffer_u = NULL;
	itc_uchar *buffer_v = NULL;
	int sizeuv = img_size->width*img_size->height;
	int uvoffset = 0;
	switch (type)
	{
	case TRACK_DRAW_YUV420P:
		uvoffset = (sizeuv >> 2);
		_track_draw_point = _track_draw_point1;
		break;
	case TRACK_DRAW_YUV420SP:
		uvoffset = 1;
		_track_draw_point = _track_draw_point2;
		break;
	default:
		uvoffset = (sizeuv >> 2);
		_track_draw_point = _track_draw_point1;
		break;
	}
	if (bufferuv!=NULL)
	{
		buffer_u = bufferuv;
		buffer_v = buffer_u + uvoffset;
	}
	
	int left_x = rect->x;
	int up_y = rect->y;
	int right_x = rect->x + rect->width - 1;
	int down_y = rect->y + rect->height - 1;
	int i = 0;
	int j = 0;
	for (j = left_x; j <= right_x; j+=2)
	{
		_track_draw_point(buffer, buffer_u, buffer_v, img_size, j, up_y, yuv_value);
		_track_draw_point(buffer, buffer_u, buffer_v, img_size, j, down_y, yuv_value);
	}
	for (i = up_y; i <= down_y; i+=2)
	{
		_track_draw_point(buffer, buffer_u, buffer_v, img_size, left_x, i, yuv_value);
		_track_draw_point(buffer, buffer_u, buffer_v, img_size, right_x, i, yuv_value);
	}
}
