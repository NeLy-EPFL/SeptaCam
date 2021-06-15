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
OPTIONS        =  -w -Wall -Wno-long-long -pedantic 
#-Wl,-rpath-link=../../lib
       
INC_GUI        = -Igui -I/usr/include/tcl8.6 -I/usr/X11 -I/usr/include/boost
#INC_CV         = -I/opencv2/opencv/include
INC_CV         = -I/usr/local/include/opencv4
INC_BASLER    := $(shell $(PYLON_ROOT)/bin/pylon-config --cflags)

LIBS           = -lpthread -lGLU -lGL -lpthread 
LIBS_GUI       = -ltcl8.6 -ltk8.6
LIBS_CV        = -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs #-lopencv_world 

#-lopencv_ml -lopencv_objdetect -lopencv_viz -lopencv_dnn          \
#			     -lopencv_videostab -lopencv_photo -lopencv_superres -lopencv_shape\
#			     -lopencv_video -lopencv_stitching -lopencv_calib3d                \
#			     -lopencv_features2d -lopencv_highgui -lopencv_videoio             \
#			     -lopencv_imgcodecs -lopencv_imgproc -lopencv_flann -lopencv_core 
LIBS_BASLER   := $(shell $(PYLON_ROOT)/bin/pylon-config --libs) -ljsoncpp

#LIBDIRS_CV     = -L/opencv2/opencv/lib/
LIBDIRS_CV     = -L/usr/local/lib/
LIBDIRS_GUI    = -L/usr/lib/x86_64-linux-gnu/
LIBDIRS_BASLER:= $(shell $(PYLON_ROOT)/bin/pylon-config --libs-rpath)

basler: GUI_BASLER_m

GUI_BASLER_m: ${ODIR}gui.o ${ODIR}cpptk.o ${ODIR}cpptkbase.o ${ODIR}analyzeImgs.o C++_Source/NIR_Imaging.h 
	#mkdir .obj
	rm -f ${ODIR}NIR_Imaging_BASLER.o 
	${CXX} ${INC_BASLER} ${INC_CV} -D MULTITHREAD=1 -c C++_Source/NIR_Imaging_BASLER.cpp \
			-o ${ODIR}NIR_Imaging_BASLER.o -std=c++11
	${CXX} ${INC_CV} ${ODIR}NIR_Imaging_BASLER.o $? -o $@ \
			${LIBDIRS_GUI} ${LIBDIRS_BASLER} ${LIBDIRS_CV} ${LIBS_GUI} \
			${LIBS_BASLER} ${LIBS} ${LIBS_CV} ${OPTIONS}
#`pkg-config opencv --cflags --libs` -std=c++11 

${ODIR}gui.o: C++_Source/gui/gui.cc 
	${CXX} ${INC_GUI} ${INC_CV} -c  $? -o $@ ${OPTIONS} -std=c++11

${ODIR}cpptk.o: C++_Source/gui/cpptk.cc  
	${CXX} ${INC_GUI} -c  $? -o $@ ${OPTIONS}

${ODIR}cpptkbase.o: C++_Source/gui/base/cpptkbase.cc
	${CXX} ${INC_GUI} -c $? -o $@ ${OPTIONS}

${ODIR}analyzeImgs.o: C++_Source/analyzeImgs.cpp
	${CXX} ${INC_CV} -c $? -o $@ ${OPTIONS}

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

