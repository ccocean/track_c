#include "tch_track.h"
#include<opencv/cv.h>
#include<opencv/highgui.h>

#define WIDTH 480
#define HEIGHT 264

CvMat MatToCV(Itc_Mat_t *src)
{
	CvMat m;

	m.type = src->type;
	m.cols = src->cols;
	m.rows = src->rows;
	m.step = src->step;
	m.data.ptr = src->data.ptr;
	m.refcount = src->refcount;
	m.hdr_refcount = src->hdr_refcount;
	return m;
};

int main()
{
	Itc_Mat_t *srcMat = itc_create_mat(HEIGHT, WIDTH, ITC_8UC1);//360,640
	//cvNamedWindow("Src", 1);
	//cvNamedWindow("teacher", 1);
	char* g_videoPath;
	char* src=0;
	//int g_count = 0;
	g_videoPath = "video/teacher2.mp4";
	/*g_frameSize = itcSize(640, 360);
	g_tchWin = itcRect(0, 100, 640, 200);
	g_blkWin = itcRect(0, 35, 640, 50);*/
	Tch_Size_t _frame = { WIDTH, HEIGHT };
	Tch_Rect_t tch = { 0, 0, WIDTH, 150 };//{0,75,480,150}
	Tch_Rect_t blk = { 0, 26, WIDTH, 37 }; //{0, 35, 640, 50}
	Tch_Threshold_t threshold = { 2, 7200, 95 };//{2, 12000, 130}
	TeaITRACK_Params *argument;
	argument = (TeaITRACK_Params *)malloc(sizeof(TeaITRACK_Params));
	argument->frame = _frame;
	argument->tch = tch;
	argument->blk = blk;
	argument->threshold = threshold;
	//argument->isSetParams = 1;
	argument->numOfPos = 10;
	argument->numOfSlide = 5;
	argument->isSetParams = 0;
	Tch_Result_t *res;
	res = (Tch_Result_t*)malloc(sizeof(Tch_Result_t));
	Tch_Data_t *data;
	//data->callbackmsg_func = printf;
	data = (Tch_Data_t*)malloc(sizeof(Tch_Data_t));
	int err = tch_Init(argument, data);
	if (err<0)
	{
		printf("error\n\r");
	}
	

	CvCapture* s_video = 0;
	IplImage *frame = NULL;
	IplImage *imgSrc = cvCreateImage(cvSize(WIDTH, HEIGHT), 8, 3);
	IplImage *grayImg = cvCreateImage(cvSize(WIDTH, HEIGHT), 8, 1);
	IplImage *yuvImg = cvCreateImage(cvSize(WIDTH, HEIGHT*3/2), 8, 1);
	CvMat testMat;
	uchar *uv;
	uv = malloc(sizeof(uchar));

	cvNamedWindow("test", 1);

	s_video = cvCaptureFromFile(g_videoPath);
	int result;

	clock_t start,end;
	double delta;
	while (1)
	{
		frame = cvQueryFrame(s_video);

		if (!frame)
		{
			return -1; break;
		}

		cvResize(frame, imgSrc, 1);
		//cvShowImage("Src", imgSrc);
		imgSrc->origin = frame->origin;
		cvCvtColor(imgSrc, grayImg, CV_RGB2GRAY);
		cvCvtColor(imgSrc, yuvImg, CV_RGB2YUV_I420);
		//memcpy(srcMat->data.ptr, grayImg->imageData, srcMat->step*srcMat->rows);
		//cvShowImage("test", imgSrc);
		testMat = MatToCV(data->mhiMatTch);
		cvShowImage("teacher", &testMat);
		start = clock();
		result = tch_track(yuvImg->imageData,uv,argument,data,res);
		end = clock();
		cvCvtColor(yuvImg, imgSrc, CV_YUV2BGR_I420);
		cvShowImage("test", imgSrc);
		delta = (end - start) ;
		printf("status:%d, position:%d, time:%f\r\n", res->status,res->pos,(double)(delta/CLOCKS_PER_SEC));
		cvWaitKey(1);
		//g_count++;
	}
	tch_trackDestroy(data);

	return 0;
}