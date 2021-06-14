// Pylon Imports
#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbInstantCamera.h>
#include <pylon/usb/BaslerUsbInstantCameraArray.h>

/* ----- TEST ---- */

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

// Local Imports
#include "NIR_Imaging.h"
#include "constants.h"
#include "helpers.h"
#include "analyzeImgs.h"
#include "common.h"

using namespace std;
using namespace Pylon;
using namespace Basler_UsbCameraParams;
using namespace GenApi;

static CBaslerUsbInstantCameraArray cameras;

// State Variables for Camera Operation

static int num_devices;
static int height[MAX_NUM_DEVICES];
static int width[MAX_NUM_DEVICES];
static int offX[MAX_NUM_DEVICES];
static int offY[MAX_NUM_DEVICES];

static pthread_t OTFThread[MAX_NUM_DEVICES];
static pthread_t GRABThread[MAX_NUM_DEVICES];

static volatile float fps;
static bool performanceMode = false;
static void **display_data = NULL;

analyzeImgs aI;
static int capture_flag;
static int annotation_color;
static int saveDataFromCam[7];
static int liveWindows[7] = {true, true, true, true, true, true, true};
static int captStreamCam;

class CustomImageEventHandler : public CImageEventHandler
{
private:
    int camera_number;
    int max_rec_frames;
    int img_height;
    int img_width;
    bool direct_save;
    bool motion_act;
    FILE *data_file;
    string path;

    int *frame_number_log = NULL;
    int *timestamp_log = NULL;
    void *buffer = NULL;
    float time_acc = 0.0;
    int missed_frame_acc = 0;

    unsigned int last_tick;
    int last_frame_count;
    int last_index;

public:
    int recorded_frames = 0;

    CustomImageEventHandler() {}

    CustomImageEventHandler(int id, int rec_frames, int curr_height, int curr_width,
                            bool motion_activation, bool many_files = true,
                            FILE *file = NULL, std::string wd = NULL)
    {
        camera_number = id;
        max_rec_frames = rec_frames;
        img_height = curr_height;
        img_width = curr_width;
        motion_act = motion_activation;
        direct_save = many_files;
        data_file = file;
        path = wd;

        if (camera_number == 0)
        {
            capture_flag = false;
            annotation_color = 0;
        }
    }

    float getIntervalAcc()
    {
        return time_acc;
    }

    int getMissedFrames()
    {
        return missed_frame_acc;
    }

    int getFrameNumber(int i)
    {
        return frame_number_log[i];
    }

    int getTimestamp(int i)
    {
        return timestamp_log[i];
    }

