#ifndef itcTrack_draw_img_h__
#define itcTrack_draw_img_h__
#include "itctype.h"

#ifdef  __cplusplus
extern "C" {
#endif

Track_Colour_t colour(itc_uchar y, itc_uchar u, itc_uchar v);
Track_Colour_t colour_RGB2YUV(int R, int G, int B);

#define TRACK_DRAW_YUV420P	1
#define TRACK_DRAW_YUV420SP 2

void track_draw_point(itc_uchar *buffery, itc_uchar *bufferuv, Track_Size_t *img_size, Track_Point_t* start_point, Track_Colour_t *yuv_value, int type);
void track_draw_line(itc_uchar *buffery, itc_uchar *bufferuv, Track_Size_t* img_size, Track_Point_t* start_point, Track_Point_t* end_point, Track_Colour_t *yuv_value, int type);
void track_draw_rectangle(itc_uchar *buffery, itc_uchar *bufferuv, Track_Size_t* img_size, Track_Rect_t* rect, Track_Colour_t *yuv_value, int type);
#ifdef  __cplusplus  
}
#endif  /* end of __cplusplus */ 

#endif
