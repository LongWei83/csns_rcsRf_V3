/*************************************************************************\
* Copyright (c) 2008 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

/* Revision-Id: ralph.lange@gmx.de-20140801152055-0gm1bl7f1i1a2546
 *
 *      Original Authors: Bob Dalesio and Marty Kraimer
 *      Date: 3/6/91
 */

#include <vxWorks.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <taskLib.h>
#include <wdLib.h>
#include <tickLib.h>
#include <sysLib.h>
#include <memLib.h>          /* Memory Statistics */
#include <private/iosLibP.h> /* fd usage          */

#include <alarm.h>
#include <dbDefs.h>
#include <dbScan.h>
#include <dbAccess.h>
#include <callback.h>
#include <recSup.h>
#include <recGbl.h>
#include <devSup.h>
#include <link.h>
#include <aiRecord.h>
#include <aoRecord.h>
#include <epicsExport.h>

/*

record(ai,"$(user):memoryUsedPercent") {
    field(SCAN,"10 second")
    field(DTYP,"VX STATS")
    field(INP,"@memory")
}
record(ai,"$(user):cpuUsedPercent") {
    field(SCAN,"10 second")
    field(DTYP,"VX STATS")
    field(INP,"@cpu")
    field(HIHI,"90.0")
    field(HIGH,"60.0")
    field(HHSV,"MAJOR")
    field(HSV,"MINOR")
}

*/

/* Create the dset for devAiSoft */
static long init(int pass);
static long init_record(aiRecord *prec);
static long read_ai(aiRecord *prec);

static double cpuBurn();
static void cpuUsageTask();
static double getCpu(void);
static void cpuUsageInit(void);
static double getMemory(void);

#define TOTAL_AI_TYPES  5

#define MEMORY_TYPE	0
#define CPU_TYPE	1
#define MEMORY_USED     2
#define MEMORY_FREE     3
#define MEMORY_TOTAL    4

#define SECONDS_TO_STARVATION 60
#define SECONDS_TO_BURN 5
#define SECONDS_TO_WAIT 15

typedef struct devPvt
{
    int type;
}devPvt;

static MEM_PART_STATS meminfo;

static int default_scan_rate[] = { 10,50,10,10,10};

static char *parmValue[TOTAL_AI_TYPES] = {"memory","cpu","memoryUsed", "memoryFree", "memoryTotal"};

typedef struct scanInfo
{
	WDOG_ID wd;
	int 	rate_tick;	/* ticks */
}scanInfo;
static scanInfo scan[TOTAL_AI_TYPES];

typedef struct cpuUsage {
    unsigned long ticksToDeclareStarvation;
    unsigned long ticksToBurn;
    unsigned long ticksToWait;
    double        nBurnNoContention;
    unsigned long ticksLastUpdate;
    double        usage;
} cpuUsage;
static cpuUsage usage;


struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_ai;
    DEVSUPFUN special_linconv;
} devVXStatsSIM = {
    6,
    NULL,
    init,
    init_record,
    NULL,
    read_ai,
    NULL
};

epicsExportAddress(dset, devVXStatsSIM);

static long init(int pass)
{
	int type;
	/*
	if(pass==1)
	{
		for(type=0;type<TOTAL_AI_TYPES;type++)
		{
			if(type==CPU_TYPE)
			{
				wdStart(scan[type].wd, scan[type].rate_tick, (FUNCPTR)wdCpu,0);
			}
			else
			{
				wdStart(scan[type].wd, scan[type].rate_tick, （FUNCPTR)wdCallback,type);
			}
		}
		return(0);
	}
	*/
	for(type=0;type<TOTAL_AI_TYPES;type++)
	{
		scan[type].wd=wdCreate();
		scan[type].rate_tick=default_scan_rate[type]*sysClkRateGet();
	}
	
	cpuUsageInit();
	
    return 0;
}

static long init_record(aiRecord* pr)
{
	int type;
	devPvt* pvt = NULL;
	char *parm;
	
	if(pr->inp.type!=INST_IO) {
	recGblRecordError(S_db_badField,(void*)pr,
		"devAiStats (init_record) Illegal INP field");
	return S_db_badField;
    }
	
	parm = pr->inp.value.instio.string;
	for(type=0; type < TOTAL_AI_TYPES; type++) {
		if(strcmp(parm,parmValue[type])==0) {
			pvt=(devPvt*)malloc(sizeof(devPvt));
			pvt->type=type;

			if(type <= CPU_TYPE)
                pr->hopr = 100.0;
			else
			{
			  memPartInfoGet(memSysPartId,&(meminfo));
			  pr->hopr = (meminfo.numBytesFree + meminfo.numBytesAlloc)/(double)(1024*1024);
			}
		}
    }
	
	if(pvt==NULL) {
	recGblRecordError(S_db_badField,(void*)pr,
		"devAiStats (init_record) Illegal INP parm field");
	return S_db_badField;
    }
	
	/* Make sure record processing routine does not perform any conversion*/
    pr->linr=0;
    pr->dpvt=pvt;
    pr->lopr = 0.0;
    pr->prec = 2;
    return 0;
}

