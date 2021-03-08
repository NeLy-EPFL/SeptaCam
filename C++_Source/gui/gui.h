#ifndef FG_GUI_H
#define FG_GUI_H

#include <stdio.h>
#include <string> 
#include <iostream> 
#include <fstream>
#include <string>
#include <pthread.h>

#include <jsoncpp/json/json.h>

#include "cpptk.h"
#include "../constants.h"
#include "../frame_grabber.h"

using namespace Tk;

class gui
{

	private: 

		frame_grabber* f_grab; 

		/** Variable for GUI tracking whether or not 
		 *   capture is currently occurring 
		 */
		volatile bool capture_running;
		
		/** Variable for GUI tracking whether or not 
		 *   postprocessing is currently occurring 
		 */
		volatile bool postprocess_running;
		
		/**
		 * Variable for GUI tracking whether or not
		 *  external hardware triggering is required for next capture
		 */
		int trigger;
		
		/** 
		 * Global Static variable for GUI tracking whether or not 
		 *  having an external strobe signal is required for next capture 
		 */
		int strobe;
		
		/** Global Static variable for GUI ftracking whether or not we 
		 *   should save images individually or in a single file per camera 
		 *   for next capture. Writing to a single file per camera has 
		 *   performance benefits. 
		 */
		int seperate_images; 
		
		/** Global Static variable for GUI tracking whether or not 
		 *    capture should be activated by motion tracking.
		 *  
		 * @see 
		 */
		int motion_activation; 
		int obs_scale;
		int rec_scale;

		void  toggle_motion_activation();
		void  set_all_ROI();
		void  set_single_ROI(); 
		void  get_single_ROI();
		void  set_default_ROI();
		void* start_capture(void* arg);
		void  start_capture_t();
		void* process(void* arg);
		void  process_t();
		float convert_time(float t, int param);
		void  update_frame_view();

	public: 
		gui(); 
		~gui();
		static void alert_error(std::string msg);

};

#endif 

