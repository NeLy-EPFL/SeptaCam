#PROJECT_ROOT = ~/Desktop/NIR_Imaging
#OUTDIR    = ${PROJECT_ROOT}

#OPT_INC = ${PROJECT_ROOT}/common/make/common.mk
#-include ${OPT_INC}

# Handle environment variables
ifeq ($(wildcard ${OPT_INC}),)
	CXX   = g++
	ODIR  = .obj/
	SDIR  = .
	MKDIR = mkdir -p
	MV    = mv
endif

UNAME          = $(shell uname)
PYLON_ROOT    ?= /opt/pylon5
BASE_NAME      = NIR_Imaging
OPTIONS        =  -w -Wall -Wno-long-long -pedantic 
       
INC_GUI        = -Igui -I/usr/include/tcl8.6 -I/usr/X11 -I/usr/include/boost
INC_CV         = -I/opencv2/opencv/include
INC_BASLER    := $(shell $(PYLON_ROOT)/bin/pylon-config --cflags)

LIBS           = -lpthread -lGLU -lGL -lpthread 
LIBS_GUI       = -ltcl8.6 -ltk8.6
LIBS_CV        = -lopencv_world

LIBS_BASLER   := $(shell $(PYLON_ROOT)/bin/pylon-config --libs) -ljsoncpp

LIBDIRS_CV     = -L/opencv2/opencv/lib/
LIBDIRS_GUI    = -L/usr/lib/x86_64-linux-gnu/
LIBDIRS_BASLER:= $(shell $(PYLON_ROOT)/bin/pylon-config --libs-rpath)

all: frame_grabber

frame_grabber: ${ODIR}gui.o ${ODIR}cpptk.o ${ODIR}cpptkbase.o ${ODIR}analyzeImgs.o C++_Source/frame_grabber.h 
	#mkdir .obj
	rm -f ${ODIR}frame_grabber.o 
	${CXX} ${INC_BASLER} ${INC_CV} -c C++_Source/frame_grabber.cpp \
			-o ${ODIR}frame_grabber.o -std=c++11
	${CXX} ${ODIR}frame_grabber.o $? -o $@ \
			${LIBDIRS_GUI} ${LIBDIRS_BASLER} ${LIBDIRS_CV} ${LIBS_GUI} \
			${LIBS_BASLER} ${LIBS} ${LIBS_CV} ${OPTIONS} -std=c++11

${ODIR}gui.o: C++_Source/gui/gui.cpp C++_Source/gui/gui.h
	${CXX} ${INC_GUI} ${INC_BASLER} -c  $? -o $@ ${OPTIONS} -std=c++11

${ODIR}cpptk.o: C++_Source/gui/cpptk.cc  
	${CXX} ${INC_GUI} -c  $? -o $@ ${OPTIONS}

${ODIR}cpptkbase.o: C++_Source/gui/base/cpptkbase.cc
	${CXX} ${INC_GUI} -c $? -o $@ ${OPTIONS}

${ODIR}analyzeImgs.o: C++_Source/analyzeImgs.cpp
	${CXX} -c $? -o $@ ${OPTIONS}

# Cleanup intermediate objects
.PHONY: clean_obj
clean_obj:
	rm -f ${ODIR}*.o
	@echo "obj cleaned up!"

# Cleanup everything
.PHONY: clean
clean: clean_obj
	rm -f ${OUTDIR}/NIR_Imaging_BASLER
	rm -f ${OUTDIR}/NIR_Imaging_FLIR
	@echo "all cleaned up!"

