# Copyright 2019 Xilinx, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

##############################
# environment settings 
ifndef XILINX_XRT
    $(error XILINX_XRT not defined)
endif

ifndef XILINX_VITIS
    $(error Environment variable XILINX_VITIS is required)
endif

SHELL = /bin/bash
CLCC = $(XILINX_VITIS)/bin/v++

##############################
BASE_DIR=$(shell pwd)
SRC_DIR=${BASE_DIR}/src
LIB_INCLUDE=${BASE_DIR}/include
INCLUDE_DIR=${BASE_DIR}/include

##############################
# default settings 
RTM_part               	= u280
RTM_kernelHlsFreq      	= 300
RTM_kernelVivadoFreq   	= 300
RTM_dataType      		= float
RTM_numFSMs       		= 2
RTM_x  					= 60
RTM_y  					= 70
RTM_z  					= 80
RTM_maxZ  				= 256
RTM_maxY  				= 256
RTM_MaxB 				= 20
RTM_NXB 				= $(RTM_MaxB)
RTM_NYB 				= $(RTM_MaxB)
RTM_NZB 				= $(RTM_MaxB)
RTM_order  				= 8
RTM_time 				= 12
RTM_parEntries 			= 16
RTM_nPE					= 4
RTM_verify				= 1
##############################

KERNEL_SRCS = ${SRC_DIR}/rtmforward_upb.cpp
KERNEL_NAME = rtmforward
ifdef KSRC
	KERNEL_SRCS=${KSRC}
endif
ifdef KNAME
	KERNEL_NAME=${KNAME}
endif

COMMON_CFLAGS_K = -D RTM_dataType=$(RTM_dataType) \
          -D RTM_numFSMs=$(RTM_numFSMs) \
          -D RTM_maxY=$(RTM_maxY) \
          -D RTM_maxZ=$(RTM_maxZ) \
          -D RTM_order=$(RTM_order) \
          -D RTM_MaxB=$(RTM_MaxB) \
          -D RTM_part=$(RTM_part) \
          -D RTM_nPE=$(RTM_nPE) \
          -D RTM_parEntries=$(RTM_parEntries)

CFLAGS_K= -I${LIB_INCLUDE} \
			  $(COMMON_CFLAGS_K) \
			  -Wno-ignored-attributes

CLCC_OPT += --xp prop:solution.hls_pre_tcl=${BASE_DIR}/hls_config.tcl
#CLCC_OPT += --advanced.param=compiler.useVitisHLS=1 

ifeq (${RTM_part},u280)
	DSA_PLATFORM=xilinx_u280_xdma_201920_3
	PLATFORM_REPO_PATH=/opt/xilinx/platforms/${DSA_PLATFORM}
	XOPENCL_LIB_PATH=${XILINX_XRT}/lib
else
	$(error Unknown RTM_part ${RTM_part})
endif

CL_LIB_PATH = ${XILINX_XRT}/include

ifndef XILINX_XRT
  XOPENCL_LIB_PATH=${XILINX_SDX}/runtime/lib/x86_64
  CL_LIB_PATH=${XILINX_SDX}/runtime/include/1_2
endif

############################################################

#SDA_FLOW could be hw, hw_emu, sw_emu
ifndef SDA_FLOW
  SDA_FLOW = sw_emu
endif

##############################
# XOCC options for both compile and link
CLCC_OPT += -t ${SDA_FLOW}
CLCC_OPT += --xp param:compiler.worstNegativeSlack=-0.1 
ifeq (${SDA_FLOW},hw_emu)
    ifeq ($(HWEMUGUI),1)
        CLCC_OPT += --xp param:hw_em.debugLevel=GUI
    else
        CLCC_OPT += --xp param:compiler.preserveHlsOutput=1 --report system
    endif
endif

KEEP_TEMP=1
KERNEL_DEBUG=1
ifeq (${KEEP_TEMP},1)
    CLCC_OPT += -s
endif
ifeq (${KERNEL_DEBUG},1)
    CLCC_OPT += -g
endif
CLCC_OPT += $(CLCC_OPT_LEVEL) -f ${PLATFORM_REPO_PATH}/$(DSA_PLATFORM).xpfm

##############################
# XOCC compile options
KERNEL_DEFS += $(CFLAGS_K)
CLCC_COMP_OPT =  ${CLCC_OPT} ${KERNEL_DEFS}
CLCC_COMP_OPT += --kernel_frequency ${RTM_kernelHlsFreq}

##############################
# XOCC link options
XP_VIVADO_PARAMS =--xp vivado_param:project.writeIntermediateCheckpoints=1
XP_VIVADO_PARAMS+=--xp vivado_prop:run.impl_1.strategy=Performance_Explore 
XP_VIVADO_PARAMS+=--xp vivado_param:place.hardVerbose=469538
XP_VIVADO_PARAMS+=--xp vivado_param:place.oldMsgVerbose=1
XP_VIVADO_PARAMS+=--xp vivado_param:route.flowDbg=1
XP_VIVADO_PARAMS+=--xp vivado_param:route.timingDbg=1
XP_VIVADO_PARAMS+=--xp param:compiler.fanoutLimit=0

ifeq (${SDA_FLOW},hw)
    CLCC_LINK_OPT += $(XP_VIVADO_PARAMS)
endif
CLCC_LINK_OPT += --kernel_frequency ${RTM_kernelVivadoFreq}
CLCC_LINK_OPT += --config ${BASE_DIR}/conn_u280.ini

