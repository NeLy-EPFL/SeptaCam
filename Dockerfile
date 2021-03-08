FROM ubuntu:16.04
MAINTAINER Name <raphael.laporte@epfl.ch>

# Install OpenCV Dependancies

RUN apt-get update && apt-get install -y    			\
	build-essential=12.1ubuntu2          				\
	cmake=3.5.1-1ubuntu3 								\
	git=1:2.7.4-0ubuntu1.3   							\
	libgtk2.0-dev=2.24.30-1ubuntu1.16.04.2				\
	pkg-config=0.29.1-0ubuntu1							\
	libavcodec-dev=7:2.8.11-0ubuntu0.16.04.1			\
	libavformat-dev=7:2.8.11-0ubuntu0.16.04.1			\
	libswscale-dev=7:2.8.11-0ubuntu0.16.04.1			\
	python-dev=2.7.12-1~16.04							\
	python-numpy=1:1.11.0-1ubuntu1						\
	libtbb2=4.4~20151115-0ubuntu3						\
	libtbb-dev=4.4~20151115-0ubuntu3					\
	libjpeg-dev=8c-2ubuntu8								\
	libpng12-dev=1.2.54-1ubuntu1						\
	libtiff5-dev=4.0.6-1ubuntu0.4						\
	libjasper-dev=1.900.1-debian1-2.4ubuntu1.1			\
	libdc1394-22-dev=2.2.4-1								
 
# Download OpenCV Source

RUN mkdir /opencv2 
WORKDIR /opencv2  
RUN git clone https://github.com/opencv/opencv.git 
WORKDIR /opencv2/opencv
RUN	git reset --hard 167034fb045a2f0ba4789d0771e6515bb09fd55c && \
 	mkdir /opencv2/release 

# Build and install OpenCV

WORKDIR /opencv2/release
RUN cmake /opencv2/opencv -G "Unix Makefiles" -DBUILD_SHARED_LIBS=OFF -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX=/opencv2/opencv -DWITH_TBB=ON -DBUILD_NEW_PYTHON_SUPPORT=ON -DWITH_V4L=ON -DINSTALL_C_EXAMPLES=ON -DINSTALL_PYTHON_EXAMPLES=ON -DBUILD_EXAMPLES=ON -DWITH_QT=OFF -DWITH_OPENGL=ON -DBUILD_FAT_JAVA_LIB=ON -DINSTALL_TO_MANGLED_PATHS=ON -DINSTALL_CREATE_DISTRIB=ON -DINSTALL_TESTS=ON -DENABLE_FAST_MATH=ON \
 && make all -j$(nproc) \
 && make install 

# Install GUI Dependencies 

RUN apt-get update && apt-get install -y 					\
	libboost-dev=1.58.0.1ubuntu1       						\
	tcl8.6-dev=8.6.5+dfsg-2          						\
	tk8.6-dev=8.6.5-1

# Install Video Codecs

RUN apt-get update && apt-get install -y  				\
	libraw1394-11=2.1.1-2 								\
	libavcodec-ffmpeg56=7:2.8.11-0ubuntu0.16.04.1		\
	libavformat-ffmpeg56=7:2.8.11-0ubuntu0.16.04.1		\
	libswscale-ffmpeg3=7:2.8.11-0ubuntu0.16.04.1		\
	libswresample-ffmpeg1=7:2.8.11-0ubuntu0.16.04.1		\
	libavutil-ffmpeg54=7:2.8.11-0ubuntu0.16.04.1		\
	libgtkmm-2.4-dev=1:2.24.4-2							\
	libglademm-2.4-dev=2.6.7-5							\
	libgtkglextmm-x11-1.2-dev=1.2.0-7					\
	libusb-1.0-0=2:1.0.20-1


#Install Misc. NIR_Imaging Dependancies

RUN apt-get update && apt-get install -y libjsoncpp-dev=1.7.2-1 firefox 

#Configure System Files

RUN touch /etc/ld.so.conf.d/libs.conf && \
	echo "/usr/local/lib\n/opt/pylon5/lib64\n" >>  /etc/ld.so.conf.d/libs.conf && \
	ldconfig


####################################################################
#
# Docker will cache each step of the build process.
# When re-building the docker image because of a necessary recompile
#	Docker should only re-execute instructions following this tag. 
#	This is of course supposing that no new instructions are added 
#	above this tag
# 
# When maintaining this file, it might be a good idea to set all 
#	commands which dont depend on source files before this tag, and 
*   all commands which do depend on them after. ( In order to keep 
#   the recompilation process as short and stable as possible. )
#
####################################################################


#Install Pylon

RUN mkdir /NIR_Imaging && mkdir /NIR_Imaging/.obj
COPY . /NIR_Imaging
RUN  chmod +x /NIR_Imaging/Dependencies/install_pylon.sh
RUN /NIR_Imaging/Dependencies/install_pylon.sh


# Setup I/O Permissions ( must occur after installing Pylon )

RUN export uid=1000 gid=1000 && \
    mkdir -p /home/developer && \
    echo "developer:x:${uid}:${gid}:Developer,,,:/home/developer:/bin/bash" >> /etc/passwd && \
    echo "developer:x:${uid}:" >> /etc/group && \
    echo "developer ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/developer && \
    chmod 0440 /etc/sudoers.d/developer && \
    chown ${uid}:${gid} -R /home/developer


# Compile NIR_Imaging

WORKDIR /NIR_Imaging
RUN make basler


# Setup Directory Permissions

RUN chown developer /NIR_Imaging
RUN mkdir /home/developer/data
RUN chown developer /home
RUN chown developer /home/developer
RUN chown developer /home/developer/data
RUN chown developer /NIR_Imaging/Config/*.pfs

USER developer


CMD /usr/bin/firefox


