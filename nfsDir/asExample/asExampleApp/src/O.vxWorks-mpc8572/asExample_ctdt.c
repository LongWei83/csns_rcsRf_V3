/* C++ static constructor and destructor lists */
/* This is generated by munch.pl, do not edit! */

#include <vxWorks.h>

/* Declarations */
extern void _GLOBAL__I__ZN14repeaterClientC2ERK11osiSockAddr(void);
extern void _GLOBAL__I__ZN18epicsThreadRunableD2Ev(void);
extern void _GLOBAL__I__ZN9dbService13contextCreateER10epicsMutexS1_R16cacContextNotify(void);
extern void _GLOBAL__I_asExample_registerRecordDeviceDriver(void);
extern void _GLOBAL__I_epicsNAN(void);
extern void _GLOBAL__I_iocshPpdbbase(void);
extern void _GLOBAL__I_localHostNameCache(void);
extern void _GLOBAL__I_noopIIU(void);
extern void _GLOBAL__I_osdNTPGet(void);
extern void _GLOBAL__I_timerQueueMgrEPICS(void);
extern void _GLOBAL__D__ZN9dbService13contextCreateER10epicsMutexS1_R16cacContextNotify(void);
extern void _GLOBAL__D_localHostNameCache(void);
extern void _GLOBAL__D_noopIIU(void);
extern void _GLOBAL__D_timerQueueMgrEPICS(void);

char __dso_handle = 0;

/* Exception handler frame */
extern const unsigned __EH_FRAME_BEGIN__[];

static void eh_ctor(void) {
    extern void __register_frame_info (const void *, void *);
    static struct {
        void *a, *b, *c, *d;
        unsigned long e;
        void *f, *g;
    } object;

    __register_frame_info(__EH_FRAME_BEGIN__, &object);
}

static void eh_dtor(void) {
    extern void *__deregister_frame_info (const void *);

    __deregister_frame_info(__EH_FRAME_BEGIN__);
}

/* List of Constructors */
void (*_ctors[])(void) = {
    eh_ctor,
    _GLOBAL__I__ZN14repeaterClientC2ERK11osiSockAddr,
    _GLOBAL__I__ZN18epicsThreadRunableD2Ev,
    _GLOBAL__I__ZN9dbService13contextCreateER10epicsMutexS1_R16cacContextNotify,
    _GLOBAL__I_asExample_registerRecordDeviceDriver,
    _GLOBAL__I_epicsNAN,
    _GLOBAL__I_iocshPpdbbase,
    _GLOBAL__I_localHostNameCache,
    _GLOBAL__I_noopIIU,
    _GLOBAL__I_osdNTPGet,
    _GLOBAL__I_timerQueueMgrEPICS,
    NULL
};

/* List of Destructors */
void (*_dtors[])(void) = {
    eh_dtor,
    _GLOBAL__D__ZN9dbService13contextCreateER10epicsMutexS1_R16cacContextNotify,
    _GLOBAL__D_localHostNameCache,
    _GLOBAL__D_noopIIU,
    _GLOBAL__D_timerQueueMgrEPICS,
    NULL
};
