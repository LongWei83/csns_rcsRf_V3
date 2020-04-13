## Example vxWorks startup file

## The following is needed if your board support package doesn't at boot time
## automatically cd to the directory containing its startup script

## 将VxWorks的同优先级的任务调度方式设置为：时间片轮转
## 时间片的长度为1/60 (s)

kernelTimeSlice(sysClkRateGet()/60)

cd "/home/long/nfsDir/rcsRfIoc_v3"

ld <bin/vxWorks-mpc8572/rcsRfIoc.munch

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","13107200")
##epicsEnvSet("EPICS_TS_MIN_WEST", -420)
##epicsEnvSet("EPICS_TS_NTP_INET","192.168.0.68")

dbLoadDatabase "dbd/rcsRfIoc.dbd"
rcsRfIoc_registerRecordDeviceDriver pdbbase
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
dbLoadRecords "db/rcsRfGl.db"
dbLoadRecords "db/rcsRf1.db"
dbLoadRecords "db/rcsRf2.db"
dbLoadRecords "db/rcsRf3.db"
dbLoadRecords "db/rcsRf4.db"
dbLoadRecords "db/rcsRf5.db"
dbLoadRecords "db/rcsRf6.db"
dbLoadRecords "db/rcsRf7.db"
dbLoadRecords "db/rcsRf8.db"
dbLoadRecords "db/rcsRf9.db"
dbLoadRecords "db/rcsRf10.db"
dbLoadRecords "db/rcsRf11.db"
dbLoadRecords "db/calcPV.db"

#D212Config(cardNum,index)
D212Config(0,0)
D212Config(1,1)
D212Config(2,2)
D212Config(3,3)
D212Config(4,4)
D212Config(5,5)
D212Config(6,6)
D212Config(7,7)
D212Config(8,8)
D212Config(9,9)
D212Config(10,10)

iocInit

create_monitor_set("auto_settings.req",5,"")

dbpf "rcsRf1:Int_Delay_set","0"
dbpf "rcsRf2:Int_Delay_set","1.5"
dbpf "rcsRf3:Int_Delay_set","3"
dbpf "rcsRf4:Int_Delay_set","4.5"
dbpf "rcsRf5:Int_Delay_set","6"
dbpf "rcsRf6:Int_Delay_set","7.5"
dbpf "rcsRf7:Int_Delay_set","9"
dbpf "rcsRf8:Int_Delay_set","10.5"
dbpf "rcsRf9:Int_Delay_set","12"
dbpf "rcsRf10:Int_Delay_set","13.5"
dbpf "rcsRf11:Int_Delay_set","15"

dbpf "rcsRf1:workPeriod_set","40"
dbpf "rcsRf2:workPeriod_set","40"
dbpf "rcsRf3:workPeriod_set","40"
dbpf "rcsRf4:workPeriod_set","40"
dbpf "rcsRf5:workPeriod_set","40"
dbpf "rcsRf6:workPeriod_set","40"
dbpf "rcsRf7:workPeriod_set","40"
dbpf "rcsRf8:workPeriod_set","40"
dbpf "rcsRf9:workPeriod_set","40"
dbpf "rcsRf10:workPeriod_set","40"
dbpf "rcsRf11:workPeriod_set","40"

dbpf "rcsRf1:phase_injection_i_set","0.01"
dbpf "rcsRf2:phase_injection_i_set","0.01"
dbpf "rcsRf3:phase_injection_i_set","0.01"
dbpf "rcsRf4:phase_injection_i_set","0.01"
dbpf "rcsRf5:phase_injection_i_set","0.01"
dbpf "rcsRf6:phase_injection_i_set","0.01"
dbpf "rcsRf7:phase_injection_i_set","0.01"
dbpf "rcsRf8:phase_injection_i_set","0.01"

dbpf "rcsRf1:amp_i_injection_set","0.01"
dbpf "rcsRf2:amp_i_injection_set","0.01"
dbpf "rcsRf3:amp_i_injection_set","0.01"
dbpf "rcsRf4:amp_i_injection_set","0.01"
dbpf "rcsRf5:amp_i_injection_set","0.01"
dbpf "rcsRf6:amp_i_injection_set","0.01"
dbpf "rcsRf7:amp_i_injection_set","0.01"
dbpf "rcsRf8:amp_i_injection_set","0.01"
