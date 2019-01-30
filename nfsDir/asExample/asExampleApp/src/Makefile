TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# xxxRecord.h will be created from xxxRecord.dbd
DBDINC += xxxRecord

# Install xxxSupport.dbd into <top>/dbd
DBD += xxxSupport.dbd
DBD += asInclude.dbd

# Build an IOC support library
LIBRARY_IOC += asExampleSupport

# Compile and add the code to the support library
asExampleSupport_SRCS += xxxRecord.c
asExampleSupport_SRCS += devXxxSoft.c

# Link locally-provided code into the support library,
# rather than directly into the IOC application.
# This is required for Windows DLL builds.
asExampleSupport_SRCS += dbSubExample.c
asExampleSupport_SRCS += asExampleHello.c
asExampleSupport_SRCS += initTrace.c

asExampleSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

# Build the IOC application
PROD_IOC = asExample

# asExample.dbd will be created and installed
DBD += asExample.dbd

# asExample.dbd will include these files:
asExample_DBD += base.dbd
asExample_DBD += xxxSupport.dbd
asExample_DBD += dbSubExample.dbd
asExample_DBD += asExampleHello.dbd
asExample_DBD += initTrace.dbd
asExample_DBD += asInclude.dbd

# asExample_registerRecordDeviceDriver.cpp derives from asExample.dbd
asExample_SRCS += asExample_registerRecordDeviceDriver.cpp

# Build the main IOC entry point where needed
asExample_SRCS_DEFAULT += asExampleMain.cpp
asExample_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#asExample_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in the code from our support library
asExample_LIBS += asExampleSupport
asExample_LIBS += autosave

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
ifneq ($(SNCSEQ),)
    # Build sncExample into asExampleSupport
    sncExample_SNCFLAGS += +r
    asExample_DBD += sncExample.dbd
    asExampleSupport_SRCS += sncExample.stt
    asExampleSupport_LIBS += seq pv
    asExample_LIBS += seq pv

    # Build sncProgram as a standalone program
    PROD_HOST += sncProgram
    sncProgram_SNCFLAGS += +m
    sncProgram_SRCS += sncProgram.st
    sncProgram_LIBS += seq pv
    sncProgram_LIBS += $(EPICS_BASE_HOST_LIBS)
endif

# Finally link IOC to the EPICS Base libraries
asExample_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