    virtual void OnImageGrabbed(CInstantCamera &camera, const CGrabResultPtr &ptr_grab_result)
    {
        // Save to Disk as quickly as possible !
        void *data = ptr_grab_result->GetBuffer();
        int index = ptr_grab_result->GetImageNumber();

        if (data == NULL)
        {
            cerr << "Null pointer for grabbed data" << endl;
            return;
        }

        if (index == 1)
        {
            buffer = malloc(MAX_IMAGE_SIZE);
            frame_number_log = (int *)malloc(sizeof(int) * max_rec_frames);
            timestamp_log = (int *)malloc(sizeof(int) * max_rec_frames);
            assignPriority(GRAB_LOOP_PRIORITY);
        }

        memcpy(buffer, (void *)data, img_height * img_width);
        const cv::Mat M(img_height, img_width, CV_8U, buffer);

        if (motion_act && camera_number == 0)
        {
            const cv::Mat M(img_height, img_width, CV_8U, buffer);
            bool fly_moving = aI.isMoving(M, camera_number);
            // bool fly_moving = capture_flag; 
            // if (!fly_moving)
            // {
            //     fly_moving = 0.25 > (static_cast <float> (rand())
            //                          / static_cast <float> (RAND_MAX));
            //     cout << fly_moving << endl;
            // }

            if (fly_moving)
            {
                if (recorded_frames <= max_rec_frames)
                {
                    annotation_color = 255;
                    if (!capture_flag)
                    {
                        cout << " Motion Started " << endl;
                    }
                    capture_flag = true;
                }
                else
                {
                    annotation_color = 127;
                    if (capture_flag)
                    {
                        cout << " Motion Ended " << endl;
                    }
                    capture_flag = false;
                }
            }
            else
            {
                annotation_color = 127;
                capture_flag = false;
            }
        }

        if (!motion_act || capture_flag)
        {

            recorded_frames += 1;

            unsigned int ticks = ((CBaslerUsbGrabResultPtr)ptr_grab_result)->
                                    ChunkTimestamp.GetValue();
            int frame_count = ((CBaslerUsbGrabResultPtr)ptr_grab_result)->GetBlockID();

            if (direct_save)
            {
                stringstream sstream_imgJ;
                string imgFilenameJ;

                sstream_imgJ << path << "/camera_" << camera_number
                             << "_img_" << frame_count << ".jpg";

                sstream_imgJ >> imgFilenameJ;

                const cv::Mat M(img_height, img_width, CV_8U, data);
                cv::imwrite(imgFilenameJ.c_str(), M);

                // stringstream sstream;
                // string temp_filename;
                // sstream << path << "/camera_" << camera_number
                // << "_img_" << frame_count << ".png";
                // sstream >> temp_filename;

                // CImagePersistence::Save(ImageFileFormat_Png,temp_filename.c_str(),
                //                         (const void*)data,img_height*img_width,
                //                         PixelType_Mono8,img_width,img_height,0,
                //                         ImageOrientation_TopDown);
            }
            else
            {
                int written = fwrite(data, 1, img_height * img_width, data_file);
                if (written != img_height * img_width)
                {
                    cerr << "Failure on writing to disk buffer" << endl;
                    fclose(data_file);
                    return;
                }
            }

            // Check Metadata
            if (recorded_frames != 1)
            {
                time_acc += ((float)(ticks - last_tick)) / 1000000000;
                if (frame_count == UINT64_MAX)
                {
                    cerr << "bad count" << endl;
                }

                if (frame_count - last_frame_count != index - last_index)
                {
                    missed_frame_acc += (frame_count - last_frame_count) - (index - last_index);
                }
            }

            frame_number_log[recorded_frames - 1] = frame_count;
            timestamp_log[recorded_frames - 1] = ticks;

            last_tick = ticks;
            last_frame_count = frame_count;
            last_index = index;
        }

        // Handle OpenCV
        //if( fps <= DISPLAY_FPS || index %((int)(fps/DISPLAY_FPS)) == 0)
        if (camera_number == captStreamCam)
        {

            ostringstream int_string;
            int_string << "Camera_" << camera_number;

            if (motion_act && camera_number == 0)
            {
                cv::circle(M, cv::Point(30, 30), 15.0, annotation_color, -1, 8);
            }

            cv::imshow(int_string.str(), M);
            cv::resizeWindow(int_string.str(),
                             (img_width * WINDOW_HEIGHT) / img_height, WINDOW_HEIGHT);
            cv::waitKey(1);
        }
    }
};

void setPerformanceMode(bool onOff)
{
    try
    {
        performanceMode = onOff;
        cout << "Performance Mode :" << onOff << endl;
    }
    catch (const GenericException &e)
    {
        cerr << "Exeception on setPerformanceMode(). Reason: "
             << e.GetDescription() << e.what() << endl;
    }
}

void setMetadata(bool onOff)
{
    try
    {
        for (int i = 0; i < num_devices; i++)
        {
            INodeMap &nodemap = cameras[i].GetNodeMap();
            CBooleanPtr _chunkMode(nodemap.GetNode("ChunkModeActive"));
            _chunkMode->SetValue(onOff);

            if (onOff)
            {
                cameras[i].ChunkSelector.SetValue(ChunkSelector_Timestamp);
                cameras[i].ChunkEnable.SetValue(true);
            }
        }
        cout << "Metadata set to : " << onOff << endl;
    }
    catch (const GenericException &e)
    {
        cerr << "Exeception on setMetadata(). Reason: "
             << e.GetDescription() << e.what() << endl;
    }
    return;
}

void setSynchronization(bool trigger, bool strobe)
{

    for (int i = 0; i < num_devices; i++)
    {
        cameras[i].TriggerSelector.SetValue(TriggerSelector_FrameStart);
        cameras[i].TriggerMode.SetValue(trigger ? TriggerMode_On : TriggerMode_Off);
        cameras[i].TriggerSource.SetValue(TriggerSource_Line1);

        // cameras[i].LineSelector.SetValue(LineSelector_Line2);
        // if(strobe)
        // {
        //     cameras[i].LineSource.SetValue(LineSource_ExposureActive);
        // }
        // else
        // {
        //     cameras[i].LineSource.SetValue(LineSource_Off);
        // }
    }

    cout << "trigger : " << trigger << endl;
}