static long read_ai(aiRecord *pr)
{
    double value = 0.0;
    devPvt* pvt=(devPvt*)pr->dpvt;

    if(!pvt) return(0);
    switch(pvt->type) {

        case MEMORY_TYPE: 
          value = getMemory();
          break;

        case CPU_TYPE:
          value = getCpu();
          break;

        case MEMORY_USED:
          memPartInfoGet(memSysPartId,&(meminfo));
          value = meminfo.numBytesAlloc/(double)(1024*1024);
          break;

        case MEMORY_FREE:
          memPartInfoGet(memSysPartId,&(meminfo));
          value = meminfo.numBytesFree/(double)(1024*1024);
          break;

        case MEMORY_TOTAL:
          memPartInfoGet(memSysPartId,&(meminfo));
          value = (meminfo.numBytesFree + meminfo.numBytesAlloc)/(double)(1024*1024);
          break;

        default: 
          recGblRecordError(S_db_badField,(void*)pr,"Illegal type");
          break;
    }
    pr->val=value;
    pr->udf=0;
    return 2; /* don't convert */
}

static void cpuUsageInit(void)
{
	unsigned long tickStart,tickEnd=0,tickNow;
        int           nBurnNoContention=0;
	usage.usage = 0.0;
	usage.ticksToDeclareStarvation = SECONDS_TO_STARVATION * sysClkRateGet();
	usage.ticksToWait = SECONDS_TO_WAIT*sysClkRateGet();
    usage.ticksToBurn = SECONDS_TO_BURN*sysClkRateGet();
	tickStart = tickGet();
	
	while(tickStart==(tickNow = tickGet())) {;}
	tickStart = tickNow;
	
	while(TRUE) {
	cpuBurn();
	nBurnNoContention++;
        tickEnd = tickGet();
        if(tickEnd<tickStart) break; /*same test as cpuUsageTask*/
        if((tickEnd - tickStart) >= usage.ticksToBurn) break;
    }
	usage.nBurnNoContention = (double)nBurnNoContention;
    usage.ticksLastUpdate = tickGet();
	taskSpawn("cpuUsageTask",255,VX_FP_TASK,3000,(FUNCPTR)cpuUsageTask,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	
}

static double cpuBurn()
{
	int i;
	double result = 0.0;
	
	for(i=0;i<5;i++) result += sqrt((double)i);
		
	return(result);
}

static void cpuUsageTask()
{
	while(TRUE)
	{
		unsigned long tickStart,tickEnd=0;
		int nBurnNow;
		double nBurnNoContention = (double)usage.nBurnNoContention;
		double nBurnContention,newusage;
		nBurnNow=0;
		tickStart = tickGet();
		while(1)
		{
			cpuBurn();
			++nBurnNow;
			tickEnd = tickGet();
			if(tickEnd<tickStart) break;
			if((tickEnd - tickStart) >= usage.ticksToBurn) break;
		}
		if(tickEnd<tickStart) continue;
		nBurnContention = (double)nBurnNow;
        newusage = 100.0*((nBurnNoContention - nBurnNow)/nBurnNoContention);
		usage.usage = newusage;
        taskDelay(usage.ticksToWait);
        usage.ticksLastUpdate = tickEnd;
	}
}

/*
static void wdCpu(void)
{
    unsigned long ticksLastUpdate = usage.ticksLastUpdate;
    unsigned long ticksNow = tickGet();

    if(ticksNow>=ticksLastUpdate
    && (ticksNow-ticksLastUpdate) >= usage.ticksToDeclareStarvation) {
        usage.usage = 100.0;
    }
    wdStart(scan[CPU_TYPE].wd,scan[CPU_TYPE].rate_tick,(FUNCPTR)wdCpu,0);
}
*/

static double getCpu()
{
	double value;
	value = usage.usage;
	return(value);
}

static double getMemory(void)
{
    double nfree,nalloc;

    memPartInfoGet(memSysPartId,&(meminfo));
    nfree = meminfo.numBytesFree;
    nalloc = meminfo.numBytesAlloc;
    return(100.0 * nalloc/(nalloc + nfree));
}
