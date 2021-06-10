#include <string>
#include <opencv2/opencv.hpp>

class CustomImageEventHandler;
struct recstat_t {
    string start_time;
    CustomImageEventHandler *grabHandlers;
    FILE **arhFile;
    bool direct_save;
    string target_path;
};

int cameras_connect();
void cameras_disconnect();
std::array<long int, 4> cameras_getROI(int index);
std::string cameras_getGUID(int index);

void cameras_setROI(unsigned int width, unsigned int height,
                    unsigned int offsetX, unsigned int offsetY, int index = -1);

void cameras_setFPS(float fps, bool for_trigger);

void cameras_start(int numFrames = 0);
void cameras_stop();

// void cameras_grab(std::string path, int obs_frames, int rec_frames, bool motion_act,
//                   bool directSave, bool trigger, bool strobe, int liveStream[],
//                   int enCams[], int streamCam);
void cameras_grab_for_preview(int liveStream[], int enCam[], int streamCam);
recstat_t start_cameras_grab(std::string path, int obs_frames, int rec_frames, bool motion_act,
                             bool direct_save, bool trigger, bool strobe, int liveStream[],
                             int enCam[], int streamCam);
void clean_up_camera_grab(recstat_t status);
void wait_for_grab_to_finish(float expected_time_sec);

void cameras_save(std::string path);