int cameras_connect()
{
    // Initialize Plyon Library
    try
    {
        PylonInitialize();

        DeviceInfoList_t lstDevices;
        CTlFactory &TlFactory = CTlFactory::GetInstance();
        num_devices = TlFactory.EnumerateDevices(lstDevices);

        cameras.Initialize(num_devices);
        cout << num_devices << " cameras detected " << endl;
        display_data = (void **)calloc(num_devices, sizeof(void *));

        cout << "Open CV Version: " << CV_VERSION << endl;

        // Set Cameras to default Configuation, create viewing windows
        for (size_t i = 0; i < num_devices; ++i)
        {
            display_data[i] = malloc(MAX_IMAGE_SIZE);
            if (display_data[i] == NULL)
            {
                cout << "Malloc Error" << endl;
                return -1;
            }

            cameras[i].Attach(TlFactory.CreateDevice(lstDevices[i]));
            cameras[i].Open();

            string guid = string(cameras[i].GetDeviceInfo().GetSerialNumber().c_str());
            string folder = string("Config/");
            string top_level = string(INSTALL_DIR);
            string extension = string(".pfs");

            extension.insert(0, guid);
            extension.insert(0, folder);
            extension.insert(0, top_level);

            cerr << extension.c_str() << endl;

            CFeaturePersistence::Load(extension.c_str(), &cameras[i].GetNodeMap(), true);

            ostringstream intString;
            intString << "Camera_" << i;
            cv::namedWindow(intString.str(), cv::WINDOW_NORMAL);
        }

        for (int i = 0; i < num_devices; i++)
        {
            // Set Static State Variables
            CIntegerPtr _width(cameras[i].GetNodeMap().GetNode("Width"));
            CIntegerPtr _height(cameras[i].GetNodeMap().GetNode("Height"));
            CIntegerPtr _offX(cameras[i].GetNodeMap().GetNode("OffsetX"));
            CIntegerPtr _offY(cameras[i].GetNodeMap().GetNode("OffsetY"));
            width[i] = _width->GetValue();
            height[i] = _height->GetValue();
            offX[i] = _offX->GetValue();
            offY[i] = _offY->GetValue();
            CIntegerPtr _maxTransferSize(cameras[i].GetStreamGrabberNodeMap()
                                                   .GetNode("MaxTransferSize"));
            CIntegerPtr _transferPriority(cameras[i].GetStreamGrabberNodeMap()
                                                    .GetNode("TransferLoopThreadPriority"));
            _maxTransferSize->SetValue(_maxTransferSize->GetMax());
            _transferPriority->SetValue(TRANSFER_PRIORITY);
        }
    }
    catch (const GenericException &e)
    {
        cerr << "Exeception on connect(). Reason: "
             << e.GetDescription() << e.what() << endl;
    }

    setSynchronization(false, false);
    setPerformanceMode(false);

    Json::Value defaultROI;
    Json::Reader reader;

    string defaultROIFilename = string("Config/defaultROI.json");
    defaultROIFilename.insert(0, string(INSTALL_DIR));

    std::ifstream roiFile(defaultROIFilename.c_str());
    reader.parse(roiFile, defaultROI, false);

    for (int i = 0; i < num_devices; i++)
    {
        string guid = string(cameras[i].GetDeviceInfo().GetSerialNumber().c_str());
        cameras_setROI(defaultROI[guid.c_str()]["Width"].asInt(),
                       defaultROI[guid.c_str()]["Height"].asInt(),
                       defaultROI[guid.c_str()]["OffX"].asInt(),
                       defaultROI[guid.c_str()]["OffY"].asInt(),
                       i);
    }

    cv::destroyAllWindows();
    aI.setNumCams(num_devices);
    captStreamCam = num_devices - 1;
    return num_devices;
}

void cameras_disconnect()
{
    try
    {
        cout << "Disconnecting cameras..." << endl;
        for (int i = 0; i < num_devices; i++)
        {

            cameras[i].Close();
        }
        //PylonTerminate();
        cout << "Cameras disconnected!" << endl;
    }
    catch (const GenericException &e)
    {
        cerr << "Exeception on disconnect(). Reason: "
             << e.GetDescription() << e.what() << endl;
    }
    return;
}

