#include "itcTrack_draw_img.h"
#include <math.h>
#include <stdlib.h>

typedef void(*draw_point_func)(uchar *buffer_y, uchar *buffer_u, uchar *buffer_v, Track_Size_t* img_size, int point_x, int point_y, Trcak_Colour_t *yuv_value);

static void _track_draw_point1(uchar *buffer_y, uchar *buffer_u, uchar *buffer_v, Track_Size_t* img_size, int point_x, int point_y, Trcak_Colour_t *yuv_value)
{
	//TRACK_DRAW_YUV420P
	if (buffer_y == NULL || buffer_u == NULL || buffer_v == NULL
		|| img_size == NULL || yuv_value == NULL)
	{
		return;
	}
	//yuv分段存储
	int point_x_off = ((point_x & 1) == 0) ? 1 : -1;
	int point_y_off = ((point_y & 1) == 0) ? img_size->width : -img_size->width;
	buffer_y += (point_y*img_size->width + point_x);
	//因为一个u和v对应4个y，对y操作
	*(uchar *)(buffer_y) = yuv_value->val[0];
	*(uchar *)(buffer_y + point_x_off) = yuv_value->val[0];
	*(uchar *)(buffer_y + point_y_off) = yuv_value->val[0];
	*(uchar *)(buffer_y + point_y_off + point_x_off) = yuv_value->val[0];

	//对u和v操作
	int u2v_offset = ((point_y >> 1)*(img_size->width >> 1) + (point_x >> 1));
	*(uchar *)(buffer_u + u2v_offset) = yuv_value->val[1];
	*(uchar *)(buffer_v + u2v_offset) = yuv_value->val[2];
}

static void _track_draw_point2(uchar *buffer_y, uchar *buffer_u, uchar *buffer_v, Track_Size_t* img_size, int point_x, int point_y, Trcak_Colour_t *yuv_value)
{
	//TRACK_DRAW_YUV420SP
	if (buffer_y == NULL || buffer_u == NULL || buffer_v == NULL
		|| img_size == NULL || yuv_value == NULL)
	{
		return;
	}
	//yuv分段存储
	int point_x_off = ((point_x & 1) == 0) ? 1 : -1;
	int point_y_off = ((point_y & 1) == 0) ? img_size->width : -img_size->width;
	buffer_y += (point_y*img_size->width + point_x);
	//因为一个u和v对应4个y，对y操作
	*(uchar *)(buffer_y) = yuv_value->val[0];
	*(uchar *)(buffer_y + point_x_off) = yuv_value->val[0];
	*(uchar *)(buffer_y + point_y_off) = yuv_value->val[0];
	*(uchar *)(buffer_y + point_y_off + point_x_off) = yuv_value->val[0];

	//对u和v操作
	int u2v_offset = ((point_y >> 1)*(img_size->width) + (point_x >> 1));
	*(uchar *)(buffer_u + u2v_offset) = yuv_value->val[1];
	*(uchar *)(buffer_v + u2v_offset) = yuv_value->val[2];
}

Trcak_Colour_t colour(uchar y, uchar u, uchar v)
{
	Trcak_Colour_t yuv;
	yuv.val[0] = y;
	yuv.val[1] = u;
	yuv.val[2] = v;
	return yuv;
}

Trcak_Colour_t colour_RGB2YUV(int R, int G, int B)
{
	uchar Y = (uchar)( 0.2990 * R + 0.5870 * G + 0.1140 * B);
	uchar U = (uchar)(-0.1687 * R - 0.3313 * G + 0.5000 * B + 128);
	uchar V = (uchar)( 0.5000 * R - 0.4187 * G - 0.0813 * B + 128);
	return colour(Y, U, V);
}


void track_draw_point(uchar *buffer, uchar *bufferuv, Track_Size_t *img_size, Track_Point_t* start_point, Trcak_Colour_t *yuv_value, int type)
{
	if (buffer == NULL || img_size == NULL
		|| start_point == NULL || yuv_value == NULL)
	{
		return;
	}
	draw_point_func _track_draw_point=NULL;
	switch (type)
	{
	case TRACK_DRAW_YUV420P:
		_track_draw_point = _track_draw_point1;
		break;
	case TRACK_DRAW_YUV420SP:
		_track_draw_point = _track_draw_point2;
		break;
	default:
		_track_draw_point = _track_draw_point1;
		break;
	}

	int sizeuv = img_size->width*img_size->height;
	uchar *buffer_u = bufferuv;
	uchar *buffer_v = buffer_u + (sizeuv >> 2);
	_track_draw_point(buffer, buffer_u, buffer_v, img_size, start_point->x, start_point->y, yuv_value);
}


void track_draw_line(uchar *buffer, uchar *bufferuv, Track_Size_t* img_size, Track_Point_t* start_point, Track_Point_t* end_point, Trcak_Colour_t *yuv_value, int type)
{
	if (buffer == NULL || img_size == NULL
		|| start_point == NULL || end_point == NULL || yuv_value == NULL)
	{
		return;
	}

	draw_point_func _track_draw_point = NULL;
	switch (type)
	{
	case TRACK_DRAW_YUV420P:
		_track_draw_point = _track_draw_point1;
		break;
	case TRACK_DRAW_YUV420SP:
		_track_draw_point = _track_draw_point2;
		break;
	default:
		_track_draw_point = _track_draw_point1;
		break;
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
	int sizeuv = img_size->width*img_size->height;
	uchar *buffer_u = bufferuv;
	uchar *buffer_v = buffer_u + (sizeuv >> 2);
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

void track_draw_rectangle(uchar *buffer, uchar *bufferuv, Track_Size_t* img_size, Track_Rect_t* rect, Trcak_Colour_t *yuv_value, int type)
{
	if (buffer == NULL || img_size == NULL
		|| rect == NULL || yuv_value == NULL)
	{
		return;
	}

	draw_point_func _track_draw_point = NULL;
	switch (type)
	{
	case TRACK_DRAW_YUV420P:
		_track_draw_point = _track_draw_point1;
		break;
	case TRACK_DRAW_YUV420SP:
		_track_draw_point = _track_draw_point2;
		break;
	default:
		_track_draw_point = _track_draw_point1;
		break;
	}

	int sizeuv = img_size->width*img_size->height;
	uchar *buffer_u = bufferuv;
	uchar *buffer_v = buffer_u + (sizeuv >> 2);
	int left_x = rect->x;
	int up_y = rect->y;
	int right_x = rect->x + rect->width - 1;
	int down_y = rect->y + rect->height - 1;
	int i = 0;
	int j = 0;
	for (j = left_x; j <= right_x; j++)
	{
		_track_draw_point(buffer, buffer_u, buffer_v, img_size, j, up_y, yuv_value);
		_track_draw_point(buffer, buffer_u, buffer_v, img_size, j, down_y, yuv_value);
	}
	for (i = up_y; i <= down_y; i++)
	{
		_track_draw_point(buffer, buffer_u, buffer_v, img_size, left_x, i, yuv_value);
		_track_draw_point(buffer, buffer_u, buffer_v, img_size, right_x, i, yuv_value);
	}
}
