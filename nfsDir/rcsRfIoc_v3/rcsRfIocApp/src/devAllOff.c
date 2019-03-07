/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* devBoSoft.c */
/* base/src/dev Revision-Id: anj@aps.anl.gov-20101005192737-disfz3vs0f3fiixd */

/* devBoSoft.c - Device Support Routines for  Soft Binary Output*/
/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Marty Kraimer
 *      Date:            6-1-90
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "boRecord.h"
#include "epicsExport.h"

#include <drvD212.h>

static long init_record(boRecord *prec);

/* Create the dset for devBoSoft */
static long write_bo(boRecord *prec);

struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_bo;
}devAllOff={
	5,
	NULL,
	NULL,
	init_record,
	NULL,
	write_bo
};
epicsExportAddress(dset,devAllOff);

static long init_record(boRecord *prec)
{
 
   long status=0;
 
    /* dont convert */
   status=2;
   return status;
 
} /* end init_record() */

static long write_bo(boRecord *prec)
{
    long status;
	int cardNum;
	
	/*PV的值写入1时执行自动开机的动作*/
	if (prec->val == 1)
	{
		for (cardNum = 0; cardNum<8; cardNum++)
		{
			if ((getCardStruct(cardNum)->processing) == 0)  /*如果该板卡没有正在运行自动开机程序则调用自动开机的函数，否则不进行任何操作*/
			{
				autoOff(cardNum);  /*调用自动关机的函数*/
			}
		}
	}
	
	prec->val = 0;
	
    return(status);
}