void cameras_start(int numFrames)
{
    try

    {
        EGrabStrategy strategy = performanceMode ?
                                     GrabStrategy_OneByOne : GrabStrategy_LatestImageOnly;

        for (int i = 0; i < num_devices; i++)
        {
            if (numFrames)
            {
                if (saveDataFromCam[i])
                {
                    cameras[i].StartGrabbing((size_t)numFrames,
                                             strategy,
                                             GrabLoop_ProvidedByInstantCamera);
                }
            }
            else
            {
                cameras[i].StartGrabbing(strategy);
                cout << "start" << i << endl;
            }
        }
    }
    catch (const GenericException &e)
    {
        cerr << "Exeception on start(). Reason: "
             << e.GetDescription() << e.what() << endl;
    }

    cout << "Cameras Started" << endl;
}

void cameras_stop()
{
    try
    {
        for (int i = 0; i < 1; i++)
        {
            cameras.StopGrabbing();
        }
    }
    catch (const GenericException &e)
    {
        cerr << "Exeception on stop(). Reason: "
             << e.GetDescription() << e.what() << endl;
    }

    cout << "Cameras Stopped" << endl;
}

void cameras_grab_for_preview(int liveStream[], int enCam[], int streamCam)
{
    captStreamCam = streamCam;
    for (int i = 0; i < 7; i++)
    {
        saveDataFromCam[i] = enCam[i];
    }

    for (int i = 0; i < num_devices; i++)
    {
        CGrabResultPtr ptr_grab_result = CGrabResultPtr();

        if (cameras[i].RetrieveResult(5000, ptr_grab_result,
                                        TimeoutHandling_ThrowException)
            && ptr_grab_result->GrabSucceeded())
        {
            void *data = ptr_grab_result->GetBuffer();

            memcpy(display_data[i], data, height[i] * width[i]);
            const cv::Mat M(height[i], width[i], CV_8U, display_data[i]);

            ostringstream int_string;
            int_string << "Camera_" << i;
            if (liveStream[i])
            {
                if (!liveWindows[i])
                {
                    liveWindows[i] = true;
                }
                cv::imshow(int_string.str(), M);
                cv::resizeWindow(int_string.str(),
                                    (width[i] * WINDOW_HEIGHT) / height[i],
                                    WINDOW_HEIGHT);
                cv::waitKey(1);
            }
            else
            {
                if (liveWindows[i])
                {
                    liveWindows[i] = false;
                    cv::destroyWindow(int_string.str());
                }
            }
        }
        else
        {
            cerr << "Failure on Grab" << ptr_grab_result->GrabSucceeded() << endl;
            return;
        }
    }
}

string get_time_string()
{
    time_t raw_time;
    struct tm *time_info;
    char buffer[80];

    time(&raw_time);
    time_info = localtime(&raw_time);

    strftime(buffer, sizeof(buffer), "%d-%m-%Y---%I-%M-%S", time_info);
    string time_string(buffer);

    return time_string;
}

recstat_t start_cameras_grab(std::string path, int obs_frames, int rec_frames, bool motion_act,
                             bool direct_save, bool trigger, bool strobe, int liveStream[],
                             int enCam[], int streamCam)
{
    // Setup
    string start_time = get_time_string();
    captStreamCam = streamCam;
    for (int i = 0; i < 7; i++)
    {
        saveDataFromCam[i] = enCam[i];
    }

    // Initialize event handlers and files
    FILE **arhFile = (FILE **) malloc(sizeof(FILE *) * num_devices);
    CustomImageEventHandler *grabHandlers =
        (CustomImageEventHandler *) malloc(sizeof(CustomImageEventHandler) * num_devices);
    for (int i = 0; i < num_devices; i ++)
    {
        if (saveDataFromCam[i])
        {
            if (!direct_save) {
                stringstream sstream;
                string tempFilename;
                sstream << path << "/camera_" << i << ".tmp";
                sstream >> tempFilename;
                arhFile[i] = fopen(tempFilename.c_str(), "wb");
                if (arhFile[i] == NULL)
                {
                    cout << "Failure Opening: " << tempFilename.c_str() << endl;
                }
            }
            
            new (&(grabHandlers[i])) CustomImageEventHandler(i, rec_frames,
                                                             height[i], width[i],
                                                             motion_act, direct_save,
                                                             arhFile[i], path);
            cameras[i].RegisterImageEventHandler(&(grabHandlers[i]),
                                                 RegistrationMode_Append,
                                                 Cleanup_None);
            
        }

        setPerformanceMode(true);
        setMetadata(true);
        setSynchronization(trigger, strobe);
        cameras_start(obs_frames);
    }

    recstat_t status;
    status.start_time = start_time;
    status.grabHandlers = grabHandlers;
    status.arhFile = arhFile;
    status.direct_save = direct_save;
    status.target_path = path;
    return status;
}

