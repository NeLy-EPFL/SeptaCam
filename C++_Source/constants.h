#define TRANSFER_PRIORITY  (80)   			// Real Time Scheduling thread priority for grabbing images over USB
#define GRAB_LOOP_PRIORITY (79)    			// Real Time Scheduling thread priority for saving data to disk
#define OTF_PROC_PRIORITY  (78)

#define DISPLAY_FPS        (30)  			// Frame Rate (in Hertz ) at which previews are refreshed 
#define WINDOW_HEIGHT      (400)   			// The height (in Pixels) of the Windows displaying frame preview
#define MAX_NUM_DEVICES    (10)    			// The maximum number of cameras that can be connected
#define MAX_IMAGE_SIZE	   (2048 * 2048)	// The maximum number of bytes that a single frame can take 

#define OTF_QUEUE_SIZE 	   1024

#define INSTALL_DIR     "/NIR_Imaging/"     // The installation directory in the runtime environment 
#define OUTPUT_DIR	    "/data/" 		    // The output directory in the runtime environment
#define WINDOWS_SHARE   NULL           // If the network connection is not fast enough for the camera set up and the two photon images to save in the same location this folder is user to create the same folder structure to save the two photon output in the same location. Ignored when NULL.


#define SEQ_FILE        "/NIR_Imaging/Code_Python/stimulation/seqFile.txt"      // The directory for the stimulation sequence file

#define VID_BATCH_FILE  "/NIR_Imaging/Code_Python/getVideos/videosBatch.txt"      // The directory for the videos batch file

#define MAKE_VIDEOS_FILE        "/NIR_Imaging/Code_Python/getVideos/makeVideos.py"      // The directory for the python program used to make videos

