#! /bin/make
##############################################
# Makefile for the 'ContinuousCaptureMultiPart' sample
##############################################
#-------------------------------------------
ifndef MVIMPACT_ACQUIRE_SOURCE_DIR
    ifndef MVIMPACT_ACQUIRE_DIR
        ROOT_PATH=../..
    else
        ROOT_PATH=$(MVIMPACT_ACQUIRE_DIR)/apps
    endif
else
    ROOT_PATH=$(MVIMPACT_ACQUIRE_SOURCE_DIR)/apps
endif

#-------------------------------------------
MODULE_NAME=ContinuousCaptureMultiPart

#-------------------------------------------
OOBJ =										\

#-------------------------------------------
vpath %.cpp ..

#-------------------------------------------
USR_INCLS =					\

#-------------------------------------------
USR_LIBS =					\

#		-lefence

#-------------------------------------------
CPPFLAGS_1 = 				\
	-pedantic -Wno-long-long

#-------------------------------------------
include $(ROOT_PATH)/Makefile.samp.inc
