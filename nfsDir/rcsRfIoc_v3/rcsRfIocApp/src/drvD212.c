
#include <vxWorks.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysLib.h>
#include <intLib.h>
#include <taskLib.h>
#include <string.h>
#include <logLib.h>
#include <ioLib.h>
#include <sockLib.h>
#include <inetLib.h>
#include <netinet/tcp.h>
#include <registryFunction.h>
#include <drv/pci/pciConfigLib.h>
#include <arch/ppc/ivPpc.h>
#include "drvD212.h"
#include "plx9656.h"
#include "llrfCommonIO.h"
#include "parameter.h"
#include "drvSup.h"
#include "epicsExport.h"
#include <semLib.h>
#include <dbScan.h>


/* D212 Register Map */
#define    REG_Identifier			0x000

#define    REG_Int_Enable			0x004

#define    REG_RF_Reset				0x00c

#define    REG_Int_Clear			0x008

#define    REG_Alarm				0x010

#define    REG_Drv_Reset			0x014

#define    REG_SG_Mode				0x018

#define    REG_ARC_COUNT			0x01C

#define    REG_BEAM_INT				0x028

#define    REG_Point_Sweep			0x100
#define    REG_Sweep_Option			0x104
#define    REG_AMP_Option			0x108
#define    REG_AMP_FF_Option			0x10C
#define    REG_AMP_Modify_Option		0x110
#define    REG_Tune_Option			0x114
#define    REG_Front_Tune_Option		0x118
#define    REG_Tune_FF_Option			0x11C
#define    REG_Tune_Modify_Option		0x120
#define	   REG_Phase_Option			0x124
#define	   REG_Phase_FF_Option			0x128
#define	   REG_Phase_Modify_Option		0x12C
#define	   REG_Fre_Change_Option		0x130
#define	   REG_Amp_Change_Option		0x134
#define	   REG_Beam_Feedforward_Option		0x138
#define	   REG_Beam_Phase_Option		0x13C
#define	   REG_Front_Tune_FF_Option		0x140
#define	   REG_Front_Tune_Modify_Option		0x144




#define    REG_Fix_Frequency_Set		0x200
#define    REG_Work_Period_Set			0x204
#define    REG_AMP_Set				0x208
#define    REG_AMP_Coefficient			0x20C
#define    REG_AMP_P_Set			0x210
#define    REG_AMP_I_Set			0x214
#define    REG_AMP_I_Injection			0x218
#define    REG_PHASE_I_Injection		0x21C
#define    REG_Bias_Set				0x224
#define    REG_Fix_Tuning_Angle			0x228
#define    REG_Tuning_Angle_Offset		0x22C
#define    REG_Tune_P_Set			0x230
#define    REG_Tune_I_Set			0x234
#define    REG_Tune_I_Set1			0x238
#define    REG_Tune_I_Set2			0x23C
#define    REG_Tune_I_Set3			0x240
#define    REG_Front_Bias_Set			0x244
#define    REG_Front_Tune_P_Set			0x248
#define    REG_Front_Tune_I_Set			0x24C
#define    REG_Front_Fix_Tuning_Angle		0x250
#define    REG_Phase_P				0x254
#define    REG_Phase_I				0x258
#define    REG_Initial_Phase			0x25C
#define    REG_FF_Delay				0x260
#define    REG_PreTrig_Delay			0x264
#define    REG_Initial_Ref_Phase		0x268
#define    REG_Int_Delay			0x26C
#define    REG_Chopper_Duty			0x270
#define    REG_Rf_Harmonic			0x274
#define    REG_EX_Phase				0x278
#define    REG_RBF_Delay			0x27C
#define    REG_BPM_Delay_Set			0x280
#define    REG_Chopper_Phase_Set		0x284
#define    REG_EX_Delay_set			0x288
#define    REG_REF_DELAY_AB			0x28C

#define    REG_REF_DELAY_C			0x290

#define    REG_REF_DELAY_D			0x294
#define    REG_BEAM_FF_COEF			0x298
#define    REG_BEAM_FF_PHASE_SET		0x29c
#define    REG_BEAM_FF_DELAY			0x2a0
#define    REG_SYN_OSC_DELAY			0x2a4
#define    REG_SYN_OSC_COUNT			0x2a8



#define    REG_AMP_Upload			0x300
#define    REG_AMP_Set_Upload			0x304
#define    REG_Tuning_Phase_Upload		0x308
#define    REG_Front_Tuning_Phase_Upload	0x30C
#define    REG_Bias_Upload			0x310
#define    REG_Front_Bias_Upload		0x314
#define    REG_Reserved_1			0x318
#define    REG_Reserved_2			0x31C    	

static long D212Report(int level);
static D212Card *firstCard = NULL;
static int dmaCount = 0;
static int intHasConnect[MAX_INT_SUP] = {0, 0, 0, 0};
static int dmaUse = 0;
int *pGBuff = NULL;
static unsigned int preTrigAll = 0;

/* This is defined for tcp ip parm */
static int listenSkt;
static int commuSkt;
static int flagNetInit;
static SEM_ID semCmdLink;
static SEM_ID semSend;
static int mode;

/* This is defined for tcp ip server program*/
STATUS netInit(int mode);
STATUS netCheckLink();
STATUS netSend();
void netCloseAll(int mode);

/* --End-- */

struct {
        long    number;
        DRVSUPFUN       report;
        DRVSUPFUN       init;
}drvD212 = {
    2,
    D212Report,
    NULL
};
epicsExportAddress (drvet, drvD212);

long D212Report (int level)
{
   D212Card *pCard;
   for (pCard = firstCard; pCard; pCard = pCard->next) 
   {
      /* print a short report */
      printf("Card %d with BDF (%d,%d,%d)\n", pCard->cardNum, pCard->bus, pCard->device, pCard->function);

      /* print additional card information */
      if(level >= 1)
      {
         printf("Bridge PCI Address: 0x%08x\n",  pCard->bridgeAddr);
         printf("FPGA PCI Address: 0x%08x\n", pCard->fpgaAddr);
         printf("Interrupt Line: %d\n", pCard->intLine);
      }

      if(level >= 2)
      {
         /* print more card information */
         printf("Index: %d\n", pCard->index);
         printf("FPGA Version: 0x%08x\n", pCard->fpgaVersion);
         printf("Buffer Address: 0x%08x\n", (unsigned int) pCard->buffer);
      }
   }

   return 0;
}

