#ifndef FG_FRAME_GRABBER_H 
#define FG_FRAME_GRABBER_H 


// Pylon Imports
#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbInstantCamera.h>
#include <pylon/usb/BaslerUsbInstantCameraArray.h>

// C Imports
#include <sys/stat.h>
#include <jsoncpp/json/json.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sched.h>
#include <pthread.h>

// C++ Imports
#include <opencv2/opencv.hpp>
#include <mutex>
#include <iostream>
#include <ctime>
#include <fstream>
#include <chrono>
#include <ctime>
#include <random>
#include <string>

// Local Imports 
#include "constants.h"
#include "helpers.h"
#include "analyzeImgs.h"


using namespace std;
using namespace Pylon;
using namespace Basler_UsbCameraParams;
using namespace GenApi;



class frame_grabber
{

	private:

		CBaslerUsbInstantCameraArray cameras;  

		int height[MAX_NUM_DEVICES];
		int width[MAX_NUM_DEVICES];
		int offX[MAX_NUM_DEVICES];
		int offY[MAX_NUM_DEVICES];

		pthread_t OTFThread[MAX_NUM_DEVICES];
		pthread_t GRABThread[MAX_NUM_DEVICES];

		volatile float fps;
		bool    	   performanceMode = false; 
		void**  	   display_data = NULL;

		analyzeImgs image_analyser;
		int 		capture_flag;
		int 		annotation_color; 

		void set_performance_mode(bool onOff);
		void set_metadata(bool onOff);
		void set_synchronization(bool trigger, bool strobe);

	public:

		frame_grabber();
		~frame_grabber();

		const int num_devices;

		void   		cameras_start(int numFrames = 0 ); 
		void   		cameras_stop(); 
	
		void   		cameras_grab(std::string path, int obs_frames, int rec_frames, bool motion_act, bool directSave, bool trigger,bool strobe); 
		void   		cameras_save(std::string path); 

		string 		cameras_getGUID(int index);
		array<long int, 4> cameras_getROI(int index); 
		void   		cameras_setROI(unsigned int width, unsigned int height,
				           unsigned int offsetX, unsigned int offsetY, int index = -1); 
	
		void        cameras_setFPS(float fps, bool for_trigger);
};

#endif 