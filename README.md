# SeptaCam
Code for controlling the seven-camera system

# Installation
- Create environment
```
$conda create -n septacam python=3.6 numpy
$conda activate septacam
```

- Install [pylon 5.0.12](https://www.baslerweb.com/en/sales-support/downloads/software-downloads/pylon-5-0-12-linux-x86-64-bit/)

- Install opencv
```
$sudo apt install build-essential cmake git pkg-config libgtk-3-dev \
    libavcodec-dev libavformat-dev libswscale-dev libv4l-dev \
    libxvidcore-dev libx264-dev libjpeg-dev libpng-dev libtiff-dev \
    gfortran openexr libatlas-base-dev python3-dev python3-numpy \
    libtbb2 libtbb-dev libdc1394-22-dev libopenexr-dev \
    libgstreamer-plugins-base1.0-dev libgstreamer1.0-dev
$git clone https://github.com/opencv/opencv.git
$git clone https://github.com/opencv/opencv_contrib.git
$git checkout 4.0.0 (both repos)
$cd ~/opencv
$mkdir build
$cd build
$cmake -D CMAKE_BUILD_TYPE=RELEASE \
       -D CMAKE_INSTALL_PREFIX=/usr/local \
       -D INSTALL_C_EXAMPLES=ON \
       -D INSTALL_PYTHON_EXAMPLES=ON \
       -D OPENCV_GENERATE_PKGCONFIG=ON \
       -D OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
       -D BUILD_EXAMPLES=ON ..
$make -j8
$sudo make install
```

- Install jsoncpp
```$sudo apt-get install libjsoncpp-dev```

- Install boost
```$sudo apt-get install libboost-all-dev```

- Install tcl
```$sudo apt-get install tcl-dev```

- Install tk
```$sudo apt-get install tk8.6-dev```

- Create .obj directory in SeptaCam folder
```$mkdir .obj```

- Compile software
```$make GUI_BASLER_m```

- Run GUI
```$./GUI_BASLER_m```