/* The configure function is called from the startup script */
int D212Config (int cardNum, int index)
{
   int bus;
   int device;
   int function;
   unsigned char intLine;
   unsigned int busAddr;
   float *initData;
   int i;
   char taskName[50];;

   D212Card *pCard;
   D212Card *pCardIndex;

   /* Check card number for sanity */
   if (cardNum < 0)
   {
       fprintf (stderr, "D212Configure: cardNum %d must be >= 0\n",
                cardNum);
       return ERROR;
   } 

   /* Check index for sanity */
   if (index < 0) 
   {
       fprintf (stderr, "D212Configure: index %d must be >= 0\n",index);
       return ERROR;
   }

   /* Find end of card list and check for duplicates */
   for (pCardIndex = firstCard; pCardIndex; pCardIndex = pCardIndex->next)
   {
       if (pCardIndex->cardNum == cardNum) 
       {
           fprintf (stderr, "D212Configure: cardNum %d already in use\n", 
                   cardNum);
           return ERROR;
       }
       if (pCardIndex->index == index)
       {
           fprintf (stderr, "D212Configure: index %d already in use\n",
                   index);
           return ERROR;
       }
   }

   /* find D212 card, the actual PCI target is PLX9656 bridge chip */
   if(pciFindDevice(PLX9656_VENDOR_ID, PLX9656_DEVICE_ID, index,   
                 &bus, &device, &function) == ERROR)
   {
       fprintf (stderr, "D212Configure: fail to find D212 index %d\n",
                index);
       return ERROR;
   }
 
   /* Create new card structure */
   pCard = (D212Card*) malloc (sizeof (D212Card));
   if (!pCard) 
   {
       fprintf (stderr, "D212Config: fail to alloc pCard\n");
       return ERROR;
   }

   /* add card struct to link list */
   if (pCardIndex == NULL && firstCard == NULL)
       firstCard = pCard;
   else
   {
       for(pCardIndex=firstCard; pCardIndex->next!=NULL;
                 pCardIndex=pCardIndex->next);
       pCardIndex->next = pCard;
   }

   pCard->next = NULL;
   pCard->cardNum = cardNum;
   pCard->preTrig_offset = 0;
   
   /*将int processing元素初始化为0，标识当前自动开机程序没有运行*/
   pCard->processing = 0;


   /*BAR0 corresponds to 9656 register*/
   pciConfigInLong (bus, device, function,
                    PCI_CFG_BASE_ADDRESS_0, &busAddr);
   busAddr &= PCI_MEMBASE_MASK;
   pCard->bridgeAddr = busAddr;
 
   /*BAR2 corresponds to FPGA register*/
   pciConfigInLong (bus, device, function,
                    PCI_CFG_BASE_ADDRESS_1, &busAddr);
   busAddr &= PCI_MEMBASE_MASK;
   pCard->fpgaAddr = busAddr;

   /* store BDF and index to card structure */
   pCard->bus = bus;
   pCard->device = device;
   pCard->function = function;
   pCard->index = index;
   pCard->readDMA1 = 0;
   pCard->readDMA2 = 0;

   /* get interrupt vector */
   pciConfigInByte (bus, device, function,
                    PCI_CFG_DEV_INT_LINE, &intLine);
   pCard->intLine = intLine;

   /*pCard->intLine = getIntLine(bus,device);*/
   
   /*创建semSaveParm互斥信号量，用来多个saveParmsCardNo的任务的通讯，避免对同一文件资源的抢占，造成文件的错误操作*/
   pCard->semSaveParm = semMCreate(SEM_Q_FIFO | SEM_DELETE_SAFE);
   if(pCard->semSaveParm == NULL)
   {
        fprintf(stderr,"create semSaveParm error\n");
        return ERROR;
   }

   /* create DMA0 semphore */
   pCard->semDMA0 = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
   if( pCard->semDMA0 == NULL)
   {
       fprintf(stderr,"create semDMA0 error\n");
       return ERROR;
   }

   /* allocate data buffer */
   pCard->buffer = (int *) calloc (DMA_TRANSFER_NUM, sizeof(int));
   if (!pCard->buffer)
   {
       fprintf (stderr, "D212Config: fail to alloc buffer\n");
       return ERROR;
   }

   /* allocate processed float data buffer */
   pCard->floatBuffer = (float *) calloc (0x680B, sizeof(float));
   if (!pCard->floatBuffer)
   {
       fprintf (stderr, "D212Config: fail to alloc float buffer\n");
       return ERROR;
   }


   /*allocate ampSkew Buffer */
   pCard->ampSkewBuffer = (float *) calloc (WAVEFOMR_NUM+1, sizeof(float));
   if (!pCard->ampSkewBuffer)
   {
       fprintf (stderr, "D212Config: fail to alloc error all buffer\n");
       return ERROR;
   }
   
   initData = pCard->ampSkewBuffer;
   for(i=0; i<WAVEFOMR_NUM+1; i++)
   {
       initData[i] =  0.0;
   }


   /*allocate grid buffer */
   pCard->gridBuffer = (float *) calloc (WAVEFOMR_NUM+1, sizeof(float));
   if (!pCard->gridBuffer)
   {
       fprintf (stderr, "D212Config: fail to alloc grid buffer\n");
       return ERROR;
   }
   initData = pCard->gridBuffer;
   for(i=0; i<WAVEFOMR_NUM+1; i++)
      {
           initData[i] =  0.0;
      }

   /*allocate front buffer */
   pCard->frontBuffer = (float *) calloc (WAVEFOMR_NUM+1, sizeof(float));
   if (!pCard->frontBuffer)
   {
       fprintf (stderr, "D212Config: fail to alloc front buffer\n");
       return ERROR;
   }
   initData = pCard->frontBuffer;
   for(i=0; i<WAVEFOMR_NUM+1; i++)
      {
           initData[i] =  0.0;
      }
   /*allocate wr_rd buffer1 */
   pCard->wrRdBuffer1 = (unsigned int *) calloc (8192, sizeof(unsigned int));
   if (!pCard->wrRdBuffer1)
   {
       fprintf (stderr, "D212Config: fail to alloc wr_rd buffer\n");
       return ERROR;
   }
   initData = pCard->wrRdBuffer1;
   for(i=0; i<8192; i++)
      {
           initData[i] =  0;
      }

   /*allocate wr_rd buffer */
   pCard->wrRdBuffer2 = (unsigned int *) calloc (16640, sizeof(unsigned int));
   if (!pCard->wrRdBuffer2)
   {
       fprintf (stderr, "D212Config: fail to alloc wr_rd buffer\n");
       return ERROR;
   }
   initData = pCard->wrRdBuffer2;
   for(i=0; i<16640; i++)
      {
           initData[i] =  0;
      }

   /*allocate wr buffer 1 */
   pCard->wrBuffer1 = (float *) calloc (8192, sizeof(float));
   if (!pCard->wrBuffer1)
   {
       fprintf (stderr, "D212Config: fail to alloc wr buffer\n");
       return ERROR;
   }
   initData = pCard->wrBuffer1;
   for(i=0; i<8192; i++)
      {
           initData[i] =  1.0;
      }

   /*allocate wr data 1*/
   pCard->wdata1 = (unsigned int *) calloc (8192, sizeof(unsigned int));
   if (!pCard->wdata1)
   {
       fprintf (stderr, "D212Config: fail to alloc wr data\n");
       return ERROR;
   }
   initData = pCard->wdata1;
   for(i=0; i<8192; i++)
   {
       initData[i] =  1.0;
   }

   /*allocate wr buffer 2 */
   pCard->wrBuffer2 = (float *) calloc (16640, sizeof(float));
   if (!pCard->wrBuffer2)
   {
       fprintf (stderr, "D212Config: fail to alloc wr buffer\n");
       return ERROR;
   }
   initData = pCard->wrBuffer2;
   for(i=0; i<16640; i++)
      {
           initData[i] =  1.0;
      }

   /*allocate wr data 2*/
   pCard->wdata2 = (unsigned int *) calloc (16640, sizeof(unsigned int));
   if (!pCard->wdata2)
   {
       fprintf (stderr, "D212Config: fail to alloc wr data\n");
       return ERROR;
   }
   initData = pCard->wdata2;
   for(i=0; i<16640; i++)
   {
       initData[i] =  1.0;
   }


   pCard->errorFlag = 0;

   scanIoInit(&pCard->ioScanPvt); 

   pCard->fpgaVersion = FPGA_REG_READ32(pCard->fpgaAddr, REG_Identifier); 

   /* ensure that each interrupt line of four be connected only once */

   if(! intHasConnect[pCard->intLine - PCIE0_INT0_VEC])
   {
      /* connect ISR to interrupt, use intLine as interrupt vector */
      if(intConnect(INUM_TO_IVEC(pCard->intLine), cpciIntISR, pCard->intLine) == ERROR)
      {
         printf("intConnect error: Card %d\tintLine %d\n", pCard->cardNum, pCard->intLine);
         return ERROR;
      }

      /*enable interrupt*/
      if(intEnable(pCard->intLine) == ERROR)
      {
         printf("intEnable error: Card %d\tintLine %d\n", pCard->cardNum, pCard->intLine);
         return ERROR;
      }

      intHasConnect[pCard->intLine - PCIE0_INT0_VEC] = 1;

      printf("Card %d, intLine %d: now intConnect\n\n", pCard->cardNum, pCard->intLine);
   }
   else
   {
      printf("Card %d, intLine %d: intLine has been connected already\n\n", pCard->cardNum, pCard->intLine);
   }

   /* initialize plx9656 bridge chip */
   plx9656Init(pCard);

   /* start data process task */
   if( ERROR == taskSpawn("dataProcessTask", 52, VX_FP_TASK, 10000, (FUNCPTR) dataProcess, (int) pCard, 0, 0, 0, 0, 0, 0, 0, 0, 0))
   {
      printf("Fail to spawn data process task!\n");
   }
       
   if(cardNum == 0)
   {

    pGBuff = (int *) calloc(2048,sizeof(int));
        for(i=0;i<2048;i++){
	    *(pGBuff +i) = i;
    }

    semDMAwr1 = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
    if(semDMAwr1 == NULL)
    {
       printf(stderr,"create semDMA1 1 error\n");
       return ERROR;
    }

    semDMAwr2 = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
    if(semDMAwr2 == NULL)
    {
       printf(stderr,"create semDMA1 2 error\n");
       return ERROR;
    }

    /* start write dma task1 */
    if( ERROR == taskSpawn("writeDmaTask1", 20, VX_FP_TASK, 100000, (FUNCPTR) writeDma1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
    {
       printf("Fail to spawn data process task!\n");
       return ERROR;
    }

    /* start write dma task2 */
    if( ERROR == taskSpawn("writeDmaTask2", 20, VX_FP_TASK, 100000, (FUNCPTR) writeDma2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
    {
       printf("Fail to spawn data process task!\n");
       return ERROR;
    }

    mode = MODE_NET_DEFAULT;
    flagNetInit = NET_INIT_NULL;
    sprintf(taskName,"%s",TNAME_NETINIT);
    printf("-------------\n");
    printf(taskName);
    /* start netInit task */
    if( ERROR == taskSpawn(taskName, TPRI_NETINIT, 0, USER_STACK_SIZE, (FUNCPTR)netInit, mode, 0,0,0,0,0,0,0,0,0))
    {
      printf("Fail to spawn netInit task!\n");
      return ERROR;
    }
    printf("spawn task %s",TNAME_NETINIT);

   }

   /* print card configuration information */
   printf("Card %d successfully initialized:\n", pCard->cardNum);
   printf("BDF: %d %d %d\n", pCard->bus, pCard->device, pCard->function);
   printf("Index: %d\n", pCard->index);
   printf("Bridge PCI Address: 0x%08x\n", pCard->bridgeAddr);
   printf("FPGA PCI Address: 0x%08x\n", pCard->fpgaAddr);
   printf("Interrupt Line: %d\n", pCard->intLine);
   printf("FPGA Version: 0x%08x\n", pCard->fpgaVersion);
   printf("Buffer Address: 0x%08x\n", (unsigned int) pCard->buffer);
   printf("Start IOC!!!\n");

   return 0;
}

/*---------------------Comment for hardware register access--------------------
 * a) CPCI_WRITE8(pCard->bridgeAddr, REG_9656_DMA0_CSR, 0x09); 
 *
 *    Clear DMA0 Interrupt
 *    Following is the equivalent:
 *    regRead8 = CPCI_READ8(pCard->bridgeAddr, REG_9656_DMA0_CSR);
 *    regRead8 |= PLX9656_DMA0_INTERRUPT_CLEAR;
 *    CPCI_WRITE8(pCard->bridgeAddr, REG_9656_DMA0_CSR, regRead8);
 *
 * b) CPCI_WRITE32(pCard->bridgeAddr, REG_9656_INTCSR, 0x0f0C0900);
 *
 *    Enable LINTi#
 *    Following is the equivalent:
 *    regRead32 = CPCI_READ32(pCard->bridgeAddr, REG_9656_INTCSR);
 *    regRead32 |= PLX9656_INTCSR_LINTi_ENABLE;
 *    CPCI_WRITE32(pCard->bridgeAddr, REG_9656_INTCSR, regRead32);       
 *
 * c) CPCI_WRITE32(pCard->bridgeAddr, REG_9656_INTCSR, 0x0f0C0100);
 *
 *    Disable LINTi#, i.e. Clear Local Interrupt
 *    Following is the equivalent:
 *    regRead32 = CPCI_READ32(pCard->bridgeAddr, REG_9656_INTCSR);
 *    regRead32 &= ~PLX9656_INTCSR_LINTi_ENABLE;
 *    CPCI_WRITE32(pCard->bridgeAddr, REG_9656_INTCSR, regRead32);
 *
 * d) CPCI_WRITE32(pCard->bridgeAddr, REG_9656_DMA0_DPR, 0x00000008);
 *
 *    Select transfers from the Local Bus to the PCI Bus
 *    Following is the equivalent:
 *    regRead32 = CPCI_READ32(pCard->bridgeAddr, REG_9656_DMA0_DPR);
 *    regRead32 |= PLX9656_DMA0_DIRECT_LOC_TO_PCI;
 *    CPCI_WRITE32(pCard->bridgeAddr, REG_9656_DMA0_DPR, regRead32);
 *
 * e) CPCI_WRITE8(pCard->bridgeAddr, REG_9656_DMA0_CSR, 0x03);
 *
 *    DMA Channel 0 Start
 *    Following is the equivalent:
 *    regRead8 = CPCI_READ8(pCard->bridgeAddr, REG_9656_DMA0_CSR);
 *    regRead8 |= PLX9656_DMA0_START;
 *    CPCI_WRITE8(pCard->bridgeAddr, REG_9656_DMA0_CSR, regRead8);
 *
 * f) CPCI_WRITE32(pCard->fpgaAddr, REG_CONTROL, 0x00000002);
 *
 *    Disable FIFO and Interrupt
 *    Following is the equivalent:
 *    regRead32 = CPCI_READ32(pCard->fpgaAddr, REG_CONTROL);
 *    regRead32 &= ~ D212_CR_FIFO_ENABLE;
 *    regRead32 &= ~ D212_CR_INT_ENABLE;
 *    CPCI_WRITE32(pCard->fpgaAddr, REG_CONTROL, regRead32);
 *
 *----------------end of Comment for hardware register access------------------
 */

/* interrupt service routine, one for 8 cards, with 4 different parameters */
void cpciIntISR(int intLine)
{
   D212Card *pCard;

   /* check which card generate interrupt */
   for (pCard = firstCard; pCard; pCard = pCard->next)
   {
 /*     if(intLine == pCard->intLine)
      {*/
         /* FPGA interrupt */
         if(BRIDGE_REG_READ32(pCard->bridgeAddr, REG_9656_INTCSR) & PLX9656_INTCSR_LINTi_ACTIVE) 
         {
	    /* disable local interrupt*/
           /* BRIDGE_REG_WRITE32(pCard->bridgeAddr, REG_9656_INTCSR, 0x0f0C0100); */
	    /* clear FPGA interrupt, i.e. de-assert LINTi line */

            int_Clear(pCard);

            /* start DMA Channel 0 transfer, 8k bytes data at once */
            BRIDGE_REG_WRITE32(pCard->bridgeAddr, REG_9656_DMA0_PCI_ADR, (unsigned int) (pCard->buffer));
            BRIDGE_REG_WRITE32(pCard->bridgeAddr, REG_9656_DMA0_LOCAL_ADR, REG_AMP_Upload);
            BRIDGE_REG_WRITE32(pCard->bridgeAddr, REG_9656_DMA0_SIZE, WAVEFORM_SIZE + 4);
            BRIDGE_REG_WRITE32(pCard->bridgeAddr, REG_9656_DMA0_DPR, 0x00000008);
            BRIDGE_REG_WRITE8(pCard->bridgeAddr, REG_9656_DMA0_CSR, 0x07);
            dmaCount++;
/*
		logMsg("9656PCI_ADR,%x",BRIDGE_REG_READ32(pCard->bridgeAddr, REG_9656_DMA0_PCI_ADR),0,0,0,0,0);
		logMsg("9656ADR,%x",BRIDGE_REG_READ32(pCard->bridgeAddr, REG_9656_DMA0_LOCAL_ADR),0,0,0,0,0);
		logMsg("9656SIZE,%x",BRIDGE_REG_READ32(pCard->bridgeAddr, REG_9656_DMA0_SIZE),0,0,0,0,0);
		logMsg("9656_DPR,%x",BRIDGE_REG_READ32(pCard->bridgeAddr, REG_9656_DMA0_DPR),0,0,0,0,0);
	        logMsg("9656CSR,%x",BRIDGE_REG_READ32(pCard->bridgeAddr, REG_9656_DMA0_CSR),0,0,0,0,0);
*/

/*
	    if(intLine == 1)
		logMsg("intLine1,%d,%d,%x",pCard->bus,pCard->device,pCard->bridgeAddr,0,0,0);
	    if(intLine == 2)
		logMsg("intLine2,%d,%d,%x",pCard->bus,pCard->device,pCard->bridgeAddr,0,0,0);
	    if(intLine == 3)
		logMsg("intLine3,%d,%d,%x",pCard->bus,pCard->device,pCard->bridgeAddr,0,0,0);
	    if(intLine == 0)
		logMsg("intLine0,%d,%d,%x",pCard->bus,pCard->device,pCard->bridgeAddr,0,0,0);

*/
         }     
         /* DMA Channel 0 interrupt, Transfer Data from hardware to CPU board */
         else if(BRIDGE_REG_READ32(pCard->bridgeAddr, REG_9656_INTCSR) & PLX9656_INTCSR_DMA0_INT_ACTIVE)
         {
            /* clear DMA Channel 0 interrupt */
            BRIDGE_REG_WRITE8(pCard->bridgeAddr, REG_9656_DMA0_CSR, 0x09);
/*
	    if(intLine == 1)
		logMsg("dmaInt1,%d,%d,%x",0,0,pCard->fpgaAddr,0,0,0);
	    if(intLine == 2)
		logMsg("dmaInt2,%d,%d,%x",0,0,pCard->fpgaAddr,0,0,0);
	    if(intLine == 3)
		logMsg("dmaInt3,%d,%d,%x",0,0,pCard->fpgaAddr,0,0,0);
	    if(intLine == 0)
		logMsg("dmaInt0,%d,%d,%x",0,0,pCard->fpgaAddr,0,0,0);
*/

        /* re-enable FPGA interrupt */
     	/*BRIDGE_REG_WRITE32(pCard->bridgeAddr, REG_9656_INTCSR, 0x0f0C0900); */

            if(dmaCount < 8)
            {
               /* start DMA Channel 0 transfer, 8k bytes data at once */
               BRIDGE_REG_WRITE32(pCard->bridgeAddr, REG_9656_DMA0_PCI_ADR, (unsigned int) (pCard->buffer + 2049 * dmaCount));
               BRIDGE_REG_WRITE32(pCard->bridgeAddr, REG_9656_DMA0_LOCAL_ADR, REG_AMP_Upload + 4 * dmaCount);
               BRIDGE_REG_WRITE32(pCard->bridgeAddr, REG_9656_DMA0_SIZE, WAVEFORM_SIZE + 4);
               BRIDGE_REG_WRITE32(pCard->bridgeAddr, REG_9656_DMA0_DPR, 0x00000008);
               BRIDGE_REG_WRITE32(pCard->bridgeAddr, REG_9656_DMA0_CSR, 0x03);
               dmaCount++; 
            }
            else
            {
               dmaCount = 0;
	       /* disable dma interrupt, enable local interrupt*/
               /*BRIDGE_REG_WRITE32(pCard->bridgeAddr, REG_9656_INTCSR, 0x0f080900); */
               /* synchronize data process task */

               semGive(pCard->semDMA0);
               /*scanIoRequest(pCard->ioScanPvt);*/
            }
         }        
	 else if(BRIDGE_REG_READ32(pCard->bridgeAddr, REG_9656_INTCSR) & PLX9656_INTCSR_DMA1_INT_ACTIVE)
         {
            if(BRIDGE_REG_READ32(pCard->bridgeAddr,REG_9656_DMA1_CSR) & 0x10){
	        /* clear DMA Channel 1 interrupt */
                BRIDGE_REG_WRITE8(pCard->bridgeAddr, REG_9656_DMA1_CSR, 0x09);
	    }
         }
   /*   }*/
   }
}

void dataProcess(D212Card *pCard)
{
   int i;
   float *pDest;
   int *pSrc;
   float temp;
   short phase;
   float *pTemp1, *pTemp2;
   UINT originIntHigh;
   UINT originIntLow;
   
   /* infinite loop, used for data process */
   while(1)
   {
      /* synchronize with ISR */
      semTake(pCard->semDMA0, WAIT_FOREVER); 

    if(pCard->cardNum < 8){

      /* process waveform 1 data */
      pDest = pCard->floatBuffer + WF1_FADDR;
      pSrc = pCard->buffer + WF1_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  (pSrc[i]>>12) * CALC_WF1_MUL + CALC_WF1_ADD;
	   /* Mark for the cavity rf vol */
	   if(i == 1024)
	   {
		pCard->cav_rf_vol = pDest[i];
	   }
      }

      /* process waveform 2 data */
      pDest = pCard->floatBuffer + WF2_FADDR;
      pSrc = pCard->buffer + WF2_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  pSrc[i] * CALC_WF2_MUL + CALC_WF2_ADD;
      }

      /* calculate amplitude skew with wf1 & wf2, i.e. (wf2 - wf1) / wf1 */
      pSrc = pCard->buffer + WF1_ADDR;
      pTemp2 = pCard->floatBuffer + WF2_FADDR;
      pDest = pCard->ampSkewBuffer;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] = (pSrc[i]*1.0 - pTemp2[i]) / pTemp2[i] * 100.0;
      }

      /* process waveform 3 data */
      pDest = pCard->floatBuffer + WF3_FADDR;
      pSrc = pCard->buffer + WF3_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  pSrc[i] * CALC_WF3_MUL + CALC_WF3_ADD;
      }

      /* process waveform 4 data */
      pDest = pCard->floatBuffer + WF4_FADDR_A;
      pSrc = pCard->buffer + WF4_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
	   phase = pSrc[i]>>16;
	   pDest[i] =  phase *32 * CALC_WF4A_MUL + CALC_WF4A_ADD;
      }
      /* process waveform 4 data */
      pDest = pCard->floatBuffer + WF4_FADDR_B;
      pSrc = pCard->buffer + WF4_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  (pSrc[i] << 16) /2048 * CALC_WF4B_MUL + CALC_WF4B_ADD;
      }


      /* process waveform 5 data */
      pDest = pCard->floatBuffer + WF5_FADDR_A;
      pSrc = pCard->buffer + WF5_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
	   phase = pSrc[i]>>16;
	   pDest[i] =  phase *32 * CALC_WF5A_MUL + CALC_WF5A_ADD;
      }
      /* process waveform 5 data */
      pDest = pCard->floatBuffer + WF5_FADDR_B;
      pSrc = pCard->buffer + WF5_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  (pSrc[i] << 16) /2048 * CALC_WF5B_MUL + CALC_WF5B_ADD;
      }

      /* process waveform 6 data */
      pDest = pCard->floatBuffer + WF6_FADDR_A;
      pSrc = pCard->buffer + WF6_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           originIntHigh = ((UINT) (pSrc[i]))>>16;
	   pDest[i] =  originIntHigh * CALC_WF6A_MUL + CALC_WF6A_ADD;
      }

      /* process waveform 6 data */
      pDest = pCard->floatBuffer + WF6_FADDR_B;
      pSrc = pCard->buffer + WF6_ADDR;
      /*pSrc = pCard->buffer + WF6_ADDR;*/
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           originIntLow=((UINT) (pSrc[i]))&0x0000FFFF;
           pDest[i] =  originIntLow * CALC_WF6B_MUL + CALC_WF6B_ADD;
      }

      /* process waveform 7 data */
      
      pDest = pCard->floatBuffer + WF7_FADDR;
      pSrc = pCard->buffer + WF7_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  pSrc[i] * CALC_WF7_MUL + CALC_WF7_ADD;
      }

      /* process waveform 8 data */
      pDest = pCard->floatBuffer + WF8_FADDR;
      pSrc = pCard->buffer + WF8_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  pSrc[i] * CALC_WF8_MUL + CALC_WF8_ADD;
      }

      /* calculate gridBuffer with wf8 */
      pDest = pCard->gridBuffer;
      pSrc = pCard->buffer + WF8_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           originIntHigh = ((UINT) (pSrc[i]))>>16;
	   pDest[i] =  originIntHigh * CALC_WF8_MUL + CALC_WF8_ADD;

	   /* Mark for the front rf vol */
	   if(i == 512)
	   {
		pCard->grid_rf_vol = pDest[i];
	   }
      }

      /* calculate frontBuffer with wf8 */
      pDest = pCard->frontBuffer;
      pSrc = pCard->buffer + WF8_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           originIntLow=((UINT) (pSrc[i]))&0x0000FFFF;
	   pDest[i] =  originIntLow * CALC_WF8_MUL + CALC_WF8_ADD;
	   /* Mark for the front rf vol */
	   if(i == 512)
	   {
		pCard->front_rf_vol = pDest[i];
	   }
      }

    }/*end of if(pCard->cardNum < 8) */
    else
    {
      semGive(semSend);
      /* process waveform BPM1P data */
      pDest = pCard->floatBuffer + WF1_FADDR;
      pSrc = pCard->buffer + WF1_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  (pSrc[i]) * CALC_WFBPM_MUL + CALC_WFBPM_ADD;
      }

      /* process waveform BPM1N data */
      pDest = pCard->floatBuffer + WF2_FADDR;
      pSrc = pCard->buffer + WF2_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  pSrc[i] * CALC_WFBPM_MUL + CALC_WFBPM_ADD;
      }

      /* calculate BPM1 data */
      pTemp1 = pCard->floatBuffer + WF1_FADDR;
      pTemp2 = pCard->floatBuffer + WF2_FADDR;
      pDest = pCard->floatBuffer + WF3_FADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
	   temp = (pTemp1[i]+pTemp2[i]);
           pDest[i] = (temp)?((pTemp1[i] - pTemp2[i])*140 / temp):0.0;
      }

      /* process waveform BPM2P data */
      pDest = pCard->floatBuffer + WF4_FADDR_A;
      pSrc = pCard->buffer + WF3_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  (pSrc[i]) * CALC_WFBPM_MUL + CALC_WFBPM_ADD;
      }

      /* process waveform BPM2N data */
      pDest = pCard->floatBuffer + WF4_FADDR_B;
      pSrc = pCard->buffer + WF4_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  pSrc[i] * CALC_WFBPM_MUL + CALC_WFBPM_ADD;
      }

      /* calculate BPM2 data */
      pTemp1 = pCard->floatBuffer + WF4_FADDR_A;
      pTemp2 = pCard->floatBuffer + WF4_FADDR_B;
      pDest = pCard->floatBuffer + WF5_FADDR_A;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
	   temp = (pTemp1[i]+pTemp2[i]);
           pDest[i] = (temp)?((pTemp1[i] - pTemp2[i])*140 / temp):0.0;
      }

      /* process waveform BPM1P_Phase data */
      pDest = pCard->floatBuffer + WF5_FADDR_B;
      pSrc = pCard->buffer + WF5_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  pSrc[i] * CALC_WFBPM_PHASE_MUL + CALC_WFBPM_PHASE_ADD;
      }

      /* process waveform BPM1N_Phase data */
      pDest = pCard->floatBuffer + WF6_FADDR_A;
      pSrc = pCard->buffer + WF6_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  pSrc[i] * CALC_WFBPM_PHASE_MUL + CALC_WFBPM_PHASE_ADD;
      }

      /* process waveform BPM2P_Phase data */
      pDest = pCard->floatBuffer + WF6_FADDR_B;
      pSrc = pCard->buffer + WF7_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  pSrc[i] * CALC_WFBPM_PHASE_MUL + CALC_WFBPM_PHASE_ADD;
      }

      /* process waveform BPM2N_Phase data */
      pDest = pCard->floatBuffer + WF7_FADDR;
      pSrc = pCard->buffer + WF8_ADDR;
      for(i=1; i<WAVEFOMR_NUM+1; i++)
      {
           pDest[i] =  pSrc[i] * CALC_WFBPM_PHASE_MUL + CALC_WFBPM_PHASE_ADD;
      }

    }

      scanIoRequest(pCard->ioScanPvt);
   }
}