/**
 * This function is called to wrap up the camera grab process. It does NOT
 * terminate camera grab if it is still in progress (in fact it will cause
 * an assertion failure). It should only be called if recording has completed,
 * either normally or by early termination.
 * 
 * @param status as returned by ``start_cameras_grab``
 */
void clean_up_camera_grab(recstat_t status)
{
    // Waiting for .IsGrabbing() to change to false for all cameras should not be necessary;
    // pylon API reference indicates that .StopGrabbing() is synchronous.
    for (int i = 0; i < num_devices; i++)
    {
        assert(!(cameras[i].IsGrabbing()));
    }

    // Get stats and deregister handlers
    float time_acc = 0;
    int missed_frames_acc = 0;
    for (int i = 0; i < num_devices; i++)
    {
        if (saveDataFromCam[i])
        {
            time_acc += status.grabHandlers[i].getIntervalAcc();
            missed_frames_acc += status.grabHandlers[i].getMissedFrames();
            cameras[i].DeregisterImageEventHandler(&(status.grabHandlers[i]));
        }
    }
    cout << "Total interval: " << time_acc << endl;
    cout << "We have missed a total of " << missed_frames_acc << " frames" << endl;

    // Close files as needed
    if (!status.direct_save)
    {
        for (int i = 1; i < num_devices; i++)
        {
            if (saveDataFromCam[i]) {
                fclose(status.arhFile[i]);
            }
        }
    }

    // Change camera states
    setPerformanceMode(false);
    setMetadata(false);
    setSynchronization(false, false);
    cameras_start();

    // Write metadata to JSON
    Json::Value capture_metadata;
    Json::StyledWriter styled_writer;
    for (int i = 0; i < num_devices; i++)
    {
        if (saveDataFromCam[i])
        {
            capture_metadata["Number of Frames"][to_string(i)]
                = status.grabHandlers[i].recorded_frames;
            capture_metadata["ROI"]["Height"][to_string(i)] = height[i];
            capture_metadata["ROI"]["Width"][to_string(i)] = width[i];
            capture_metadata["ROI"]["OffX"][to_string(i)] = offX[i];
            capture_metadata["ROI"]["OffY"][to_string(i)] = offY[i];

            for (int j = 0; j < status.grabHandlers[i].recorded_frames; j++)
            {
                capture_metadata["TimeStamps"][to_string(i)][to_string(j)]
                    = status.grabHandlers[i].getTimestamp(j);
                capture_metadata["Frame Counts"][to_string(i)][to_string(j)]
                    = status.grabHandlers[i].getFrameNumber(j);
            }
        }
    }
    capture_metadata["Number of Cameras"] = num_devices;
    capture_metadata["FPS"] = fps;
    capture_metadata["Capture Start Time and Date"] = status.start_time.c_str();

    stringstream sstream;
    string meta_filename;
    ofstream meta_file;
    sstream << status.target_path << "/capture_metadata.json";
    sstream >> meta_filename;
    meta_file.open(meta_filename.c_str());
    meta_file << styled_writer.write(capture_metadata);
    meta_file.close();

    // Clean up
    free(status.arhFile);
    free(status.grabHandlers);
}

/**
 * This function is the entry point to terminate camera grabbing prematurely.
 * 
 * @param status as returned by ``start_cameras_grab``
 */
void terminate_camera_grab()
{
    for (int i = 0; i < num_devices; i ++)
    {
        cameras[i].StopGrabbing();
    }
}

/**
 * Are all cameras done grabbing?
 */
bool is_all_cameras_done()
{
    for (int i = 0; i < num_devices; i++)
    {
        if (cameras[i].IsGrabbing()) {
            return false;
        }
    }
    return true;
}

void wait_for_grab_to_finish(float expected_time_sec = 0)
{
    usleep(expected_time_sec * 1e6);
    while (!is_all_cameras_done())
    {
        usleep(1e5);
    }
}


