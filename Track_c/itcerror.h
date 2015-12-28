/************************************************************************** 
    *  @Copyright (c) 2015, XueYB, All rights reserved. 
 
    *  @file     : itcerror.h 
    *  @version  : ver 1.0 
 
    *  @author   : XueYB 
    *  @date     : 2015/10/09 11:45 
    *  @brief    : 一些错误的处理 
**************************************************************************/

#ifndef itcerror_h__
#define itcerror_h__

#define ITC_ERROR_(errors) printf("error:%s\n\r",errors);
#define ITC_ERROR_DETAIL(errCode,errors) printf("code:%d,err:%s\n\r",errCode,errors);

#define __BEGIN__       {
#define __END__         goto exit; exit: ; }
#define __CLEANUP__
#define EXIT            goto exit

//error code
#define  ITC_OK 0
#define  ITC_StsBadSize    -201 /* the input/output structure size is incorrect  */
#define  ITC_StsOutOfRange -211  /* some of parameters are out of range */
#define  ITC_StsNoMem -4	/* insufficient memory */
#define  ITC_StsBadArg               -5  /* function arg/param is bad       */
#define  ITC_StsNullPtr  -27 /* null pointer */
#define  ITC_BADARG_ERR   -49  //ipp comp

#endif // itcerror_h__