/*---------------------Comment for hardware register access--------------------
 * a) CPCI_WRITE32(pCard->fpgaAddr, REG_CONTROL, 0x00000007);
 *
 *    Enable FIFO, Period Generation, and Interrupt
 *    Following is the equivalent:
 *    regRead32 = CPCI_READ32(fpgaAddr, REG_CONTROL);
 *    regRead32 |= ( D212_CR_FIFO_ENABLE |
 *                   D212_CR_PERIOD_GEN |
 *                   D212_CR_INT_ENABLE );
 *    CPCI_WRITE32(fpgaAddr, REG_CONTROL, regRead32);
 *
 *----------------end of Comment for hardware register access------------------
 */

/*---------------------Comment for hardware register access--------------------
 * a) CPCI_WRITE32(bridgeAddr, REG_9656_INTCSR, 0x0f0C0900);
 *
 *    Enable PCI Interrupt, LINTi# and DMA0 Interrupt
 *    Disable LINTo#
 *    Following is the equivalent:
 *
 *    regRead32 = CPCI_READ32(bridgeAddr, REG_9656_INTCSR);
 *    regRead32 &= ~PLX9656_INTCSR_LINTo_ENABLE;
 *    regRead32 |= ( PLX9656_INTCSR_PCI_INT_ENABLE |
 *                   PLX9656_INTCSR_LINTi_ENABLE   |
 *                   PLX9656_INTCSR_DMA0_INT_ENABLE|
 *                   PLX9656_INTCSR_DMA1_INT_ENABLE );
 *    CPCI_WRITE32(bridgeAddr, REG_9656_INTCSR, regRead32);
 *
 * b) CPCI_WRITE8(bridgeAddr, REG_9656_DMA0_CSR, 0x01);
 *
 *    Enable DMA Channel 0
 *    Following is the equivalent:
 *    regRead8 = CPCI_READ8(bridgeAddr, REG_9656_DMA0_CSR);
 *    regRead8 |= PLX9656_DMA0_ENABLE;
 *    CPCI_WRITE8(bridgeAddr, REG_9656_DMA0_CSR, regRead8);
 *
 * c) CPCI_WRITE32(bridgeAddr, REG_9656_DMA0_MODE, 0x00020d43);
 *
 *    Enable Local Burst, DMA0 Done Interrupt
 *    Select 32 Data Witth, Constant Local Address, DMA0 Interrupt to INTA#
 *    Following is the equivalent:
 *    regRead32 = CPCI_READ32(bridgeAddr, REG_9656_DMA0_MODE);
 *    regRead32 |= ( PLX9656_DMA0_DATA_WIDTH_32 |
 *                   PLX9656_DMA0_LOC_BURST_ENABLE |
 *                   PLX9656_DMA0_DONE_INT_ENABLE |
 *                   PLX9656_DMA0_LOCAL_ADDR_CONST |
 *                   PLX9656_DMA0_INT_SELECT_INTA );
 *    CPCI_WRITE32(bridgeAddr, REG_9656_DMA0_MODE, regRead32);
 *
 * d) The initialization of DMA Channel 1 is equivalent to Channel 0
 *
 *----------------end of Comment for hardware register access------------------
 */