void _legacy_cameras_grab(std::string path, int obs_frames, int rec_frames, bool motion_act,
                          bool direct_save, bool trigger, bool strobe, int liveStream[],
                          int enCam[], int streamCam)
{
    try
    {
        time_t raw_time;
        struct tm *time_info;
        char buffer[80];

        time(&raw_time);
        time_info = localtime(&raw_time);

        strftime(buffer, sizeof(buffer), "%d-%m-%Y---%I-%M-%S", time_info);
        string time_string(buffer);

        captStreamCam = streamCam;

        for (int i = 0; i < 7; i++)
        {
            saveDataFromCam[i] = enCam[i];
        }

        // We want to save a determined number of frames
        if (obs_frames != 0)
        {
            FILE **arhFile = (FILE **)malloc(sizeof(FILE *) * num_devices);
            CustomImageEventHandler *grabHandlers = (CustomImageEventHandler *)
                malloc(sizeof(CustomImageEventHandler) * num_devices);

            for (int i = 0; i < num_devices; i++)
            {
                if (saveDataFromCam[i])
                {
                    if (!direct_save)
                    {
                        stringstream sstream;
                        string tempFilename;
                        sstream << path << "/camera_" << i << ".tmp";
                        sstream >> tempFilename;
                        arhFile[i] = fopen(tempFilename.c_str(), "wb");
                        if (arhFile[i] == NULL)
                        {
                            cout << "Failure Opening: " << tempFilename.c_str() << endl;
                        }
                    }

                    new (&(grabHandlers[i])) CustomImageEventHandler(
                        i, rec_frames, height[i], width[i],
                        motion_act, direct_save, arhFile[i], path
                    );
                    cameras[i].RegisterImageEventHandler(&(grabHandlers[i]),
                                                         RegistrationMode_Append,
                                                         Cleanup_None);
                }
            }

            setPerformanceMode(true);
            setMetadata(true);
            setSynchronization(trigger, strobe);
            cameras_start(obs_frames);

            if (trigger)
            {
                //primeTrigger();
            }
            bool grabbing_done = false;
            WaitObject::Sleep(obs_frames / (fps * 1000.0) + 2000);
            while (!grabbing_done)
            {
                bool done = true;
                for (int i = 0; i < num_devices; i++)
                {
                    done = done && !(cameras[i].IsGrabbing());
                }
                grabbing_done = done;
                WaitObject::Sleep(500);
            }
            cout << "Cameras stopped (isGrabbing is false)" << endl;

            float time_acc = 0;
            int missed_frame_acc = 0;

            for (int i = 0; i < num_devices; i++)
            {
                if (saveDataFromCam[i])
                {
                    time_acc += grabHandlers[i].getIntervalAcc();
                    missed_frame_acc += grabHandlers[i].getMissedFrames();
                    cameras[i].DeregisterImageEventHandler(&(grabHandlers[i]));
                }
            }

            cout << "Total interval :" << time_acc << endl;
            cout << "We have missed a total of " << missed_frame_acc << " frames" << endl;

            if (!direct_save)
            {
                for (int i = 0; i < num_devices; i++)
                {
                    if (saveDataFromCam[i])
                    {
                        fclose(arhFile[i]);
                    }
                }
            }

            setPerformanceMode(false);
            setMetadata(false);
            setSynchronization(false, false);
            cameras_start();

            Json::Value capture_metadata;
            Json::StyledWriter styled_writer;

            for (int i = 0; i < num_devices; i++)
            {
                if (saveDataFromCam[i])
                {
                    capture_metadata["Number of Frames"][to_string(i)]
                        = grabHandlers[i].recorded_frames;
                    capture_metadata["ROI"]["Height"][to_string(i)] = height[i];
                    capture_metadata["ROI"]["Width"][to_string(i)] = width[i];
                    capture_metadata["ROI"]["OffX"][to_string(i)] = offX[i];
                    capture_metadata["ROI"]["OffY"][to_string(i)] = offY[i];
                    for (int j = 0; j < grabHandlers[i].recorded_frames; j++)
                    {
                        capture_metadata["TimeStamps"][to_string(i)][to_string(j)]
                            = grabHandlers[i].getTimestamp(j);
                        capture_metadata["Frame Counts"][to_string(i)][to_string(j)]
                            = grabHandlers[i].getFrameNumber(j);
                    }
                }
            }

            capture_metadata["Number of Cameras"] = num_devices;
            capture_metadata["FPS"] = fps;
            capture_metadata["Capture Start Time and Date"] = time_string.c_str();

            stringstream sstream;
            string meta_filename;
            ofstream meta_file;

            sstream << path << "/capture_metadata.json";
            sstream >> meta_filename;

            meta_file.open(meta_filename.c_str());
            meta_file << styled_writer.write(capture_metadata);
            meta_file.close();

            free(arhFile);
            free(grabHandlers);

            if (direct_save)
            {
                //createPreview(num_devices,numImages,path);
            }
        }

        // We only want to view frames here, not save them
        else
        {
            for (int i = 0; i < num_devices; i++)
            {
                CGrabResultPtr ptr_grab_result = CGrabResultPtr();

                if (cameras[i].RetrieveResult(5000, ptr_grab_result,
                                              TimeoutHandling_ThrowException)
                    && ptr_grab_result->GrabSucceeded())
                {
                    void *data = ptr_grab_result->GetBuffer();

                    memcpy(display_data[i], data, height[i] * width[i]);
                    const cv::Mat M(height[i], width[i], CV_8U, display_data[i]);

                    ostringstream int_string;
                    int_string << "Camera_" << i;
                    if (liveStream[i])
                    {
                        if (!liveWindows[i])
                        {
                            liveWindows[i] = true;
                        }
                        cv::imshow(int_string.str(), M);
                        cv::resizeWindow(int_string.str(),
                                         (width[i] * WINDOW_HEIGHT) / height[i],
                                         WINDOW_HEIGHT);
                        cv::waitKey(1);
                    }
                    else
                    {
                        if (liveWindows[i])
                        {
                            liveWindows[i] = false;
                            cv::destroyWindow(int_string.str());
                        }
                    }
                }
                else
                {
                    cerr << "Failure on Grab" << ptr_grab_result->GrabSucceeded() << endl;
                    return;
                }
            }
        }
    }
    catch (const GenericException &e)
    {
        cerr << "Exeception on grab(). Reason: "
             << e.GetDescription() << e.what() << endl;
    }

    return;
}

