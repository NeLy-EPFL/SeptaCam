#include <string>
#include <opencv2/opencv.hpp>

int cameras_connect();
void cameras_disconnect();
std::array<long int, 4> cameras_getROI(int index);
std::string cameras_getGUID(int index);

void cameras_setROI(unsigned int width, unsigned int height,
                    unsigned int offsetX, unsigned int offsetY, int index = -1);

void cameras_setFPS(float fps, bool for_trigger);

void cameras_start(int numFrames = 0);
void cameras_stop();

void cameras_grab(std::string path, int obs_frames, int rec_frames, bool motion_act,
                  bool directSave, bool trigger, bool strobe, int liveStream[],
                  int enCams[], int streamCam);
void cameras_save(std::string path);