/* initialize PLX9656 bridge chip */
void plx9656Init(D212Card *pCard)
{
   int bridgeAddr = pCard->bridgeAddr;

   BRIDGE_REG_WRITE32(bridgeAddr, REG_9656_INTCSR, 0x0f0C0900); 
/*   BRIDGE_REG_WRITE32(bridgeAddr, REG_9656_INTCSR, 0x0f040900); */

   BRIDGE_REG_WRITE8(bridgeAddr, REG_9656_DMA0_CSR, 0x05); 
/*   BRIDGE_REG_WRITE32(bridgeAddr, REG_9656_DMA0_MODE, 0x00020d43); */ 
   BRIDGE_REG_WRITE32(bridgeAddr, REG_9656_DMA0_MODE, 0x00020dC3); 
        

   BRIDGE_REG_WRITE8(bridgeAddr, REG_9656_DMA1_CSR, 0x05);
   BRIDGE_REG_WRITE32(bridgeAddr, REG_9656_DMA1_MODE, 0x00020dC3);
}

void plx9656ReadBack(D212Card *pCard)
{
   int bridgeAddr = pCard->bridgeAddr;

   logMsg("regReadINTCSR %x",BRIDGE_REG_READ32(bridgeAddr, REG_9656_INTCSR),0,0,0,0,0);

   logMsg("regReadDMA0_CSR %x",BRIDGE_REG_READ32(bridgeAddr, REG_9656_DMA0_CSR),0,0,0,0,0);

   logMsg("regReadDMA0_MODE %x",BRIDGE_REG_READ32(bridgeAddr, REG_9656_DMA0_MODE),0,0,0,0,0); 
}

unsigned getIntLine(int bus, int device)
{
   if(bus == 11)
   {
      switch(device){
         case 10:
            return 0;
            break;
         case 11:
            return 1;
            break;
         case 12:
            return 2;
            break;
         case 13:
            return 3;
            break;
         case 14:
            return 0;
            break;
         case 15:
            return 1;
            break;
      }
   }
   if(bus == 12)
   {
      switch(device){
         case 11:
            return 1;
            break;
         case 12:
            return 2;
            break;
         case 13:
            return 3;
            break;
         case 14:
            return 0;
            break;
         case 15:
            return 1;
            break;
         case 8:
            return 2;
            break;
      }
   }
   if(bus == 13)
   {
      switch(device){
         case 11:
            return 2;
            break;
         case 12:
            return 3;
            break;
         case 13:
            return 0;
            break;
         case 14:
            return 1;
            break;
         case 15:
            return 2;
            break;
         case 8:
            return 3;
            break;
      }
   }
}

/* get card structure by card number */
D212Card* getCardStruct (int cardNum)
{
   D212Card* pCard;
   for (pCard = firstCard; pCard; pCard = pCard->next)
      if (pCard->cardNum == cardNum)
         return pCard;
   return NULL;
}

void writeDma1(){
   int i;
   while(1)
   {
       semTake(semDMAwr1, WAIT_FOREVER); 
       if(dmaUse == 1);
       else{
           dmaUse = 1;
           for(i=0;i<9;i++){
               int_Disable(getCardStruct(i));
           }
           taskDelay(sysClkRateGet()*1.5);
           for(i=0;i<9;i++){
	       BRIDGE_REG_WRITE8(getCardStruct(i)->bridgeAddr, REG_9656_DMA1_CSR, 0x05);
               BRIDGE_REG_WRITE32(getCardStruct(i)->fpgaAddr, 0x20, 0xaaaaaaaa);
               BRIDGE_REG_WRITE32(getCardStruct(i)->bridgeAddr, REG_9656_DMA1_PCI_ADR, (unsigned int) (getCardStruct(i)->wdata1));
               BRIDGE_REG_WRITE32(getCardStruct(i)->bridgeAddr, REG_9656_DMA1_LOCAL_ADR, 0x324);
               BRIDGE_REG_WRITE32(getCardStruct(i)->bridgeAddr, REG_9656_DMA1_SIZE, 32768);
               BRIDGE_REG_WRITE32(getCardStruct(i)->bridgeAddr, REG_9656_DMA1_DPR, 0x00000000);
               BRIDGE_REG_WRITE32(getCardStruct(i)->bridgeAddr, REG_9656_DMA1_CSR, 0x03);
           }
           taskDelay(sysClkRateGet()*1.5);
           dmaUse = 0;
           for(i=0;i<9;i++){
               getCardStruct(i)->readDMA2 = 0;
           }
           for(i=0;i<9;i++){
               getCardStruct(i)->readDMA1 = 1;
           }
       }
   }
}

void writeDma2(){
   int i;
   while(1)
   {
       semTake(semDMAwr2, WAIT_FOREVER); 
       if(dmaUse == 1);
       else{
           dmaUse = 1;
           for(i=0;i<9;i++){
               int_Disable(getCardStruct(i));
           }
           taskDelay(sysClkRateGet()*1.5);
           for(i=0;i<8;i++){
	       BRIDGE_REG_WRITE8(getCardStruct(i)->bridgeAddr, REG_9656_DMA1_CSR, 0x05);
               BRIDGE_REG_WRITE32(getCardStruct(i)->fpgaAddr, 0x24, 0xaaaaaaaa);
               BRIDGE_REG_WRITE32(getCardStruct(i)->bridgeAddr, REG_9656_DMA1_PCI_ADR, (unsigned int) (getCardStruct(i)->wdata2));
               BRIDGE_REG_WRITE32(getCardStruct(i)->bridgeAddr, REG_9656_DMA1_LOCAL_ADR, 0x32C);
               BRIDGE_REG_WRITE32(getCardStruct(i)->bridgeAddr, REG_9656_DMA1_SIZE, 66560);
               BRIDGE_REG_WRITE32(getCardStruct(i)->bridgeAddr, REG_9656_DMA1_DPR, 0x00000000);
               BRIDGE_REG_WRITE32(getCardStruct(i)->bridgeAddr, REG_9656_DMA1_CSR, 0x03);
           }
           taskDelay(sysClkRateGet()*1.5);
           dmaUse = 0;
           for(i=0;i<9;i++){
               getCardStruct(i)->readDMA1 = 0;
           }
           for(i=0;i<9;i++){
               getCardStruct(i)->readDMA2 = 1;
           }
       }
   }
}

void int_Enable (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Int_Enable, OPTION_SET);
}

void int_Disable (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Int_Enable, OPTION_CLEAR);
}

int int_Enable_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Int_Enable) == 0xAAAAAAAA)
      return 1;
   else 
      return 0;
}

void int_Clear (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Int_Clear, OPTION_SET);
}


void set_RFReset_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_RF_Reset, OPTION_SET);
}

void clear_RFReset_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_RF_Reset, OPTION_CLEAR);
}

int RFReset_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_RF_Reset) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}

void set_Sweep_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Sweep_Option, OPTION_SET);
}

void clear_Sweep_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Sweep_Option, OPTION_CLEAR);
}

int SweepOption_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Sweep_Option) == 0xAAAAAAAA)
      return 1;
   else 
      return 0;
}


void set_AMP_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_AMP_Option, OPTION_SET);
}

void clear_AMP_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_AMP_Option, OPTION_CLEAR);
}

int AMP_OPTION_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_AMP_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}

void set_AMP_FF_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_AMP_FF_Option, OPTION_SET);
}

void clear_AMP_FF_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_AMP_FF_Option, OPTION_CLEAR);
}

int AMP_FF_OPTION_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_AMP_FF_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}

void set_AMP_Modify_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_AMP_Modify_Option, OPTION_SET);
}

void clear_AMP_Modify_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_AMP_Modify_Option, OPTION_CLEAR);
}

int AMP_Modify_OPTION_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_AMP_Modify_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}

void set_Tune_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Tune_Option, OPTION_SET);
}

void clear_Tune_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Tune_Option, OPTION_CLEAR);
}

int Tune_OPTION_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Tune_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}

void set_Front_Tune_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Front_Tune_Option, OPTION_SET);
}

void clear_Front_Tune_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Front_Tune_Option, OPTION_CLEAR);
}

int Front_Tune_OPTION_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Front_Tune_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}

void set_Tune_FF_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Tune_FF_Option, OPTION_SET);
}

void clear_Tune_FF_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Tune_FF_Option, OPTION_CLEAR);
}

int Tune_FF_OPTION_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Tune_FF_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}

void set_Tune_Modify_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Tune_Modify_Option, OPTION_SET);
}

void clear_Tune_Modify_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Tune_Modify_Option, OPTION_CLEAR);
}

int Tune_Modify_OPTION_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Tune_Modify_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}


void set_Front_Tune_FF_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Front_Tune_FF_Option, OPTION_SET);
}

void clear_Front_Tune_FF_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Front_Tune_FF_Option, OPTION_CLEAR);
}

int Front_Tune_FF_OPTION_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Front_Tune_FF_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}

void set_Front_Tune_Modify_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Front_Tune_Modify_Option, OPTION_SET);
}

void clear_Front_Tune_Modify_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Front_Tune_Modify_Option, OPTION_CLEAR);
}

int Front_Tune_Modify_OPTION_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Front_Tune_Modify_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}

void set_Phase_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Phase_Option, OPTION_SET);
}

void clear_Phase_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Phase_Option, OPTION_CLEAR);
}

int Phase_OPTION_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Phase_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}

void set_Phase_FF_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Phase_FF_Option, OPTION_SET);
}

void clear_Phase_FF_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Phase_FF_Option, OPTION_CLEAR);
}

int Phase_FF_Option_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Phase_FF_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}

void set_Phase_Modify_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Phase_Modify_Option, OPTION_SET);
}

void clear_Phase_Modify_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Phase_Modify_Option, OPTION_CLEAR);
}

int Phase_Modify_Option_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Phase_Modify_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}


void set_Beam_Phase_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Beam_Phase_Option, OPTION_SET);
}

void clear_Beam_Phase_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Beam_Phase_Option, OPTION_CLEAR);
}

int Beam_Phase_Option_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Beam_Phase_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}

void set_Fre_Change_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Fre_Change_Option, OPTION_SET);
}

void clear_Fre_Change_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Fre_Change_Option, OPTION_CLEAR);
}

int Fre_Change_Option_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Fre_Change_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}

void set_Amp_Change_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Amp_Change_Option, OPTION_SET);
}

void clear_Amp_Change_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Amp_Change_Option, OPTION_CLEAR);
}

int Amp_Change_Option_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Amp_Change_Option) == 0xAAAAAAAA)
      return 1;
   else
      return 0;
}

void set_curve_Change ()
{
   int i;
   for(i=0;i<8;i++)
   {
   	FPGA_REG_WRITE32(getCardStruct(i)->fpgaAddr, REG_Amp_Change_Option, OPTION_SET);
   }
   for(i=0;i<9;i++)
   {
   	FPGA_REG_WRITE32(getCardStruct(i)->fpgaAddr, REG_Fre_Change_Option, OPTION_SET);
   }
}

void clear_curve_Change ()
{
   int i;
   for(i=0;i<8;i++)
   {
   	FPGA_REG_WRITE32(getCardStruct(i)->fpgaAddr, REG_Amp_Change_Option, OPTION_CLEAR);
   }
   for(i=0;i<9;i++)
   {
   	FPGA_REG_WRITE32(getCardStruct(i)->fpgaAddr, REG_Fre_Change_Option, OPTION_CLEAR);
   }
}