void cameras_save(std::string path)
{
    try
    {
        stringstream sstream, sstream2;
        string metaFilename, jsonString;
        sstream << path << "/capture_metadata.json";
        sstream >> metaFilename;
        Json::Value captureMetadata;
        Json::Reader reader;
        std::ifstream metaFile(metaFilename.c_str());
        reader.parse(metaFile, captureMetadata, false);

        char *buffer = (char *)malloc(MAX_IMAGE_SIZE);
        if (buffer == NULL)
        {
            cerr << "Malloc Failure" << endl;
            return;
        }

        int maxFrameNumber = captureMetadata["Max Frame Number"].asInt();

        for (int j = 0; j < captureMetadata["Number of Cameras"].asInt(); j++)
        {
            stringstream sstream;
            string tempFilename;
            FILE *tempFile;

            sstream << path << "/camera_" << j << ".tmp";
            sstream >> tempFilename;

            int s_height = captureMetadata["ROI"]["Height"][to_string(j)].asInt();
            int s_width = captureMetadata["ROI"]["Width"][to_string(j)].asInt();

            for (int i = 0; i < captureMetadata["Number of Frames"][to_string(j)].asInt(); i++)
            {
                if (i == 0)
                {
                    tempFile = fopen(tempFilename.c_str(), "r");
                    if (tempFile == NULL)
                    {
                        cerr << "Failure on opening disk buffer - Err: "
                             << strerror(errno) << endl;
                        return;
                    }
                }

                int read = fread((void *)buffer, (s_height * s_width), 1, tempFile);

                if (read != 1)
                {
                    cerr << "Failure on reading from disk buffer: - ErrNum: "
                         << ferror(tempFile) << endl;
                    perror("Error : ");
                    cerr << feof(tempFile) << endl;
                    return;
                }
                stringstream sstream_img, sstream_imgJ;
                string imgFilename, imgFilenameJ;

                // sstream_img << path <<"/camera_" << j << "_img_"
                //     << captureMetadata["Frame Counts"][to_string(j)][to_string(i)] << ".png";

                sstream_imgJ << path << "/camera_" << j << "_img_"
                    << captureMetadata["Frame Counts"][to_string(j)][to_string(i)] << ".jpg";

                // sstream_img >> imgFilename;
                sstream_imgJ >> imgFilenameJ;

                const cv::Mat M(s_height, s_width, CV_8U, buffer);
                cv::imwrite(imgFilenameJ.c_str(), M);

                // CImagePersistence::Save(ImageFileFormat_Png,
                //                         imgFilename.c_str(),
                //                         (const void*) buffer,
                //                         s_height * s_width,
                //                         PixelType_Mono8,
                //                         s_width,
                //                         s_height,
                //                         0,
                //                         ImageOrientation_TopDown);

                if (i == captureMetadata["Number of Frames"][to_string(j)].asInt())
                {
                    fclose(tempFile);
                }
            }
        }
        free(buffer);

        createPreview(captureMetadata["Number of Cameras"].asInt(), 0, path);
    }
    catch (const GenericException &e)
    {
        cerr << "Exeception on grabAndSave(). Reason: "
             << e.GetDescription() << e.what() << endl;
    }

    cout << "Image Seperation done and Preview created" << endl;

    return;
}