###############################################################################
# For Makefile target only 
CONFIG_INFO = $(shell echo ${CFLAGS_K} | sed 's/.*TEST_SDX=1//; s/-D //g; s/ -Wno.*//')
XCLBIN_FREQ=${BASE_DIR}/xclbin_get_freq.pl

BUILD_DIR = ${BASE_DIR}/build
BUILD_CACHE_DIR = ${BASE_DIR}/build/tmp
OUT_DIR = ${BUILD_DIR}/out_${SDA_FLOW}
OUT_HOST_DIR=${OUT_DIR}/xconfig
KERNEL_XOS = ${OUT_DIR}/k_dir/${KERNEL_NAME}.xo
XCLBIN = ${OUT_DIR}/${KERNEL_NAME}_${RTM_x}x${RTM_y}x${RTM_z}_b${RTM_MaxB}.xclbin
MAKE_EXIT_OK_HW_FILE = ${BUILD_DIR}/out_hw/${KERNEL_NAME}.xclbin
MAKE_LOG=${BUILD_DIR}/log-run_${SDA_FLOW}.txt
###############################################################################
.PHONY: all
all: help

# emulation config settings
xconfig : ${OUT_HOST_DIR}/emconfig.json | ${OUT_HOST_DIR}

${OUT_HOST_DIR}/emconfig.json :
	cd ${BUILD_DIR}; $(XILINX_VITIS)/bin/emconfigutil -f ${PLATFORM_REPO_PATH}/$(DSA_PLATFORM).xpfm --od ${OUT_HOST_DIR}

# generates kernel hw
xbin: ${XCLBIN}
	
${XCLBIN}: ${KERNEL_XOS}
	@echo "************* Compile XCLBIN ${XCLBIN}  from  ${KERNEL_XOS} *************"
	cd ${BUILD_CACHE_DIR}; ${CLCC} -l ${CLCC_OPT} ${CLCC_LINK_OPT} $^ -o ${XCLBIN}

${KERNEL_XOS}: ${OUT_DIR}/k_dir
	@echo "***** Compiling XO $@ *****"
	cd ${BUILD_CACHE_DIR}; ${CLCC} --temp_dir ${OUT_DIR}/k_dir -c ${CLCC_COMP_OPT} --xp prop:kernel.${KERNEL_NAME}.kernel_flags=-std=c++0x -k ${KERNEL_NAME} -o $@ ${KERNEL_SRCS}

sw_emu:
	+make SDA_FLOW=sw_emu emu_int  2>&1 | tee ${MAKE_LOG}

hw_emu:
	+make SDA_FLOW=hw_emu emu_int  2>&1 | tee ${MAKE_LOG}

emu_int: ${BUILD_DIR} xconfig xbin
	@echo INFO: kernel xclbin frequency is $(shell ${XCLBIN_FREQ} ${XCLBIN}) MHz
	XCL_EMULATION_MODE=${SDA_FLOW} XILINX_OPENCL=${XILINX_VITIS}

hw:
	+make SDA_FLOW=hw hw_int  2>&1 | tee ${MAKE_LOG}; test -f ${MAKE_EXIT_OK_HW_FILE}
	+make dump_config

hw_int : xbin
	@echo INFO: kernel xclbin frequency is $(shell ${XCLBIN_FREQ} ${XCLBIN}) MHz

clean : cleancache
	+make SDA_FLOW=sw_emu clean_int
	+make SDA_FLOW=hw_emu clean_int
	+make SDA_FLOW=hw clean_int
	${RM} -rf ${OUT_HOST_DIR} _xocc_link_${KERNEL_NAME}_${KERNEL_NAME}.dir sdaccel_profile* .Xil _sds iprepo bd.* *.bit *.ltx *.dat *.hpfm *.xml _new_clk_freq dr.bd.tcl routed.dcp xocc_${KERNEL_NAME}* v++_${KERNEL_NAME}* _x timeline_trace.csv xrc.log xilinx_u280_xdma_201920_1-0-${KERNEL_NAME}_simulate.log
clean_int:
	${RM} -rf ${OUT_DIR} ${MAKE_LOG}

cleanlog:
	cd ${BUILD_CACHE_DIR}; ${RM} -rf *.log

cleancache:
	${RM} -rf ${BUILD_CACHE_DIR};

dump_config: ${OUT_DIR}
	@echo ${CONFIG_INFO} | tr " " "\n" > ${OUT_DIR}/config_info.dat
	cp ${OUT_DIR}/k_dir/${KERNEL_NAME}/${KERNEL_NAME}/${KERNEL_NAME}/ip/drivers/${KERNEL_NAME}_${KERNEL_NAME}_v1_0/src/*_hw.h ${OUT_DIR}
	
help:
	@echo "************* Help *************"
	@echo "Usage:"
	@echo "make KSRC=<path_to_kernel_src> KNAME=<kernel_name> sw_emu|hw_emu|hw"
	@echo "********************************"


${BUILD_DIR} : ${BUILD_CACHE_DIR}
	@echo "************* Creating DIR $@ *************"
	mkdir -p $@

${BUILD_CACHE_DIR} :
	@echo "************* Creating DIR $@ *************"
	mkdir -p $@

${OUT_DIR} :
	@echo "************* Creating DIR $@ *************"
	mkdir -p $@

${OUT_HOST_DIR} :
	@echo "************* Creating DIR $@ *************"
	mkdir -p $@
	
${OUT_DIR}/k_dir : | ${OUT_DIR}
	@echo "************* Creating DIR $@ *************"
	mkdir -p $@