int alarm0_get (D212Card* pCard)
{
   if ((FPGA_REG_READ32(pCard->fpgaAddr, REG_Alarm)  & (BYTE_ORDER_SWAP(0x00000001))) == (BYTE_ORDER_SWAP(0x00000001)))
      return 0;
   else 
      return 1;
}
int alarm1_get (D212Card* pCard)
{
   if ((FPGA_REG_READ32(pCard->fpgaAddr, REG_Alarm) & (BYTE_ORDER_SWAP(0x00000002))) == (BYTE_ORDER_SWAP(0x00000002)))
      return 0;
   else 
      return 1;
}
int alarm2_get (D212Card* pCard)
{
   if ((FPGA_REG_READ32(pCard->fpgaAddr, REG_Alarm) & (BYTE_ORDER_SWAP(0x00000004))) == (BYTE_ORDER_SWAP(0x00000004)))
      return 0;
   else 
      return 1;
}
int alarm3_get (D212Card* pCard)
{
   if ((FPGA_REG_READ32(pCard->fpgaAddr, REG_Alarm) & (BYTE_ORDER_SWAP(0x00000008))) == (BYTE_ORDER_SWAP(0x00000008)))
      return 0;
   else 
      return 1;
}
int alarm4_get (D212Card* pCard)
{
   if ((FPGA_REG_READ32(pCard->fpgaAddr, REG_Alarm) & (BYTE_ORDER_SWAP(0x00000010))) == (BYTE_ORDER_SWAP(0x00000010)))
      return 0;
   else 
      return 1;
}
int alarm5_get (D212Card* pCard)
{
   if ((FPGA_REG_READ32(pCard->fpgaAddr, REG_Alarm) & (BYTE_ORDER_SWAP(0x00000020))) == (BYTE_ORDER_SWAP(0x00000020)))
      return 0;
   else 
      return 1;
}
int alarm6_get (D212Card* pCard)
{
   if ((FPGA_REG_READ32(pCard->fpgaAddr, REG_Alarm) & (BYTE_ORDER_SWAP(0x00000040))) == (BYTE_ORDER_SWAP(0x00000040)))
      return 0;
   else 
      return 1;
}
int alarm7_get (D212Card* pCard)
{
   if ((FPGA_REG_READ32(pCard->fpgaAddr, REG_Alarm) & (BYTE_ORDER_SWAP(0x00000080))) == (BYTE_ORDER_SWAP(0x00000080)))
      return 0;
   else 
      return 1;
}
int alarm8_get (D212Card* pCard)
{
   if ((FPGA_REG_READ32(pCard->fpgaAddr, REG_Alarm) & (BYTE_ORDER_SWAP(0x00000100))) == (BYTE_ORDER_SWAP(0x00000100)))
      return 0;
   else 
      return 1;
}

void set_Drv_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Drv_Reset, OPTION_SET);
}

void clear_Drv_Option (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Drv_Reset, OPTION_CLEAR);
}

int Drv_Reset_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Drv_Reset) == 0xAAAAAAAA)
      return 1;
   else 
      return 0;
}

void set_SG_Mode (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_SG_Mode, OPTION_SET);
}

void clear_SG_Mode (D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_SG_Mode, OPTION_CLEAR);
}

int SG_Mode_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_SG_Mode) == 0xAAAAAAAA)
      return 1;
   else 
      return 0;
}

void set_point_Sweep(D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Point_Sweep, OPTION_SET);
}

void clear_point_Sweep(D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Point_Sweep, OPTION_CLEAR);
}

int point_Sweep_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Point_Sweep) == 0xAAAAAAAA)
      return 1;
   else 
      return 0;
}

void set_beam_Int(D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_BEAM_INT, OPTION_SET);
}

void clear_beam_Int(D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_BEAM_INT, OPTION_CLEAR);
}

int beam_Int_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_BEAM_INT) == 0xAAAAAAAA)
      return 1;
   else 
      return 0;
}

void set_beam_feedforward(D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Beam_Feedforward_Option, OPTION_SET);
}

void clear_beam_feedforward(D212Card* pCard)
{
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Beam_Feedforward_Option, OPTION_CLEAR);
}

int beam_feedforward_get (D212Card* pCard)
{
   if (FPGA_REG_READ32(pCard->fpgaAddr, REG_Beam_Feedforward_Option) == 0xAAAAAAAA)
      return 1;
   else 
      return 0;
}

void set_Work_Period (D212Card* pCard, float period)
{
   unsigned int value;
   value = (unsigned int)(period * CALC_Work_Period_Set_MUL + CALC_Work_Period_Set_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Work_Period_Set, value);
}

float get_Work_Period (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Work_Period_Set) - CALC_Work_Period_Set_ADD) / CALC_Work_Period_Set_MUL;
}


void set_Fix_Frequency (D212Card* pCard, float frequency)
{
   float fvalue;
   unsigned int value;
   if(frequency>1.3)
	fvalue = 1.3;
   else if(frequency<0)
	fvalue = 0;
   else
	fvalue = frequency;
   value = (unsigned int)(fvalue * CALC_Fix_Frequency_Set_MUL + CALC_Fix_Frequency_Set_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Fix_Frequency_Set, value);
}

float get_Fix_Frequency (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Fix_Frequency_Set) - CALC_Fix_Frequency_Set_ADD) / CALC_Fix_Frequency_Set_MUL;
}

void set_AMP (D212Card* pCard, float ampSet)
{
   unsigned int value;
   value = (unsigned int)(ampSet * CALC_AMP_Set_MUL + CALC_AMP_Set_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_AMP_Set, value);
}

float get_AMP (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_AMP_Set) - CALC_AMP_Set_ADD) / CALC_AMP_Set_MUL;
}

void set_AMP_Coefficient (D212Card* pCard, float ampCoefficient)
{
   unsigned int value;
   value = (unsigned int)(ampCoefficient * CALC_AMP_Coefficient_MUL + CALC_AMP_Coefficient_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_AMP_Coefficient, value);
   
   /*调用保存参数的函数saveParms*/
   /*使用PV值覆写序号为29的文件参数*/
   if(pCard->processing == 0)
   {
	   saveParms(29, pCard->cardNum, ampCoefficient);
   }
   /*调用保存参数的函数saveParms——结束*/
}

float get_AMP_Coefficient (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_AMP_Coefficient) - CALC_AMP_Coefficient_ADD) / CALC_AMP_Coefficient_MUL;
}

void set_AMP_P (D212Card* pCard, float ampP)
{
   unsigned int value;
   value = (unsigned int)(ampP * CALC_AMP_P_Set_MUL + CALC_AMP_P_Set_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_AMP_P_Set, value);
}

float get_AMP_P (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_AMP_P_Set) - CALC_AMP_P_Set_ADD) / CALC_AMP_P_Set_MUL;
}

void set_AMP_I (D212Card* pCard, float ampI)
{
   unsigned int value;
   value = (unsigned int)(ampI * CALC_AMP_I_Set_MUL + CALC_AMP_I_Set_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_AMP_I_Set, value);
   
   /*调用保存参数的函数saveParms*/
   /*使用PV值覆写序号为6的文件参数*/
   if(pCard->processing == 0)
   {
	   saveParms(6, pCard->cardNum, ampI);
   }
   /*调用保存参数的函数saveParms——结束*/
}

float get_AMP_I (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_AMP_I_Set) - CALC_AMP_I_Set_ADD) / CALC_AMP_I_Set_MUL;
}

void set_Bias (D212Card* pCard, float bias)
{
   unsigned int value;
   value = (unsigned int)(bias * CALC_Bias_Set_MUL + CALC_Bias_Set_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Bias_Set, value);
}

float get_Bias (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Bias_Set) - CALC_Bias_Set_ADD) / CALC_Bias_Set_MUL;
}

/* Angle value, add 360 if negative value */
void set_Fix_Tuning_Angle (D212Card* pCard, float angle)
{
   unsigned int value;
   angle = angle >= 0 ? angle : angle + 360;
   value = (unsigned int)(angle * CALC_Fix_Tuning_Angle_MUL + CALC_Fix_Tuning_Angle_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Fix_Tuning_Angle, value);
}

float get_Fix_Tuning_Angle (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Fix_Tuning_Angle) - CALC_Fix_Tuning_Angle_ADD) / CALC_Fix_Tuning_Angle_MUL;
}

/* Angle value, add 360 if negative value */
void set_Tuning_Angle_Offset (D212Card* pCard, float offset)
{
   unsigned int value;
   offset = offset >= 0 ? offset : offset + 360;
   value = (unsigned int)(offset * CALC_Tuning_Angle_Offset_MUL + CALC_Tuning_Angle_Offset_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Tuning_Angle_Offset, value);
}

float get_Tuning_Angle_Offset (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Tuning_Angle_Offset) - CALC_Tuning_Angle_Offset_ADD) / CALC_Tuning_Angle_Offset_MUL;
}

void set_Tune_P (D212Card* pCard, float tuneP)
{
   unsigned int value;
   value = (unsigned int)(tuneP * CALC_Tune_P_Set_MUL + CALC_Tune_P_Set_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Tune_P_Set, value);
}

float get_Tune_P (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Tune_P_Set) - CALC_Tune_P_Set_ADD) / CALC_Tune_P_Set_MUL;
}

void set_Tune_I (D212Card* pCard, float tuneI)
{
   unsigned int value;
   value = (unsigned int)(tuneI * CALC_Tune_I_Set_MUL + CALC_Tune_I_Set_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Tune_I_Set, value);
   
   /*调用保存参数的函数saveParms*/
   /*使用PV值覆写序号为23的文件参数*/
   if(pCard->processing == 0)
   {
	   saveParms(23, pCard->cardNum, tuneI);
   }
   /*调用保存参数的函数saveParms——结束*/
}

float get_Tune_I (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Tune_I_Set) - CALC_Tune_I_Set_ADD) / CALC_Tune_I_Set_MUL;
}

void set_Tune_I_1 (D212Card* pCard, float tuneI_1)
{
   unsigned int value;
   value = (unsigned int)(tuneI_1 * CALC_Tune_I_Set1_MUL + CALC_Tune_I_Set1_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Tune_I_Set1, value);
   
   /*调用保存参数的函数saveParms*/
   /*使用PV值覆写序号为24的文件参数*/
   if(pCard->processing == 0)
   {
	   saveParms(24, pCard->cardNum, tuneI_1);
   }
   /*调用保存参数的函数saveParms——结束*/
}

float get_Tune_I_1 (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Tune_I_Set1) - CALC_Tune_I_Set1_ADD) / CALC_Tune_I_Set1_MUL;
}

void set_Tune_I_2 (D212Card* pCard, float tuneI_2)
{
   unsigned int value;
   value = (unsigned int)(tuneI_2 * CALC_Tune_I_Set2_MUL + CALC_Tune_I_Set2_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Tune_I_Set2, value);
   
   /*调用保存参数的函数saveParms*/
   /*使用PV值覆写序号为25的文件参数*/
   if(pCard->processing == 0)
   {
	   saveParms(25, pCard->cardNum, tuneI_2);
   }
   /*调用保存参数的函数saveParms——结束*/
}

float get_Tune_I_2 (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Tune_I_Set2) - CALC_Tune_I_Set2_ADD) / CALC_Tune_I_Set2_MUL;
}

void set_Tune_I_3 (D212Card* pCard, float tuneI_3)
{
   unsigned int value;
   value = (unsigned int)(tuneI_3 * CALC_Tune_I_Set3_MUL + CALC_Tune_I_Set3_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Tune_I_Set3, value);
   
   /*调用保存参数的函数saveParms*/
   /*使用PV值覆写序号为31的文件参数*/
   if(pCard->processing == 0)
   {
	   saveParms(31, pCard->cardNum, tuneI_3);
   }
   /*调用保存参数的函数saveParms——结束*/
}

float get_Tune_I_3 (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Tune_I_Set3) - CALC_Tune_I_Set3_ADD) / CALC_Tune_I_Set3_MUL;
}

void set_Front_Bias (D212Card* pCard, float frontBias)
{
   unsigned int value;
   value = (unsigned int)(frontBias * CALC_Front_Bias_Set_MUL + CALC_Front_Bias_Set_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Front_Bias_Set, value);
}

float get_Front_Bias (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Front_Bias_Set) - CALC_Front_Bias_Set_ADD) / CALC_Front_Bias_Set_MUL;
}

void set_Front_Tune_P (D212Card* pCard, float frontTuneP)
{
   unsigned int value;
   value = (unsigned int)(frontTuneP * CALC_Front_Tune_P_Set_MUL + CALC_Front_Tune_P_Set_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Front_Tune_P_Set, value);
}