void cameras_setFPS(float new_fps, bool for_trigger)
{
    try
    {
        if (!for_trigger)
        {
            for (int i = 0; i < num_devices; i++)
            {
                INodeMap &nodemap = cameras[i].GetNodeMap();
                CFloatPtr _fps(nodemap.GetNode("AcquisitionFrameRate"));
                CBooleanPtr _enable(nodemap.GetNode("AcquisitionFrameRateEnable"));
                _fps->SetValue(new_fps);
                _enable->SetValue(true);
            }
            cout << "FPS set to: " << fps << endl;
        }
        else
        {
            for (int i = 0; i < num_devices; i++)
            {
                INodeMap &nodemap = cameras[i].GetNodeMap();
                CBooleanPtr _enable(nodemap.GetNode("AcquisitionFrameRateEnable"));
                _enable->SetValue(false);
                cout << "FPS set to: Free Running" << fps << endl;
            }
        }
        fps = new_fps;
        aI.setFPS(fps);
    }
    catch (const GenericException &e)
    {
        cerr << "Exeception on setFPS(). Reason: "
             << e.GetDescription() << endl;
    }
}

void cameras_setROI(unsigned int newWidth, unsigned int newHeight,
                    unsigned int newOffsetX, unsigned int newOffsetY, int index)
{
    try
    {
        int init = (index == -1) ? 0 : index;
        int ceil = (index == -1) ? num_devices : index + 1;

        for (int i = init; i < ceil; i++)
        {

            INodeMap &nodemap = cameras[i].GetNodeMap();

            CIntegerPtr _offsetX(nodemap.GetNode("OffsetX"));
            CIntegerPtr _offsetY(nodemap.GetNode("OffsetY"));
            CIntegerPtr _width(nodemap.GetNode("Width"));
            CIntegerPtr _height(nodemap.GetNode("Height"));

            //offX[i] = Adjust(newOffsetX, _offsetX->GetMin(),
            //                  _offsetX->GetMax(), _offsetX->GetInc());
            _offsetX->SetValue(offX[i]);

            //offY[i] = Adjust(newOffsetY, _offsetY->GetMin(),
            //                  _offsetY->GetMax(), _offsetY->GetInc());
            _offsetY->SetValue(offY[i]);

            //width[i] = Adjust(newWidth, _width->GetMin(),
            //                  _width->GetMax(), _width->GetInc());
            _width->SetValue(width[i]);

            //height[i] = Adjust(newHeight, _height->GetMin(),
            //                  _height->GetMax(), _height->GetInc());
            _height->SetValue(height[i]);
            //cout<<offX[i]<<" "<<offY[i]<<" "<<width[i]<<" "<<height[i]<<endl;
        }
    }
    catch (const GenericException &e)
    {
        cerr << "Exeception on setROI(). Reason: "
             << e.GetDescription() << e.what() << endl;
    }
    cout << "ROI Set" << endl;
}

std::array<long int, 4> cameras_getROI(int i)
{
    CIntegerPtr _width(cameras[i].GetNodeMap().GetNode("Width"));
    CIntegerPtr _height(cameras[i].GetNodeMap().GetNode("Height"));
    CIntegerPtr _offX(cameras[i].GetNodeMap().GetNode("OffsetX"));
    CIntegerPtr _offY(cameras[i].GetNodeMap().GetNode("OffsetY"));

    std::array<long int, 4> res{{_width->GetValue(), _height->GetValue(),
                                 _offX->GetValue(), _offY->GetValue()}};
    return res;
}

string cameras_getGUID(int i)
{
    return string(cameras[i].GetDeviceInfo().GetSerialNumber().c_str());
}

#ifdef USE_MAIN
int main()
{
    cameras_connect();
    cout << "Connected" << endl;
    cameras_start();
    stringstream sstream;
    string tempFilename;
    sstream << "data";
    sstream >> tempFilename;
    cameras_grab(tempFilename, 10, true);
    cout << "Grabbed" << endl;
    cameras_stop();
    cout << "Stopped" << endl;
    cameras_disconnect();
}
#endif
