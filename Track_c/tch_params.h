#ifndef _TCH_PARAMS_H_
#define _TCH_PARAMS_H_

typedef struct Tch_Size_t
{
	int width;
	int height;
}Tch_Size_t;

typedef struct Tch_Rect_t
{
	int x;
	int y;
	int width;
	int height;
}Tch_Rect_t;

//返回结构体
typedef struct Result
{
	int status;
	int pos; //云台摄像机拍摄的位置
}Tch_Result_t;

//阈值结构体
typedef struct Threshold
{
	int stand;
	int targetArea;
	int outside;
}Tch_Threshold_t;

typedef struct 	_TeaITRACK_Params
{
	int isSetParams;
	int numOfPos;
	int numOfSlide;
	Tch_Size_t frame;
	Tch_Rect_t tch;
	Tch_Rect_t blk;
	Tch_Threshold_t threshold;

}TeaITRACK_Params;

#endif