float get_Front_Tune_P (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Front_Tune_P_Set) - CALC_Front_Tune_P_Set_ADD) / CALC_Front_Tune_P_Set_MUL;
}

void set_Front_Tune_I (D212Card* pCard, float frontTuneI)
{
   unsigned int value;
   value = (unsigned int)(frontTuneI * CALC_Front_Tune_I_Set_MUL + CALC_Front_Tune_I_Set_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Front_Tune_I_Set, value);
   
   /*调用保存参数的函数saveParms*/
   /*使用PV值覆写序号为30的文件参数*/
   if(pCard->processing == 0)
   {
	   saveParms(30, pCard->cardNum, frontTuneI);
   }
   /*调用保存参数的函数saveParms——结束*/
}

float get_Front_Tune_I (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Front_Tune_I_Set) - CALC_Front_Tune_I_Set_ADD) / CALC_Front_Tune_I_Set_MUL;
}

/* Angle value, add 360 if negative value */
void set_Front_Fix_Tuning_Angle (D212Card* pCard, float frontFixTunAng)
{
   unsigned int value;
   frontFixTunAng = frontFixTunAng >= 0 ? frontFixTunAng : frontFixTunAng + 360;
   value = (unsigned int)(frontFixTunAng * CALC_Front_Fix_Tuning_Angle_MUL + CALC_Front_Fix_Tuning_Angle_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Front_Fix_Tuning_Angle, value);
}

float get_Front_Fix_Tuning_Angle (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Front_Fix_Tuning_Angle) - CALC_Front_Fix_Tuning_Angle_ADD) / CALC_Front_Fix_Tuning_Angle_MUL;
}

/* Temp1 value */
void set_phase_i (D212Card* pCard, float phase_i)
{
   unsigned int value;
   value = (unsigned int)(phase_i * CALC_PHASE_I_MUL + CALC_PHASE_I_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Phase_I, value);
}

float get_Phase_i (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Phase_I) - CALC_PHASE_I_ADD) / CALC_PHASE_I_MUL;
}

/* Temp2 value */
void set_phase_p (D212Card* pCard, float phase_p)
{
   unsigned int value;
   value = (unsigned int)(phase_p * CALC_PHASE_P_MUL + CALC_PHASE_P_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Phase_P, value);
}

float get_Phase_p (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Phase_P) - CALC_PHASE_P_ADD) / CALC_PHASE_P_MUL;
}

void set_Initial_Phase (D212Card* pCard, float initial_phase)
{
   unsigned int value;
   value = (unsigned int)(initial_phase * CALC_INITIAL_PHASE_MUL + CALC_INITIAL_PHASE_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Initial_Phase, value);
}

float get_Initial_Phase (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Initial_Phase) - CALC_INITIAL_PHASE_ADD) / CALC_INITIAL_PHASE_MUL;
}

void set_FF_Delay (D212Card* pCard, float ff_delay)
{
   unsigned int value;
   value = (unsigned int)(ff_delay * CALC_FF_DELAY_MUL + CALC_FF_DELAY_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_FF_Delay, value);
}

float get_FF_Delay (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_FF_Delay) - CALC_FF_DELAY_ADD) / CALC_FF_DELAY_MUL;
}

void set_PreTrig_Delay (D212Card* pCard, float ff_delay)
{
   unsigned int value;
   value = (unsigned int)(ff_delay * CALC_PreTrig_Delay_MUL + CALC_PreTrig_Delay_ADD);
   pCard->preTrig_offset = value;
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_PreTrig_Delay, value + preTrigAll);
}

float get_PreTrig_Delay (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_PreTrig_Delay) - CALC_PreTrig_Delay_ADD) / CALC_PreTrig_Delay_MUL;
}

void set_Initial_Ref_Phase (D212Card* pCard, float ff_delay)
{
   unsigned int value;   
   ff_delay = ff_delay >= 0 ? ff_delay : ff_delay + 360;
   value = (unsigned int)(ff_delay * CALC_Initial_Ref_Phase_MUL + CALC_Initial_Ref_Phase_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Initial_Ref_Phase, value);
}

float get_Initial_Ref_Phase (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Initial_Ref_Phase) - CALC_Initial_Ref_Phase_ADD) / CALC_Initial_Ref_Phase_MUL;
}

void set_Int_Delay (D212Card* pCard, float ff_delay)
{
   unsigned int value;
   value = (unsigned int)(ff_delay * CALC_Int_Delay_MUL + CALC_Int_Delay_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Int_Delay, value);
}

float get_Int_Delay (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Int_Delay) - CALC_Int_Delay_ADD) / CALC_Int_Delay_MUL;
}



void set_Chopper_Duty (D212Card* pCard, float chopper_duty)
{
   unsigned int value;
   value = (unsigned int)(CALC_CHOPPER_DUTY_ADD - (100-chopper_duty) * CALC_CHOPPER_DUTY_MUL);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Chopper_Duty, value);
}

float get_Chopper_Duty (D212Card* pCard)
{
   return 100 - (CALC_CHOPPER_DUTY_ADD - FPGA_REG_READ32(pCard->fpgaAddr, REG_Chopper_Duty)) / CALC_CHOPPER_DUTY_MUL;
}

void set_Rf_Harmonic (D212Card* pCard, float rf_harmonic)
{
   unsigned int value;
   value = 2 - (unsigned int)(rf_harmonic * CALC_RF_HARMONIC_MUL + CALC_RF_HARMONIC_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Rf_Harmonic, value);
}

float get_Rf_Harmonic (D212Card* pCard)
{
   return 2 - (FPGA_REG_READ32(pCard->fpgaAddr, REG_Rf_Harmonic) - CALC_RF_HARMONIC_ADD) / CALC_RF_HARMONIC_MUL;
}

float get_ARC_COUNT (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_ARC_COUNT) - CALC_ARC_COUNT_ADD) / CALC_ARC_COUNT_MUL;
}

void set_All_Frequency(float all_fre)
{
   int i;
   unsigned int value;
   value = (unsigned int)(all_fre * CALC_Fix_Frequency_Set_MUL + CALC_Fix_Frequency_Set_ADD);
   for(i=0;i<9;i++){
       FPGA_REG_WRITE32(getCardStruct(i)->fpgaAddr, REG_Fix_Frequency_Set, value);
   }
}


void set_All_Beam_Phase(float all_beam_phase)
{
   int i;
   unsigned int value;
   value = (unsigned int)(all_beam_phase * CALC_Tuning_Angle_Offset_MUL + CALC_Tuning_Angle_Offset_ADD);
   for(i=0;i<8;i++){
       FPGA_REG_WRITE32(getCardStruct(i)->fpgaAddr, REG_Tuning_Angle_Offset, value);
   }
}

float get_All_Pretrig (D212Card* pCard)
{
   float value = (float)preTrigAll;
   return (value - CALC_PreTrig_Delay_ADD) / CALC_PreTrig_Delay_MUL;
}

void set_All_Pretrig(float all_preTrig)
{
   int i;
   unsigned int value;
   value = (unsigned int)(all_preTrig * CALC_PreTrig_Delay_MUL + CALC_PreTrig_Delay_ADD);

   preTrigAll = value;

   for(i=0;i<9;i++){
       
       FPGA_REG_WRITE32(getCardStruct(i)->fpgaAddr, REG_PreTrig_Delay, value + (getCardStruct(i)->preTrig_offset));
   }
}


void set_All_Amp_Coeffic(float all_ampCeffic)
{
   
   unsigned int value;
   value = (unsigned int)(all_ampCeffic * CALC_AMP_Coefficient_MUL + CALC_AMP_Coefficient_ADD);

   FPGA_REG_WRITE32(getCardStruct(3)->fpgaAddr, REG_AMP_Coefficient, value);
   FPGA_REG_WRITE32(getCardStruct(2)->fpgaAddr, REG_AMP_Coefficient, value + 0);
   FPGA_REG_WRITE32(getCardStruct(1)->fpgaAddr, REG_AMP_Coefficient, value + 0);
   FPGA_REG_WRITE32(getCardStruct(0)->fpgaAddr, REG_AMP_Coefficient, value + 0);
   FPGA_REG_WRITE32(getCardStruct(4)->fpgaAddr, REG_AMP_Coefficient, value + 0);
   FPGA_REG_WRITE32(getCardStruct(7)->fpgaAddr, REG_AMP_Coefficient, value + 0);
   FPGA_REG_WRITE32(getCardStruct(6)->fpgaAddr, REG_AMP_Coefficient, value + 0);
   FPGA_REG_WRITE32(getCardStruct(5)->fpgaAddr, REG_AMP_Coefficient, value + 0);
}

void set_EX_Phase (D212Card* pCard, float ex_phase)
{
   unsigned int value;
   value = (unsigned int)(ex_phase * CALC_EX_Phase_MUL + CALC_EX_Phase_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_EX_Phase, value);
}

float get_EX_Phase (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_EX_Phase) - CALC_EX_Phase_ADD) / CALC_EX_Phase_MUL;
}

void set_RBF_Delay (D212Card* pCard, float rbf_delay)
{
   unsigned int value;
   value = (unsigned int)(rbf_delay);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_RBF_Delay, value);
}

float get_RBF_Delay (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_RBF_Delay));
}

void set_BPM_Delay_Set (D212Card* pCard, float bpm_delay_set)
{
   unsigned int value;
   value = (unsigned int)(bpm_delay_set);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_BPM_Delay_Set, value);
}

float get_BPM_Delay_Set (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_BPM_Delay_Set));
}

void set_Chopper_Phase_Set (D212Card* pCard, float chopper_phase_set)
{
   unsigned int value;
   value = (unsigned int)(chopper_phase_set * CALC_CHOPPER_Phase_MUL + CALC_CHOPPER_Phase_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_Chopper_Phase_Set, value);
}

float get_Chopper_Phase_Set (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_Chopper_Phase_Set) - CALC_CHOPPER_Phase_ADD) / CALC_CHOPPER_Phase_MUL;
}

void set_EX_Delay_set (D212Card* pCard, float ex_delay_set)
{
   unsigned int value;
   value = (unsigned int)(ex_delay_set);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_EX_Delay_set, value);
}

void set_ref_delayAB_set(D212Card* pCard, float ref_delayAB)
{
   unsigned int value;
   value = (unsigned int)(ref_delayAB);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_REF_DELAY_AB, value);
}

float get_ref_delayAB_set (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_REF_DELAY_AB) - CALC_REF_DELAY_AB_ADD) / CALC_REF_DELAY_AB_MUL;
}

void set_ref_delayC_set(D212Card* pCard, float ref_delayC)
{
   unsigned int value;
   value = (unsigned int)(ref_delayC);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_REF_DELAY_C, value);
}

float get_ref_delayC_set (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_REF_DELAY_C) - CALC_REF_DELAY_C_ADD) / CALC_REF_DELAY_C_MUL;
}

void set_ref_delayD_set(D212Card* pCard, float ref_delayD)
{
   unsigned int value;
   value = (unsigned int)(ref_delayD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_REF_DELAY_D, value);
}

float get_ref_delayD_set (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_REF_DELAY_D) - CALC_REF_DELAY_D_ADD) / CALC_REF_DELAY_D_MUL;
}

void set_amp_i_injection (D212Card* pCard, float amp_i_injection)
{
   unsigned int value;
   value = (unsigned int)(amp_i_injection * CALC_AMP_I_Set_MUL + CALC_AMP_I_Set_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_AMP_I_Injection, value);
}

float get_amp_i_injection (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_AMP_I_Injection) - CALC_AMP_I_Set_ADD) / CALC_AMP_I_Set_MUL;
}

void set_phase_i_injection (D212Card* pCard, float phase_i_injection)
{
   unsigned int value;
   value = (unsigned int)(phase_i_injection * CALC_INJ_PHASE_I_MUL + CALC_INJ_PHASE_I_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_PHASE_I_Injection, value);
}

float get_phase_i_injection (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_PHASE_I_Injection) - CALC_INJ_PHASE_I_ADD) / CALC_INJ_PHASE_I_MUL;
}

void set_beam_ff_coef (D212Card* pCard, float beam_ff_coef)
{
   unsigned int value;
   value = (unsigned int)(beam_ff_coef * CALC_AMP_Coefficient_MUL + CALC_AMP_Coefficient_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_BEAM_FF_COEF, value);
}

float get_beam_ff_coef (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_BEAM_FF_COEF) - CALC_AMP_Coefficient_ADD) / CALC_AMP_Coefficient_MUL;
}


