## Example vxWorks startup file

## The following is needed if your board support package doesn't at boot time
## automatically cd to the directory containing its startup script
cd "/home/long/nfsDir/asExample"


## You may have to change asExample to something else
## everywhere it appears in this file
ld <bin/vxWorks-mpc8572/asExample.munch

## epicsEnvSet("EPICS_TS_MIN_WEST", -360)
## epicsEnvSet("EPICS_TS_NTP_INET","192.168.0.68")

dbLoadDatabase "dbd/asExample.dbd"
asExample_registerRecordDeviceDriver pdbbase

putenv("TIMEZONE=UTC::-480:0:0")

save_restoreSet_Debug(0)
save_restoreSet_IncompleteSetsOk(1)
save_restoreSet_DatedBackupFiles(1)
set_savefile_path("/home/long/nfsDir","");
set_requestfile_path("/home/long/nfsDir","")
set_pass0_restoreFile("auto_settings.sav")
set_pass1_restoreFile("auto_settings.sav")

save_restoreSet_NumSeqFiles(5)
save_restoreSet_SeqPeriodInSeconds(600)
save_restoreSet_RetrySeconds(60)
save_restoreSet_CAReconnect(1)
save_restoreSet_CallbackTimeout(-1)
save_restoreSet_NFSHost("RCSRFDevelopment","192.168.0.88")

## Load record instances
dbLoadTemplate "db/user.substitutions"
dbLoadRecords "db/dbSubExample.db", "user=long"

## Set this to see messages from mySub
#mySubDebug = 1

## Run this to trace the stages of iocInit
#traceIocInit


iocInit

create_monitor_set("auto_settings.req",5,"")

## Start any sequence programs
#seq &sncExample, "user=long"
