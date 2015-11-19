#ifndef itcTrack_draw_img_h__
#define itcTrack_draw_img_h__
#include "itctype.h"

#ifdef  __cplusplus
extern "C" {
#endif
typedef struct _colour
{
	uchar val[3];
}Trcak_Colour_t;

Trcak_Colour_t colour(uchar y, uchar u, uchar v);
Trcak_Colour_t colour_RGB2YUV(int R, int G, int B);

#define TRACK_DRAW_YUV420P	1
#define TRACK_DRAW_YUV420SP 2

void track_draw_point(uchar *buffery, uchar *bufferuv, Track_Size_t *img_size, Track_Point_t* start_point, Trcak_Colour_t *yuv_value,int type);
void track_draw_line(uchar *buffery, uchar *bufferuv, Track_Size_t* img_size, Track_Point_t* start_point, Track_Point_t* end_point, Trcak_Colour_t *yuv_value, int type);
void track_draw_rectangle(uchar *buffery, uchar *bufferuv, Track_Size_t* img_size, Track_Rect_t* rect, Trcak_Colour_t *yuv_value, int type);
#ifdef  __cplusplus  
}
#endif  /* end of __cplusplus */ 

#endif