void set_Syn_Osc_Delay (D212Card* pCard, float syn_osc_delay)
{
   unsigned int value;
   value = (unsigned int)(syn_osc_delay * CALC_Syn_Osc_Delay_MUL + CALC_Syn_Osc_Delay_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_SYN_OSC_DELAY, value);
}

float get_Syn_Osc_Delay (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_SYN_OSC_DELAY) - CALC_Syn_Osc_Delay_ADD) / CALC_Syn_Osc_Delay_MUL;
}


void set_Syn_Osc_Count (D212Card* pCard, float syn_osc_count)
{
   unsigned int value;
   value = (unsigned int)(syn_osc_count * CALC_Syn_Osc_Count_MUL + CALC_Syn_Osc_Count_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_SYN_OSC_COUNT, value);
}

float get_Syn_Osc_Count (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_SYN_OSC_COUNT) - CALC_Syn_Osc_Count_ADD) / CALC_Syn_Osc_Count_MUL;
}


void set_beam_ff_phase_set (D212Card* pCard, float beam_ff_phase_set)
{
   unsigned int value;
   value = (unsigned int)(beam_ff_phase_set * CALC_INITIAL_PHASE_MUL + CALC_INITIAL_PHASE_ADD);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_BEAM_FF_PHASE_SET, value);
}

float get_beam_ff_phase_set (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_BEAM_FF_PHASE_SET) - CALC_INITIAL_PHASE_ADD) / CALC_INITIAL_PHASE_MUL;
}

void set_beam_ff_delay(D212Card* pCard, float beam_ff_delay)
{
   unsigned int value;
   value = (unsigned int)(beam_ff_delay);
   FPGA_REG_WRITE32(pCard->fpgaAddr, REG_BEAM_FF_DELAY, value);
}

float get_beam_ff_delay (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_BEAM_FF_DELAY) - CALC_BEAM_FF_DELAY_ADD) / CALC_BEAM_FF_DELAY_MUL;
}


float get_EX_Delay_set (D212Card* pCard)
{
   return (FPGA_REG_READ32(pCard->fpgaAddr, REG_EX_Delay_set));
}

float get_Front_RF_Vol(D212Card* pCard)
{
   return (pCard->front_rf_vol);
}

float get_Cav_RF_Vol(D212Card* pCard)
{
   return (pCard->cav_rf_vol);
}

float get_Grid_RF_Vol(D212Card* pCard)
{
   return (pCard->grid_rf_vol);
}

/* This is for tcp ip server program */
STATUS netInit(int mode){
	struct sockaddr_in serverAddr;
	struct sockaddr_in clientAddr;
	int sockAddrSize;
	int taskId;
	int i;
	char taskName[50];;

	char optval = 1;

/*        pCard = getCardStruct(cardNum); */
	
	
	if(flagNetInit == NET_INIT_COMMUSKT){
		logMsg("netInit: server has already been initialized\n",0,0,0,0,0,0);
		return(OK);
	}
	else if(flagNetInit == NET_INIT_NULL){
		if(mode == MODE_NET_DEFAULT){
			if((listenSkt = socket(AF_INET,SOCK_STREAM,0)) == ERROR){
				printf("netInit: can not open listen socket\n");
				return(ERROR);
			}
			
			sockAddrSize = sizeof(struct sockaddr_in);
			bzero((char *) &serverAddr, sockAddrSize);
			serverAddr.sin_family = AF_INET;
			serverAddr.sin_len = (u_char) sockAddrSize;
			serverAddr.sin_port = htons(LOCAL_SERVER_PORT);
			serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

			if(bind(listenSkt,(struct sockaddr *) &serverAddr,sockAddrSize) == ERROR){
				printf("netInit: unable bind to port %d\n",LOCAL_SERVER_PORT);
				close(listenSkt);
				return(ERROR);
			}
			
			printf("netInit: successfully bind to port\n");
			if(listen(listenSkt,1) == ERROR){
				printf("netInit: can not listen to listen socket\n");
				close(listenSkt);
				return(ERROR);
			}
			else{
				flagNetInit = NET_INIT_LISTENSKT;
			}
		}
	}

	commuSkt = accept(listenSkt,(struct sockaddr *)(&clientAddr), &sockAddrSize);
	
	if(commuSkt == ERROR){
		printf("netInit: can not accept command socket\n");
		close(commuSkt);
		return(ERROR);
	}
	else{
		printf("netInit: netInit finished\n");
		setsockopt(commuSkt, IPPROTO_TCP,TCP_NODELAY,&optval,sizeof(optval));
		flagNetInit = NET_INIT_COMMUSKT;
	}

	if(flagNetInit == NET_INIT_COMMUSKT){
		sprintf(taskName, "%s",TNAME_NETCHECKLINK);
		taskId = taskNameToId(taskName);
		if(taskId != taskIdSelf() && taskId != ERROR){
			taskDelete(taskId);
		}
		taskSpawn(taskName, TPRI_NETCHECKLINK, 0, USER_STACK_SIZE, (FUNCPTR)netCheckLink,0,0,0,0,0,0,0,0,0,0);

		sprintf(taskName, "%s",TNAME_NETSEND);
		taskId = taskNameToId(taskName);
		if(taskId != taskIdSelf() && taskId != ERROR){
			taskDelete(taskId);
		}
		taskSpawn(taskName, TPRI_NETSEND, 0, USER_STACK_SIZE, (FUNCPTR)netSend,0,0,0,0,0,0,0,0,0,0);
	}
	semSend = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
	
	return(OK);
}

STATUS netCheckLink(){
	char taskName[50];;
	semCmdLink = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
	semTake(semCmdLink, WAIT_FOREVER);
	logMsg("netCheckLink: semCmdLink taked\n",0,0,0,0,0,0);
	semDelete(semCmdLink);
	sprintf(taskName, "%s",TNAME_NETCLOSEALL);
	taskSpawn(taskName, TPRI_NETCLOSEALL, 0, USER_STACK_SIZE, (FUNCPTR)netCloseAll, MODE_NET_REINIT, 0,0,0,0,0,0,0,0,0);
	return(OK);
}

STATUS netSend(){
	int buffLen;
	int acLen;
	int acTotal = 0;
	int i,j;
	D212Card *pCard = NULL;
	char *p = NULL;
	buffLen = 2049 * sizeof(int);
	

	while(flagNetInit == NET_INIT_COMMUSKT){
		semTake(semSend, WAIT_FOREVER);
	    for(j=0;j<8;j++){
		pCard = getCardStruct(j);
		p = (char *)(pCard->buffer);
		for(i=0;i<8;i++){
			acTotal = 0;
			while(acTotal < buffLen){
				acLen = send(commuSkt, (char *)(p + i*8196 + acTotal), buffLen - acTotal, 0);
				if(acLen == ERROR){
					printf("netSend: error in sending buff to client\n");
					semGive(semCmdLink);
					return(ERROR);
				}
				acTotal = acTotal + acLen;
			}
		}
	    }
		/*taskDelay(sysClkRateGet()/100);*/
	}
	
	if(flagNetInit == NET_INIT_NULL)
		printf("netSend: server has not been initialized\n");
	else if(flagNetInit == NET_INIT_LISTENSKT){
		printf("netSend: no connection created yet\n");
		printf("netSend: please connect to VxWorks server with PC program\n");
	}
	else{
		printf("netSend: net connection is shut down\n");
	}
	return(ERROR);
}

void netCloseAll(int mode){
	int taskId;
	char taskName[50];;

	sprintf(taskName, "%s",TNAME_NETSEND);
	taskId = taskNameToId(taskName);
	if(taskId != taskIdSelf() && taskId != ERROR){
		taskDelete(taskId);
	}

	sprintf(taskName, "%s",TNAME_NETCHECKLINK);
	taskId = taskNameToId(taskName);
	if(taskId != taskIdSelf() && taskId != ERROR){
		taskDelete(taskId);
	}

	if(mode == MODE_NET_DEFAULT){
		if(flagNetInit == NET_INIT_COMMUSKT){
			close(commuSkt);
			close(listenSkt);
		}
		else if(flagNetInit == NET_INIT_LISTENSKT){
			close(listenSkt);
		}
		flagNetInit = NET_INIT_NULL;
		logMsg("netCloseAll: net connection shut down\n",0,0,0,0,0,0);
	}
	else if(mode == MODE_NET_REINIT){
		if(flagNetInit == NET_INIT_COMMUSKT){
			close(commuSkt);
		}
		sprintf(taskName, "%s",TNAME_NETINIT);
		taskId = taskNameToId(taskName);
		if(taskId != taskIdSelf() && taskId != ERROR){
			taskDelete(taskId);
		}
		semDelete(semSend);
		flagNetInit = NET_INIT_NULL;
		printf("netCloseAll: net connection shut down\n");
		printf("netCloseAll: net connection will restart\n");
		taskSpawn(taskName, TPRI_NETINIT, 0, USER_STACK_SIZE, (FUNCPTR)netInit, mode, 0,0,0,0,0,0,0,0,0);
	}
}

int autoOn(int cardNum)
{
	char taskName[10];
	
	sprintf(taskName,"%s%d","autoOnCardNo",cardNum); /*在每个自动开机任务名的后面加上板卡号，用来识别开机的系统*/
	/* 发起自动开机的任务 */
	if( ERROR == taskSpawn(taskName, 100, VX_FP_TASK, 100000, (FUNCPTR) autoOnCardNo, cardNum, 0, 0, 0, 0, 0, 0, 0, 0, 0))
	{
	   printf("Fail to spawn autoOn task!\n");
	   return ERROR;
	}
}

int autoOff(int cardNum)
{
	char taskName[10];
	
	sprintf(taskName,"%s%d","autoOffCardNo",cardNum); /*在每个自动关机任务名的后面加上板卡号，用来识别关机的系统*/
	/* 发起自动关机的任务 */
	if( ERROR == taskSpawn(taskName, 99, VX_FP_TASK, 100000, (FUNCPTR) autoOffCardNo, cardNum, 0, 0, 0, 0, 0, 0, 0, 0, 0))
	{
	   printf("Fail to spawn autoOff task!\n");
	   return ERROR;
	}
}

/*自动开机任务的执行函数*/
void autoOnCardNo(int cardNum)
{
	D212Card *pCard;
	
	FILE * fp;
	
	double parms[35]={0}; /*自动开机所用到的参数为35个参数*/
	
	char fileName[20];
	
	char charParm[20];
	
	int i = 0;
	
	unsigned int value; /*value used to set the FPGA reg*/
	
	pCard = getCardStruct(cardNum); /*获取对应板卡号的FPGA板卡的资源<结构体>*/
	
	pCard->processing = 1; /*将板卡结构体中int processing元素赋值为1，用来标识板卡开始运行自动开机程序*/
	
	sprintf(fileName,"parmCardNo%d.txt",cardNum); /*存放参数的文件名为parmCardNo[cardNum].txt*/
	/*读取文件中的各参数并存入数组中*/
	if((fp = fopen(fileName,"r")) == NULL) /*打开参数文件*/
	{
		printf("can't open file\n");
	}
	
	while(fscanf(fp,"%s",charParm) == 1) /*循环读取文件的每个参数并存入字符串charParm中*/
	{
		parms[i] = atof(charParm); /*将读取到的字符串参数转化为double类型的数据并存入到数组parms中*/
                i++;
	}
	
	rewind(fp); /*将文件指针返回到文件的开头*/
	
	/*关闭文件*/
	if(fclose(fp) != 0)
	{
		printf("Error in closing file\n");
	}
	
	/*对FPGA寄存器进行写操作，正式进入自动开机的过程*/
	/*设置点频频率*/
	set_Fix_Frequency (pCard, parms[0]);
	
	/*设置工作的脉冲周期*/
	set_Work_Period (pCard, parms[1]);
	
	/*设置点频下幅度*/
	set_AMP (pCard, parms[2]);
	
	/*设置FF Delay的值*/
	set_FF_Delay (pCard, parms[3]);
	
	/*设置幅度曲线的系数*/
	set_AMP_Coefficient (pCard, parms[4]);
	
	/*设置幅度闭环的P参数*/
	set_AMP_P (pCard, parms[5]);
	
	/*设置幅度闭环的I参数*/
	set_AMP_I (pCard, parms[6]);
	
	/*设置调谐闭环大偏流的值*/
	set_Bias (pCard, parms[7]);
	
	/*设置调谐闭环的相位角*/
	set_Fix_Tuning_Angle (pCard, parms[8]);
	
	/*设置调谐闭环的P值*/
	set_Tune_P (pCard, parms[9]);
	
	/*设置调谐闭环的I值*/
	set_Tune_I (pCard, parms[10]);
	
	/*设置调谐闭环的I1值*/
	set_Tune_I_1 (pCard, parms[11]);
	
	/*设置调谐闭环的I2值*/
	set_Tune_I_2 (pCard, parms[12]);
	
	/*设置调谐闭环的I3值*/
	set_Tune_I_3 (pCard, parms[13]);
	
	/*设置栅极调谐闭环的小偏流值*/
	set_Front_Bias (pCard, parms[14]);
	
	/*设置栅极调谐闭环的相位角*/
	set_Front_Fix_Tuning_Angle (pCard, parms[15]);
	
	/*设置栅极调谐闭环的P值*/
	set_Front_Tune_P (pCard, parms[16]);
	
	/*设置栅极调谐的I值*/
	set_Front_Tune_I (pCard, parms[17]);
	
	/*设置相位闭环的初始相位*/
	set_Initial_Phase (pCard, parms[18]);
	
	/*设置相位闭环的P值*/
	set_phase_p (pCard, parms[19]);
	
	/*设置相位闭环的I值*/
	set_phase_i (pCard, parms[20]);
	
	pCard->processing = 10; /*标识自动开机过程参数初始化完成*/
	
	/*切换到扫频模式*/
	set_point_Sweep(pCard);
	
	/*切换到脉冲工作模式*/
	set_Sweep_Option (pCard);
	
	/*切换sg到脉冲模式*/
	set_SG_Mode (pCard);
	
	/*开幅度闭环*/
	clear_AMP_Option (pCard);
	
	/*关幅度闭环前馈功能*/
	clear_AMP_FF_Option (pCard);
	
	/*关幅度闭环前馈表计算功能*/
	clear_AMP_Modify_Option (pCard);
	
	/*开调谐闭环*/
	clear_Tune_Option (pCard);
	
	/*关调谐闭环前馈功能*/
	clear_Tune_FF_Option (pCard);
	
	/*关调谐闭环前馈表计算功能*/
	clear_Tune_Modify_Option (pCard);
	
	/*开栅极调谐闭环*/
	clear_Front_Tune_Option (pCard);
	
	/*开相位闭环*/
	clear_Phase_Option (pCard);
	
	/*关相位闭环前馈功能*/
	clear_Phase_FF_Option (pCard);
	
	/*关相位闭环前馈表计算功能*/
	clear_Phase_Modify_Option (pCard);
	
	/*复位中断*/
	set_RFReset_Option (pCard);
	
	/*复位驱动*/
	set_Drv_Option (pCard);
	
	/*复位中断*/
	set_RFReset_Option (pCard);
	
	/*复位驱动*/
	set_Drv_Option (pCard);
	
	pCard->processing = 20; /*标识自动开机过程开关量初始化完成*/
	
	/*启动幅度闭环前馈表计算功能*/
	set_AMP_Modify_Option (pCard);
	
	/*启动调谐闭环前馈表计算功能*/
	set_Tune_Modify_Option (pCard);
	
	/*启动相位闭环前馈表计算功能*/
	set_Phase_Modify_Option (pCard);
	
	/*延时0.5s*/
	/*任务主动放弃CPU资源进入延时态，此时其他同级别的任务可以获取CPU资源并运行*/
	taskDelay(sysClkRateGet()/2);
	
	/*复位中断*/
	set_RFReset_Option (pCard);
	
	/*复位驱动*/
	set_Drv_Option (pCard);
	
	/*复位中断*/
	set_RFReset_Option (pCard);
	
	/*复位驱动*/
	set_Drv_Option (pCard);
	
	pCard->processing = 30; /*标识自动开机过程前馈计算启动完成*/
	
	/*升点频幅度值*/
	set_AMP (pCard, parms[21]);
	
	/*延时1s*/
	taskDelay(sysClkRateGet());
	
	/*闭调谐闭环*/
	set_Tune_Option (pCard);
	
	/*延时0.5s*/
	taskDelay(sysClkRateGet());
	
	/*闭栅极调谐闭环*/
	set_Front_Tune_Option (pCard);
	
	pCard->processing = 40; /*标识调谐闭环和栅极调谐闭环完成*/
	
	/*延时5s*/
	taskDelay(sysClkRateGet() * 5);
	
	/*加调谐闭环前馈功能*/
	set_Tune_FF_Option (pCard);
	
	/*延时5s*/
	taskDelay(sysClkRateGet() * 5 * 1.6);
	
	/*关调谐闭环前馈表计算功能，固定前馈表的值*/
	clear_Tune_Modify_Option (pCard);
	
	pCard->processing = 60; /*标识调谐闭环前馈修正完成*/
	
	/*调整栅极调谐闭环的I值*/
	set_Front_Tune_I (pCard, parms[22]);
	
	/*延时5s*/
	taskDelay(sysClkRateGet() * 5);
	
	/*闭幅度闭环*/
	set_AMP_Option (pCard);
	
	/*延时0.5s*/
	taskDelay(sysClkRateGet()/2);
	
	/*调整调谐闭环I的值*/
	set_Tune_I (pCard, parms[23]);
	
	/*调整调谐闭环I1的值*/
	set_Tune_I_1 (pCard, parms[24]);
	
	/*调整调谐闭环I2的值*/
	set_Tune_I_2 (pCard, parms[25]);
	
	/*调整调谐闭环I3的值*/
	set_Tune_I_3 (pCard, parms[26]);
	
	pCard->processing = 70; /*标识幅度闭环、调谐闭环和栅极调谐闭环I值调整完成*/
	
	/*延时0.5s*/
	taskDelay(sysClkRateGet()/2);
	
	/*加幅度闭环前馈功能*/
	set_AMP_FF_Option (pCard);
	
	/*延时0.5s*/
	taskDelay(sysClkRateGet()/2);
	
	/*升扫频幅度曲线系数*/
	set_AMP_Coefficient (pCard, parms[27]);
	
		/*延时0.5s*/
	taskDelay(sysClkRateGet()/2);
	
	/*升扫频幅度曲线系数*/
	set_AMP_Coefficient (pCard, parms[28]);
	
	/*延时0.5s*/
	taskDelay(sysClkRateGet()/2);
	
	/*升扫频幅度曲线系数*/
	set_AMP_Coefficient (pCard, parms[29]);
	
	pCard->processing = 90; /*标识加幅度闭环前馈功能和升幅度系数完成*/
	
	/*最后调整栅极调谐闭环I值*/
	set_Front_Tune_I (pCard, parms[30]);
	
	/*最后调整调谐闭环I3值*/
	set_Tune_I_3 (pCard, parms[31]);
	
	
	/*延时1s*/
	taskDelay(sysClkRateGet());
	
	/*再次计算调谐前馈表的值*/
	set_Tune_Modify_Option (pCard);
	
	/*延时0.5s*/
	taskDelay(sysClkRateGet()/2);
	
	/*闭相位闭环*/
	set_Phase_Option (pCard);
	
	/*延时0.5s*/
	taskDelay(sysClkRateGet()/2);
	
	/*加相位闭环前馈功能*/
	set_Phase_FF_Option (pCard);
	
	/*设置腔间相位*/
	set_Initial_Ref_Phase (pCard, parms[32]);
	
	/*设置FF DELAY的值*/
	set_FF_Delay (pCard, parms[33]);
	
	/*设置RBF DELAY的值*/
	set_RBF_Delay (pCard, parms[34]);
	
	pCard->processing = 100; /*最后的栅极调谐闭环I值和调谐闭环I3值调整完成、相位闭环完成和腔间相位以及电缆延时补偿设置完成*/
	
	/*延时1s*/
	taskDelay(sysClkRateGet());
	
	pCard->processing = 0;  /*标识自动开机任务的完成*/
}

void autoOffCardNo(int cardNum)
{
	D212Card *pCard;
	
	char taskName[50];
	
	int taskId;
	
	pCard = getCardStruct(cardNum); /*获取对应板卡号的FPGA板卡的资源<结构体>*/
	
	/*如果对应板卡上的自动开机任务正在运行则删除该任务*/
	if(pCard->processing != 0)
	{
		sprintf(taskName, "%s%d","autoOnCardNo",cardNum);
		taskId = taskNameToId(taskName);
		if(taskId != taskIdSelf() && taskId != ERROR)
		{
			taskDelete(taskId);
		}
	}
	
	/*如果此时没有正在运行的自动开机任务，需要在自动关机任务进入延时状态时禁止运行自动开机任务*/
	if(pCard->processing == 0)
	{
		pCard->processing = 200;
	}
	
	/*开相位闭环*/
	clear_Phase_Option (pCard);
	
	/*延时0.5s*/
	taskDelay(sysClkRateGet()/2);
	
	/*开栅极调谐闭环*/
	clear_Front_Tune_Option (pCard);
	
	/*延时0.5s*/
	taskDelay(sysClkRateGet()/2);
	
	/*点频下幅度降为0*/
	set_AMP (pCard, 0);
	
	/*停止计算幅度闭环前馈表的值*/
	clear_AMP_Modify_Option (pCard);
	
	/*停止计算调谐闭环前馈表的值*/
	clear_Tune_Modify_Option (pCard);
	
	/*停止计算相位闭环前馈表的值*/
	clear_Phase_Modify_Option (pCard);
	
	/*关调谐闭环前馈功能*/
	clear_Tune_FF_Option (pCard);
	
	/*关幅度闭环前馈功能*/
	clear_AMP_FF_Option (pCard);
	
	/*关相位闭环前馈功能*/
	clear_Phase_FF_Option (pCard);
	
	pCard->processing = 0; /*此时可以允许自动开机任务的运行了*/
}

int saveParms(int index, int cardNum, double val)
{
	char taskName[10];
	int val2int;
	
	sprintf(taskName,"%s%d","saveParmsCardNo",cardNum); /*在每个保存参数任务名的后面加上板卡号，用来识别保存的是第几套高频系统的参数*/
	
	/*因为传递给任务的参数只能为整数，因此将其保留小数点后3位，再乘上1000*/
	/*在保存参数任务中需要将该值转换为double类型再除以1000*/
	if(0 < val < 100)
	{
		val2int = (int)(val * 1000);
		/* 发起保存参数的任务 */
		/*保存参数的任务的具体的执行函数为saveParmsCardNo*/
		/*任务的优先级别设置为较低的优先级别，低于之前的自动开关机程序以及其它的数据采集任务等*/
		/*这样可以不影响系统其他较为重要功能的运行*/
		if( ERROR == taskSpawn(taskName, 150, VX_FP_TASK, 100000, (FUNCPTR) saveParmsCardNo, index, cardNum, val2int, 0, 0, 0, 0, 0, 0, 0))
		{
		   printf("Fail to spawn saveParms task!\n");
		   return ERROR;
		}
	}
	else
	{
		return ERROR;
	}
	return OK;
}

void saveParmsCardNo(int index, int cardNum, int val2int)
{
	D212Card *pCard;
	double val;
	FILE * fp;
	char fileName[20];
	char charParm[20];
	int i;
        double parms[35]={0}; /*自动开机所用到的参数为35个参数*/
	
	pCard = getCardStruct(cardNum); /*获取对应板卡号的FPGA板卡的资源<结构体>*/
	semTake(pCard->semSaveParm, WAIT_FOREVER);
	
	val = ((double) val2int) / 1000; /*将之前saveParms函数传递过来的PV的值恢复为double类型，并保留小数点后3位精度*/
	sprintf(fileName,"parmCardNo%d.txt",cardNum); /*存放参数的文件名为parmCardNo[cardNum].txt*/
	
	/*读取文件中的各参数并存入数组中*/
	if((fp = fopen(fileName,"r")) == NULL) /*打开参数文件*/
	{
		printf("can't open file\n");
	}
	
        i = 0;
	while(fscanf(fp,"%s",charParm) == 1) /*循环读取文件的每个参数并存入字符串charParm中*/
	{
		parms[i] = atof(charParm); /*将读取到的字符串参数转化为double类型的数据并存入到数组parms中*/
                i++;
	}
	
	rewind(fp); /*将文件指针返回到文件的开头*/
	
	/*关闭文件*/
	if(fclose(fp) != 0)
	{
		printf("Error in closing file\n");
	}

        parms[index] = val;

        /*open parms file for write*/
	if((fp = fopen(fileName,"w")) == NULL) /*打开参数文件*/
	{
		printf("can't open file\n");
	}

	for(i =0; i< 35; i++)
        {
            fprintf(fp,"%f\n",parms[i]);
	}

        rewind(fp);
	/*关闭文件*/
	if(fclose(fp) != 0)
	{
		printf("Error in closing file\n");
	}

	/*释放互斥信号量*/
	semGive(pCard->semSaveParm);
}
