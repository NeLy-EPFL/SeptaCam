#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <jsoncpp/json/json.h>

#include "cpptk.h"
#include "../constants.h"
#include "../NIR_Imaging.h"
#include "../common.h"

using namespace Tk;

static bool captureRunning = false;
static int trigger = true;
static int strobe = false;
static int seperate_images = true;
static int motion_activation = false;
static int thImage = true;
static int opFlow = true;
static int behCams = true;
static int stimulus = false;
static int mostRecent = true;
static int selectExp = false;
std::string trialStr = "001";
std::string stimTimeStr = "";
int saveCams[7] = {false, false, false, false, false, false, false};
int liveStream[7] = {false, false, false, false, false, false, false};
int captStream[7] = {false, false, false, false, false, false, false};
int videofrom[8] = {false, false, false, false, false, false, false, true};
int streamCam = 6;
std::string imgDirectory;
std::ostringstream sequence, stimTime;

static unsigned int numCameras;
int obs_scale = 1;
int rec_scale = 1;

void alert_error(std::string msg)
{
    tk_messageBox() - defaultbutton("ok") - messagetext(msg) - messagetype(ok);
}

void updateFrameView()
{
    if (!captureRunning)
    {
        cameras_grab("Not Applicable", 0, 0, false, false, false, false,
                     liveStream, saveCams, streamCam);
    }
    after(50, updateFrameView);
    return;
}

float convertTime(float t, int param)
{
    float secTime = 0;
    switch (param)
    {
    case 1:
        secTime = t;
        break;
    case 2:
        secTime = t * 60;
        break;
    case 3:
        secTime = t * 3600;
        break;
    }

    return secTime;
}

std::string creatingDirectory(std::string output_dir, std::string experimenter,
                              std::string genotype, std::string flyNum, std::string typeImg,
                              std::string trial, bool two_photon_dir = false)
{
    time_t raw_time;
    struct tm *time_info;
    char bufferDate[80], bufferTime[80];
    time(&raw_time);
    time_info = localtime(&raw_time);

    strftime(bufferDate, sizeof(bufferDate), "%y%m%d", time_info);
    strftime(bufferTime, sizeof(bufferTime), "%H%M%S", time_info);
    std::string date_string(bufferDate);
    std::string time_string(bufferTime);
    struct stat sb;

    std::string newPath;
    newPath.insert(0, output_dir);
    newPath.insert(newPath.length(), experimenter);
    newPath.insert(newPath.length(), "/");

    if (!(stat(newPath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)))
    {
        std::cout << "Creating " << newPath.c_str() << std::endl;
        const int dir_err = mkdir(newPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (-1 == dir_err)
        {
            std::cout << "Failure Creating Directory: " << newPath.c_str() << std::endl;
        }
    }

    newPath.insert(newPath.length(), date_string);
    newPath.insert(newPath.length(), "_");
    newPath.insert(newPath.length(), genotype);
    newPath.insert(newPath.length(), "/");

    if (!(stat(newPath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)))
    {
        std::cout << "Creating " << newPath.c_str() << std::endl;
        const int dir_err = mkdir(newPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (-1 == dir_err)
        {
            std::cout << "Failure Creating Directory: " << newPath.c_str() << std::endl;
        }
    }

    newPath.insert(newPath.length(), "Fly");
    newPath.insert(newPath.length(), flyNum);
    newPath.insert(newPath.length(), "/");

    if (!(stat(newPath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)))
    {
        std::cout << "Creating " << newPath.c_str() << std::endl;
        const int dir_err = mkdir(newPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (-1 == dir_err)
        {
            std::cout << "Failure Creating Directory: " << newPath.c_str() << std::endl;
        }
    }

    newPath.insert(newPath.length(), trial);
    newPath.insert(newPath.length(), "_");
    newPath.insert(newPath.length(), typeImg);
    newPath.insert(newPath.length(), "/");

    if (!(stat(newPath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)))
    {
        std::cout << "Creating " << newPath.c_str() << std::endl;
        const int dir_err = mkdir(newPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (-1 == dir_err)
        {
            std::cout << "Failure Creating Directory: " << newPath.c_str() << std::endl;
        }
    }
    
    // if (thImage)
    // {
    //     std::string thImPath, thSynPath;
    //     thImPath.insert(0,newPath);
    //     thSynPath.insert(0,newPath);

    //     thImPath.insert(thImPath.length(),"ThorImage_");
    //     thSynPath.insert(thSynPath.length(),"ThorSync_");

    //     thImPath.insert(thImPath.length(),trial);
    //     thSynPath.insert(thSynPath.length(),trial);

    //     if (!(stat(thImPath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)))
    //     {
    //         std::cout << "Creating "<<thImPath.c_str() << std::endl;
    //         const int dir_err1 = mkdir(thImPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    //         std::cout << "Creating "<<thSynPath.c_str() << std::endl;
    //     const int dir_err2 = mkdir(thSynPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    //     if (dir_err1 == -1 || dir_err2 == -1 )
    //     {
    //         std::cout << "Failure Creating 2-Photon Directories: "<< std::endl;
    //     }
            
    //     }
    //     else
    //     {
    //         alert_error("2-Photon Directories already exist");
    //         captureRunning = false;
    //         return "";
    //     }
    // }

    if (two_photon_dir)
    {
        newPath.insert(newPath.length(), "2p");

        if (!(stat(newPath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)))
        {
            std::cout << "Creating " << newPath.c_str() << std::endl;
            const int dir_err3 = mkdir(newPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if (dir_err3 == -1)
            {
                std::cout << "Failure Creating BehCams Directory: " << std::endl;
            }
        }
        else
        {
            alert_error("BehCams Directory already exists");
            captureRunning = false;
            return "";
        }
    }
    else
    {
        newPath.insert(newPath.length(), "behData");

        if (!(stat(newPath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)))
        {
            std::cout << "Creating " << newPath.c_str() << std::endl;
            const int dir_err3 = mkdir(newPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if (dir_err3 == -1)
            {
                std::cout << "Failure Creating BehCams Directory: " << std::endl;
            }
        }
        else
        {
            alert_error("BehCams Directory already exists");
            captureRunning = false;
            return "";
        }
    }

    return newPath;

    // else{
    //     newPath.insert(newPath.length(),"_");
    //     newPath.insert(newPath.length(),time_string);
    //     std::cout << "Creating "<<newPath.c_str() << " instead" << std::endl;

    //     const int dir_err = mkdir(newPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    //     if (-1 == dir_err)
    //     {
    //         std::cout << "Failure Creating Directory: "<< newPath.c_str() << std::endl;
    //     }
    // }
}

void start_capture()
{
    int fd, fd_stim;
    std::string dirCreated;
    char *fifo_path = "/tmp/guiData.fifo";
    char *fifo_path_stim = "/tmp/stimData.fifo";

    captureRunning = true;

    std::string string_time_rec(".entry_time_rec" << get());
    std::string string_time_obs(".entry_time_obs" << get());
    std::string string_fps(".entry_FPS" << get());
    std::string experimenter(".entry_experimenter" << get());
    std::string genotype(".entry_genotype" << get());
    std::string flyNum(".entry_fly" << get());
    std::string typeImg(".entry_type" << get());
    std::string trial(".entry_trial" << get());

    float fps = behCams ? std::stof(string_fps) : 0;
    float fl_rec_time = motion_activation ?
        std::stof(string_time_rec) : std::stof(string_time_obs);
    float fl_obs_time = std::stof(string_time_obs);
    float rec_time_sec = motion_activation ?
        convertTime(fl_rec_time, rec_scale) : convertTime(fl_rec_time, obs_scale);
    float obs_time_sec = convertTime(fl_obs_time, obs_scale);
    int rec_frames = (int)(fps * rec_time_sec);
    int obs_frames = (int)(fps * obs_time_sec);

    if (obs_frames < 1 && behCams)
    {
        alert_error("Invalid Parameters");
        captureRunning = false;
        return;
    }

    if (WINDOWS_SHARE)
    {
        dirCreated = creatingDirectory(WINDOWS_SHARE, experimenter, genotype, flyNum,
                                       typeImg, trial, true);
    }
    dirCreated = creatingDirectory(OUTPUT_DIR, experimenter, genotype, flyNum, typeImg, trial);

    if (captureRunning == false)
    {
        return;
    }

    int mode = thImage * 4 + opFlow * 2 + behCams * 1;
    // std::cout << "obs_frames "<< obs_frames << std::endl;
    // std::cout << "rec_frames "<< rec_frames << std::endl;

    // Writing data to FIFO (Named pipe) for cameras and optic flow
    if (trigger)
    {
        fd = open(fifo_path, O_WRONLY);

        std::ostringstream strModeFR;
        strModeFR << mode << "," << fps << "," << obs_time_sec << "," << dirCreated;

        write(fd, strModeFR.str().c_str(), strlen(strModeFR.str().c_str()));
        close(fd);
        unlink(fifo_path);
    }

    // Writing data to FIFO (Named pipe) for stimulus
    if (stimulus)
    {
        fd_stim = open(fifo_path_stim, O_WRONLY);

        std::ostringstream strStim;
        strStim << "1," << obs_time_sec << "," << dirCreated;

        write(fd_stim, strStim.str().c_str(), strlen(strStim.str().c_str()));
        close(fd_stim);
        unlink(fifo_path_stim);
    }

    // Create directory only for images
    struct stat st;
    dirCreated.insert(dirCreated.length(), "/images");
    if (!(stat(dirCreated.c_str(), &st) == 0 && S_ISDIR(st.st_mode)))
    {
        std::cout << "Creating " << dirCreated.c_str() << std::endl;
        const int dir_err3 = mkdir(dirCreated.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (dir_err3 == -1)
        {
            std::cout << "Failure Creating BehCams Directory: " << std::endl;
        }
    }

    imgDirectory = dirCreated;

    // Starting cameras
    if (mode % 2 == 1)
    {
        cameras_stop();
        cameras_setFPS(fps, trigger);
        // cameras_start();

        // cameras_grab(dirCreated, obs_frames, rec_frames, motion_activation, seperate_images,
        //              trigger, strobe, liveStream, saveCams, streamCam);
        recstat_t recording_status = start_cameras_grab(
            dirCreated, obs_frames, rec_frames, motion_activation, seperate_images,
            trigger, strobe, liveStream, saveCams, streamCam
        );
        wait_for_grab_to_finish(rec_time_sec);
        clean_up_camera_grab(recording_status);

        cameras_stop();
        cameras_setFPS(30, false);
        cameras_start();

        // Capture completed
        alert_error("Capture Completed");
    }
    else
    {
        alert_error("Recording only optical flow data");
    }

    int valTrial = stoi(trial);
    valTrial += 1;
    std::ostringstream incTrial;
    if (valTrial < 10)
    {
        incTrial << "00" << valTrial;
    }
    else
    {
        incTrial << "0" << valTrial;
    }
    trialStr = incTrial.str();

    captureRunning = false;
    return;
}

void camsStatus()
{
    for (int i = 0; i < numCameras; i++)
    {
        std::ostringstream name;
        std::ostringstream name2;
        std::ostringstream name3;
        name << ".check_cam" << i;
        name2 << ".check_liveStream" << i;
        name3 << ".check_captStream" << i;
        name.str() << configure() - state("normal");
        name2.str() << configure() - state("normal");
        saveCams[i] = true;
        liveStream[i] = true;
        if (i == numCameras - 1)
        {
            name3.str() << configure() - state("normal");
            captStream[i] = true;
        }
        else
        {
            name3.str() << configure() - state("disabled");
            captStream[i] = false;
        }
    }
    for (int j = numCameras; j < 7; j++)
    {
        std::ostringstream name;
        std::ostringstream name2;
        std::ostringstream name3;
        name << ".check_cam" << j;
        name2 << ".check_liveStream" << j;
        name3 << ".check_captStream" << j;
        name.str() << configure() - state("disabled");
        name2.str() << configure() - state("disabled");
        name3.str() << configure() - state("disabled");
        saveCams[j] = false;
        liveStream[j] = false;
        captStream[j] = false;
    }
}

void refresh_cameras()
{
    //cameras_disconnect();
    numCameras = cameras_connect();
    streamCam = numCameras - 1;
    cameras_start();
    camsStatus();
    updateFrameView();
}

void toggle_most_recent()
{
    if (mostRecent)
    {
        ".entry_working_directory_postproc" << configure() - state("disabled");
        selectExp = false;
    }
    else
    {
        ".entry_working_directory_postproc" << configure() - state("normal");
        selectExp = true;
    }
}

void toggle_select_experiment()
{
    if (selectExp)
    {
        ".entry_working_directory_postproc" << configure() - state("normal");
        mostRecent = false;
    }
    else
    {
        ".entry_working_directory_postproc" << configure() - state("disabled");
        mostRecent = true;
    }
}

void toggle_all()
{
    if (videofrom[7])
    {
        ".check_0" << configure() - state("disabled");
        ".check_1" << configure() - state("disabled");
        ".check_2" << configure() - state("disabled");
        ".check_3" << configure() - state("disabled");
        ".check_4" << configure() - state("disabled");
        ".check_5" << configure() - state("disabled");
        ".check_6" << configure() - state("disabled");
        for (int i = 0; i < 7; i++)
        {
            videofrom[i] = false;
        }
    }
    else
    {
        ".check_0" << configure() - state("normal");
        ".check_1" << configure() - state("normal");
        ".check_2" << configure() - state("normal");
        ".check_3" << configure() - state("normal");
        ".check_4" << configure() - state("normal");
        ".check_5" << configure() - state("normal");
        ".check_6" << configure() - state("normal");
        mostRecent = true;
    }
}

void toggle_motion_activation()
{
    if (captureRunning)
    {
        alert_error("captureRunning");
        return;
    }
    if (motion_activation)
    {
        ".entry_time_rec" << configure() - state("normal");
    }
    else
    {
        ".entry_time_rec" << configure() - state("disabled");
    }
}

std::string getDataFromSeqFile(std::string option)
{
    float val = 0;
    sequence.str("");
    stimTime.str("");
    std::string line, strNumber;
    std::ifstream seqfileRead(SEQ_FILE);
    if (seqfileRead.is_open())
    {
        while (std::getline(seqfileRead, line))
        {
            if (line.empty())
            {
                continue;
            }
            sequence << line << '\n';
            strNumber = line.substr(line.find(",") + 1);
            std::size_t found = line.find("*");
            if (found != std::string::npos)
            {
                val = val * std::stof(strNumber);
            }
            else
            {
                val += std::stof(strNumber) / 1000;
            }
        }
        stimTime << val;
        seqfileRead.close();
    }

    else
        std::cout << "Unable to open file\n";

    if (option == "sequence")
    {
        return sequence.str();
    }
    if (option == "time")
    {
        return stimTime.str();
    }
}

void toggle_stimulus()
{
    if (captureRunning)
    {
        alert_error("captureRunning");
        return;
    }
    if (stimulus)
    {
        ".label_currentStim" << configure() - state("normal");
        ".button_editStim" << configure() - state("normal");
        ".button_stimulation" << configure() - state("normal");

        std::string sequenceStr = getDataFromSeqFile("sequence");

        ".currentStim" << configure() - state("normal");
        ".currentStim" << deletetext(txt(1, 0), end);
        ".currentStim" << insert(end, sequenceStr);
        ".currentStim" << configure() - state("disabled");
    }
    else
    {
        ".label_currentStim" << configure() - state("disabled");
        ".currentStim" << configure() - state("normal");
        ".currentStim" << deletetext(txt(1, 0), end);
        ".currentStim" << configure() - state("disabled") - background("white");
        ".button_stimulation" << configure() - state("disabled");
        ".button_editStim" << configure() - state("disabled");
    }
}

void toggle_beh_cams()
{
    if (captureRunning)
    {
        alert_error("captureRunning");
        return;
    }
    if (behCams)
    {
        ".entry_FPS" << configure() - state("normal");
    }
    else
    {
        ".entry_FPS" << configure() - state("disabled");
    }
}

void toggle_capt_stream()
{
    int val = captStream[0] + captStream[1] + captStream[2] + captStream[3]
              + captStream[4] + captStream[5] + captStream[6];

    if (val == 0)
    {
        for (int i = 0; i < numCameras; i++)
        {
            std::ostringstream name;
            name << ".check_captStream" << i;
            name.str() << configure() - state("normal");
        }
    }
    else
    {
        for (int i = 0; i < numCameras; i++)
        {
            if (captStream[i] == 0)
            {
                std::ostringstream name;
                name << ".check_captStream" << i;
                name.str() << configure() - state("disabled");
            }
            else
            {
                streamCam = i;
            }
        }
    }
}

void new_stim_sequence()
{

    std::string content(".currentStim" << get(txt(1, 0), end));

    std::ofstream seqfile(SEQ_FILE);
    if (seqfile.is_open())
    {
        seqfile << content;
        seqfile.close();
    }

    else
        std::cout << "Unable to write in file\n";

    std::string timeStr = getDataFromSeqFile("time");
    ".currentStim" << configure() - state("disabled") - background("green");

    stimTimeStr = timeStr;
    obs_scale = 1;
}

void addBatch(int mode)
{
    if (captureRunning)
    {
        alert_error("Capture is Running!");
        return;
    }

    std::string dir;

    if (mostRecent)
    {
        dir = imgDirectory;
        if (dir.empty())
        {
            alert_error("No experiment run yet!");
            return;
        }
    }
    else
    {
        std::string working_dir(".entry_working_directory_postproc" << get());
        working_dir.insert(working_dir.length(), "/behData/images");
        dir = working_dir;
    }

    std::ofstream batchFile;
    batchFile.open(VID_BATCH_FILE, std::ios_base::app);
    batchFile << dir << "," << videofrom[0] << "," << videofrom[1] << "," << videofrom[2]
              << "," << videofrom[3] << "," << videofrom[4] << "," << videofrom[5]
              << "," << videofrom[6] << "," << videofrom[7] << std::endl;

    if (mode == 1)
    {
        batchFile << "now" << std::endl;
    }
    else
    {
        alert_error("Experiment added to batch");
    }
    return;
}

void addBatch_button()
{
    addBatch(0);
}

void seeBatch()
{
    std::string command;
    command.insert(0, "xdg-open ");
    command.insert(command.length(), VID_BATCH_FILE);
    system(command.c_str());
}

void editSequence()
{
    ".currentStim" << configure() - state("normal") - background("white");
    // std::string command;
    // command.insert(0,"xdg-open ");
    // command.insert(command.length(),SEQ_FILE);
    // system(command.c_str());
}

void makeVideo()
{
    if (captureRunning)
    {
        alert_error("Capture is Running!");
        return;
    }
    addBatch(1);
    std::string command;
    command.insert(0, "python3 ");
    command.insert(command.length(), MAKE_VIDEOS_FILE);
    system(command.c_str());
    alert_error("Video Completed");
    return;
}

void process()
{
    if (captureRunning)
    {
        alert_error("Capture is Running!");
        return;
    }

    std::string working_dir(".entry_working_directory_postproc_images" << get());
    working_dir.insert(0, OUTPUT_DIR);
    cameras_save(working_dir);
    alert_error("Post-Processing Completed");
    return;
}

void setAllROI()
{
    if (captureRunning)
    {
        alert_error("Capture is Running!");
        return;
    }

    std::string height(".entry_height" << get());
    std::string width(".entry_width" << get());
    std::string offsetX(".entry_Xoff" << get());
    std::string offsetY(".entry_Yoff" << get());
    cameras_stop();
    cameras_setROI(std::stoi(width), std::stoi(height),
                   std::stoi(offsetX), std::stoi(offsetY));
    cameras_start();
    return;
}

void setSingleROI()
{

    if (captureRunning)
    {
        alert_error("Capture is Running!");
        return;
    }

    std::string height(".entry_height" << get());
    std::string width(".entry_width" << get());
    std::string offsetX(".entry_Xoff" << get());
    std::string offsetY(".entry_Yoff" << get());
    std::string index(".entry_ROI_single" << get());

    if (std::stoi(index) >= numCameras)
    {
        alert_error("Invalid Camera Index");
    }

    cameras_stop();
    cameras_setROI(std::stoi(width), std::stoi(height),
                   std::stoi(offsetX), std::stoi(offsetY), std::stoi(index));
    cameras_start();
    return;
}

void setDefaultROI()
{
    Json::Value defaultROI;
    Json::StyledWriter styledWriter;

    for (int i = 0; i < numCameras; i++)
    {
        std::array<long int, 4> roi = cameras_getROI(i);
        std::string guid = cameras_getGUID(i);

        defaultROI[guid.c_str()]["Width"] = (Json::Value::Int64)roi[0];
        defaultROI[guid.c_str()]["Height"] = (Json::Value::Int64)roi[1];
        defaultROI[guid.c_str()]["OffX"] = (Json::Value::Int64)roi[2];
        defaultROI[guid.c_str()]["OffY"] = (Json::Value::Int64)roi[3];
    }

    std::ofstream roiFile;
    roiFile.open("Config/defaultROI.json");
    roiFile << styledWriter.write(defaultROI);
    roiFile.close();

    return;
}

void getSingleROI()
{
    if (captureRunning)
    {
        alert_error("Capture is Running!");
        return;
    }

    std::string index(".entry_ROI_single" << get());
    std::array<long int, 4> res = cameras_getROI(std::stoi(index));

    ".entry_width" << deletetext(0, end);
    ".entry_width" << insert(0, std::to_string(res[0]));
    ".entry_height" << deletetext(0, end);
    ".entry_height" << insert(0, std::to_string(res[1]));
    ".entry_Xoff" << deletetext(0, end);
    ".entry_Xoff" << insert(0, std::to_string(res[2]));
    ".entry_Yoff" << deletetext(0, end);
    ".entry_Yoff" << insert(0, std::to_string(res[3]));

    return;
}

void setup()
{
    numCameras = cameras_connect();
    streamCam = numCameras - 1;
    cameras_start();

    frame(".main");
    frame(".config");
    // frame(".roi_settings") -bd(10);
    frame(".cameras") - bd(10);
    frame(".capture") - bd(10);
    frame(".stimulus") - bd(10);
    frame(".experiment") - bd(10);
    frame(".folderdata") - bd(3);
    frame(".postprocess0") - bd(10);
    frame(".postprocess1") - bd(1);
    frame(".postprocess2") - bd(2);
    frame(".postprocess3") - bd(2);
    frame(".postprocessImages") - bd(10);

    fonts(create, "title");
    fonts(configure, "title") - weight("bold") - size(11);

    fonts(create, "title2");
    fonts(configure, "title2") - weight("bold") - size(9);

    // // ROI configuration
    // label(".label_ROI") -text("ROI properties:") -justify("left") -font("title");
    // grid(configure,".label_ROI") -in(".roi_settings") -column(0) -row(0);

    // label(".label_width") -text("Width");
    // entry(".entry_width") -width(7);
    // grid(configure,".label_width") -in(".roi_settings") -column(0) -row(1); 
    // grid(configure,".entry_width") -in(".roi_settings") -column(1) -row(1);
    // label(".label_height") -text("Height");
    // entry(".entry_height") -width(7);
    // grid(configure,".label_height") -in(".roi_settings") -column(0) -row(2);
    // grid(configure,".entry_height") -in(".roi_settings") -column(1) -row(2);
    // label(".label_Xoff") -text("X Offset"); 
    // entry(".entry_Xoff") -width(7);
    // grid(configure,".label_Xoff") -in(".roi_settings") -column(0) -row(3);
    // grid(configure,".entry_Xoff") -in(".roi_settings") -column(1) -row(3);
    // label(".label_Yoff") -text("Y Offset");  
    // entry(".entry_Yoff") -width(7); 
    // grid(configure,".label_Yoff") -in(".roi_settings") -column(0) -row(4);
    // grid(configure,".entry_Yoff") -in(".roi_settings") -column(1) -row(4);

    // label(".label_empty") -text("      ");
    // grid(configure,".label_empty") -in(".roi_settings") -column(0) -row(5);

    // button(".button_apply_roi") -text("Apply ROI to All") -command(setAllROI);
    // grid(configure,".button_apply_roi") -in(".roi_settings") -column(4) -row(2);
    // button(".button_set_default_roi") -text("Save ROI setup as default")
    //                                   -command(setDefaultROI);
    // grid(configure, ".button_set_default_roi") -in(".roi_settings") -column(4) -row(3); 

    // label(".label_ROI_single") -text("Enter Camera Index:");
    // entry(".entry_ROI_single") -width(7);
    // grid(configure,".label_ROI_single") -in(".roi_settings") -column(0) -row(6);
    // grid(configure,".entry_ROI_single") -in(".roi_settings") -column(1) -row(6);

    // button(".button_apply_roi_single") -text("Apply ROI to one camera") -command(setSingleROI);
    // button(".button_read_roi")  -text("Read ROI from one camera") -command(getSingleROI); 
    // grid(configure,".button_apply_roi_single") -in(".roi_settings") -column(4) -row(6);
    // grid(configure,".button_read_roi")  -in(".roi_settings") -column(4) -row(7);
    // pack(".roi_settings");

    // Cameras selection
    label(".label_selec") - text("Select cameras:") - justify("left") - font("title");
    grid(configure, ".label_selec") - in(".cameras") - column(0) - row(0);

    label(".label_live") - text("Live\nStream") - font("title2");
    grid(configure, ".label_live") - in(".cameras") - column(1) - row(1);
    label(".label_save") - text("Capture\nSave") - font("title2");
    grid(configure, ".label_save") - in(".cameras") - column(2) - row(1);
    label(".label_capt") - text("Capture\nStream") - font("title2");
    grid(configure, ".label_capt") - in(".cameras") - column(3) - row(1);

    label(".label_cam0") - text("\tCamera 0");
    grid(configure, ".label_cam0") - in(".cameras") - column(0) - row(2);
    checkbutton(".check_liveStream0") - variable(liveStream[0]);
    grid(configure, ".check_liveStream0") - in(".cameras") - column(1) - row(2);
    checkbutton(".check_cam0") - variable(saveCams[0]);
    grid(configure, ".check_cam0") - in(".cameras") - column(2) - row(2);
    checkbutton(".check_captStream0") - variable(captStream[0]) - state("disabled")
                                      - command(toggle_capt_stream);
    grid(configure, ".check_captStream0") - in(".cameras") - column(3) - row(2);

    label(".label_cam1") - text("\tCamera 1");
    grid(configure, ".label_cam1") - in(".cameras") - column(0) - row(3);
    checkbutton(".check_liveStream1") - variable(liveStream[1]);
    grid(configure, ".check_liveStream1") - in(".cameras") - column(1) - row(3);
    checkbutton(".check_cam1") - variable(saveCams[1]);
    grid(configure, ".check_cam1") - in(".cameras") - column(2) - row(3);
    checkbutton(".check_captStream1") - variable(captStream[1]) - state("disabled")
                                      - command(toggle_capt_stream);
    grid(configure, ".check_captStream1") - in(".cameras") - column(3) - row(3);

    label(".label_cam2") - text("\tCamera 2");
    grid(configure, ".label_cam2") - in(".cameras") - column(0) - row(4);
    checkbutton(".check_liveStream2") - variable(liveStream[2]);
    grid(configure, ".check_liveStream2") - in(".cameras") - column(1) - row(4);
    checkbutton(".check_cam2") - variable(saveCams[2]);
    grid(configure, ".check_cam2") - in(".cameras") - column(2) - row(4);
    checkbutton(".check_captStream2") - variable(captStream[2]) - state("disabled")
                                      - command(toggle_capt_stream);
    grid(configure, ".check_captStream2") - in(".cameras") - column(3) - row(4);

    label(".label_cam3") - text("\tCamera 3");
    grid(configure, ".label_cam3") - in(".cameras") - column(0) - row(5);
    checkbutton(".check_liveStream3") - variable(liveStream[3]);
    grid(configure, ".check_liveStream3") - in(".cameras") - column(1) - row(5);
    checkbutton(".check_cam3") - variable(saveCams[3]);
    grid(configure, ".check_cam3") - in(".cameras") - column(2) - row(5);
    checkbutton(".check_captStream3") - variable(captStream[3]) - state("disabled")
                                      - command(toggle_capt_stream);
    grid(configure, ".check_captStream3") - in(".cameras") - column(3) - row(5);

    label(".label_cam4") - text("\tCamera 4");
    grid(configure, ".label_cam4") - in(".cameras") - column(0) - row(6);
    checkbutton(".check_liveStream4") - variable(liveStream[4]);
    grid(configure, ".check_liveStream4") - in(".cameras") - column(1) - row(6);
    checkbutton(".check_cam4") - variable(saveCams[4]);
    grid(configure, ".check_cam4") - in(".cameras") - column(2) - row(6);
    checkbutton(".check_captStream4") - variable(captStream[4]) - state("disabled")
                                      - command(toggle_capt_stream);
    grid(configure, ".check_captStream4") - in(".cameras") - column(3) - row(6);

    label(".label_cam5") - text("\tCamera 5");
    grid(configure, ".label_cam5") - in(".cameras") - column(0) - row(7);
    checkbutton(".check_liveStream5") - variable(liveStream[5]);
    grid(configure, ".check_liveStream5") - in(".cameras") - column(1) - row(7);
    checkbutton(".check_cam5") - variable(saveCams[5]);
    grid(configure, ".check_cam5") - in(".cameras") - column(2) - row(7);
    checkbutton(".check_captStream5") - variable(captStream[5]) - state("disabled")
                                      - command(toggle_capt_stream);
    grid(configure, ".check_captStream5") - in(".cameras") - column(3) - row(7);

    label(".label_cam6") - text("\tCamera 6");
    grid(configure, ".label_cam6") - in(".cameras") - column(0) - row(8);
    checkbutton(".check_liveStream6") - variable(liveStream[6]);
    grid(configure, ".check_liveStream6") - in(".cameras") - column(1) - row(8);
    checkbutton(".check_cam6") - variable(saveCams[6]);
    grid(configure, ".check_cam6") - in(".cameras") - column(2) - row(8);
    checkbutton(".check_captStream6") - variable(captStream[6]) - state("disabled")
                                      - command(toggle_capt_stream);
    grid(configure, ".check_captStream6") - in(".cameras") - column(3) - row(8);

    label(".label_empty2") - text("        ");
    grid(configure, ".label_empty2") - in(".cameras") - column(4) - row(4);

    button(".button_refresh") - text("Refresh") - command(refresh_cameras);
    grid(configure, ".button_refresh") - in(".cameras") - column(5) - row(4);

    pack(".cameras");

    // Capture Settings
    label(".label_capture") - text("Capture Settings:") - justify("left") - font("title");
    grid(configure, ".label_capture") - in(".capture") - column(0) - row(0);

    label(".label_opFlow") - text("\tUsing Optic Flow");
    grid(configure, ".label_opFlow") - in(".capture") - column(0) - row(1);
    checkbutton(".check_opFlow") - variable(opFlow);
    grid(configure, ".check_opFlow") - in(".capture") - column(1) - row(1);

    label(".label_behCams") - text("\tUsing Behavior Cams");
    grid(configure, ".label_behCams") - in(".capture") - column(0) - row(2);
    checkbutton(".check_behCams") - variable(behCams) - command(toggle_beh_cams);
    grid(configure, ".check_behCams") - in(".capture") - column(1) - row(2);

    label(".label_trigger") - text("\tEnable Trigger");
    grid(configure, ".label_trigger") - in(".capture") - column(0) - row(3);
    checkbutton(".check_trigger") - variable(trigger);
    grid(configure, ".check_trigger") - in(".capture") - column(1) - row(3);

    label(".label_ThorImage") - text("\tUsing ThorImage");
    grid(configure, ".label_ThorImage") - in(".capture") - column(0) - row(4);
    checkbutton(".check_ThorImage") - variable(thImage);
    grid(configure, ".check_ThorImage") - in(".capture") - column(1) - row(4);

    label(".label_Stimulus") - text("\tUsing Stimulation");
    grid(configure, ".label_Stimulus") - in(".capture") - column(0) - row(5);
    checkbutton(".check_Stimulus") - variable(stimulus) - command(toggle_stimulus);
    grid(configure, ".check_Stimulus") - in(".capture") - column(1) - row(5);

    label(".label_motion_activation") - text("\tEnable Motion Analysis");
    grid(configure, ".label_motion_activation") - in(".capture") - column(0) - row(6);
    checkbutton(".check_motion_activation") - variable(motion_activation)
                                            - command(toggle_motion_activation);
    grid(configure, ".check_motion_activation") - in(".capture") - column(1) - row(6);

    // label(".label_strobe") -text("\tEnable Strobe");
    // grid(configure,".label_strobe") -in(".capture") -column(0) -row(7);
    // checkbutton(".check_strobe") -variable(strobe);
    // grid(configure,".check_strobe") -in(".capture") -column(1) -row(7);

    label(".label_seperate_images") - text("\tSave Images Directly");
    grid(configure, ".label_seperate_images") - in(".capture") - column(0) - row(7);
    checkbutton(".check_seperate_images") - variable(seperate_images);
    grid(configure, ".check_seperate_images") - in(".capture") - column(1) - row(7);

    label(".label_empty3") - text("        ");
    grid(configure, ".label_empty3") - in(".capture") - column(2) - row(1);

    pack(".capture");

    //Stimulus labels

    label(".label_currentStim") - text("Current Stimulation Sequence") - state("disabled");
    grid(configure, ".label_currentStim") - in(".stimulus") - column(3) - row(1);

    textw(".currentStim") - state("disabled") - background("white") - yscrollcommand(".s set")
                          - width(25) - height(5);
    scrollbar(".s") - command(std::string(".currentStim yview"));
    grid(configure, ".currentStim") - in(".stimulus") - column(3) - row(2);
    grid(configure, ".s") - in(".stimulus") - column(4) - row(2) - sticky("e");

    button(".button_stimulation") - text("Upload sequence") - state("disabled")
                                  - command(new_stim_sequence);
    grid(configure, ".button_stimulation") - in(".stimulus") - column(3) - row(3);
    button(".button_editStim") - text("Edit stimulation sequence") - state("disabled")
                               - command(editSequence);
    grid(configure, ".button_editStim") - in(".stimulus") - column(3) - row(4);

    pack(".stimulus");

    //Experiment Settings

    label(".label_experiment") - text("Experiment Settings:") - justify("left") - font("title");
    grid(configure, ".label_experiment") - in(".experiment") - column(0) - row(0);

    label(".label_time_obs") - text("Max. Observation Time");
    grid(configure, ".label_time_obs") - in(".experiment") - column(0) - row(1);
    entry(".entry_time_obs") - width(8) - textvariable(stimTimeStr);
    grid(configure, ".entry_time_obs") - in(".experiment") - column(1) - row(1);
    radiobutton(".radio_sec_obs") - text("sec") - variable(obs_scale) - value(1);
    grid(configure, ".radio_sec_obs") - in(".experiment") - column(2) - row(1);
    radiobutton(".radio_min_obs") - text("min") - variable(obs_scale) - value(2);
    grid(configure, ".radio_min_obs") - in(".experiment") - column(3) - row(1);
    radiobutton(".radio_hrs_obs") - text("hrs") - variable(obs_scale) - value(3);
    grid(configure, ".radio_hrs_obs") - in(".experiment") - column(4) - row(1);

    label(".label_time_rec") - text("Max. Scene Time");
    grid(configure, ".label_time_rec") - in(".experiment") - column(0) - row(2);
    entry(".entry_time_rec") - state("disabled") - width(8);
    grid(configure, ".entry_time_rec") - in(".experiment") - column(1) - row(2);
    radiobutton(".radio_sec_rec") - text("sec") - variable(rec_scale) - value(1);
    grid(configure, ".radio_sec_rec") - in(".experiment") - column(2) - row(2);
    radiobutton(".radio_min_rec") - text("min") - variable(rec_scale) - value(2);
    grid(configure, ".radio_min_rec") - in(".experiment") - column(3) - row(2);
    radiobutton(".radio_hrs_rec") - text("hrs") - variable(rec_scale) - value(3);
    grid(configure, ".radio_hrs_rec") - in(".experiment") - column(4) - row(2);

    label(".label_FPS") - text("Frames per Second");
    grid(configure, ".label_FPS") - in(".experiment") - column(0) - row(3);
    entry(".entry_FPS") - width(8);
    grid(configure, ".entry_FPS") - in(".experiment") - column(1) - row(3);

    pack(".experiment");

    // Folders data

    label(".label_experimenter") - text("Experimenter");
    grid(configure, ".label_experimenter") - in(".folderdata") - column(0) - row(0);
    entry(".entry_experimenter") - width(8);
    grid(configure, ".entry_experimenter") - in(".folderdata") - column(1) - row(0);

    label(".label_genotype") - text("Genotype");
    grid(configure, ".label_genotype") - in(".folderdata") - column(2) - row(0);
    entry(".entry_genotype") - width(26);
    grid(configure, ".entry_genotype") - in(".folderdata") - columnspan(3) - column(3) - row(0);

    label(".label_fly") - text("Fly #");
    grid(configure, ".label_fly") - in(".folderdata") - column(0) - row(1);
    entry(".entry_fly") - width(5);
    grid(configure, ".entry_fly") - in(".folderdata") - column(1) - row(1);

    label(".label_type") - text("Type of imaging");
    grid(configure, ".label_type") - in(".folderdata") - column(2) - row(1);
    entry(".entry_type") - width(8);
    grid(configure, ".entry_type") - in(".folderdata") - column(3) - row(1);

    label(".label_trial") - text("Trial #");
    grid(configure, ".label_trial") - in(".folderdata") - column(4) - row(1);
    entry(".entry_trial") - width(5) - textvariable(trialStr);
    grid(configure, ".entry_trial") - in(".folderdata") - column(5) - row(1);

    //label(".label_empty4") -text("        ");
    //grid(configure,".label_empty4") -in(".folderdata") -column(2) -row(1);

    button(".button_start_capture") - text("Start Capture") - command(start_capture);
    grid(configure, ".button_start_capture") - in(".folderdata") - columnspan(2)
                                             - column(2) - row(2);

    pack(".folderdata");

    // Post Processing
    label(".label_video_set") - text("Video Settings:") - justify("left") - font("title");
    grid(configure, ".label_video_set") - in(".postprocess0") - column(0) - row(0);

    label(".label_getVideo") - text("Getting video from:");
    grid(configure, ".label_getVideo") - in(".postprocess0") - column(0) - row(1);

    label(".label_mostRecent") - text("Most recent experiment");
    grid(configure, ".label_mostRecent") - in(".postprocess0") - column(1) - row(1);
    checkbutton(".check_mostRecent") - variable(mostRecent) - command(toggle_most_recent);
    grid(configure, ".check_mostRecent") - in(".postprocess0") - column(2) - row(1);
    label(".label_select") - text("  Select experiment");
    grid(configure, ".label_select") - in(".postprocess0") - column(3) - row(1);
    checkbutton(".check_select") - variable(selectExp) - command(toggle_select_experiment);
    grid(configure, ".check_select") - in(".postprocess0") - column(4) - row(1);
    pack(".postprocess0");

    //Select location

    label(".label_working_directory_postproc") - text("\tSelected location:");
    grid(configure, ".label_working_directory_postproc") - in(".postprocess1")
                                                         - column(0) - row(2);
    entry(".entry_working_directory_postproc") - width(40) - state("disabled");
    grid(configure, ".entry_working_directory_postproc") - in(".postprocess1") - columnspan(3)
                                                         - column(1) - row(2);

    pack(".postprocess1");

    //check boxes

    label(".label_0") - text("0");
    grid(configure, ".label_0") - in(".postprocess2") - column(1) - row(2);
    label(".label_1") - text("1");
    grid(configure, ".label_1") - in(".postprocess2") - column(2) - row(2);
    label(".label_2") - text("2");
    grid(configure, ".label_2") - in(".postprocess2") - column(3) - row(2);
    label(".label_3") - text("3");
    grid(configure, ".label_3") - in(".postprocess2") - column(4) - row(2);
    label(".label_4") - text("4");
    grid(configure, ".label_4") - in(".postprocess2") - column(5) - row(2);
    label(".label_5") - text("5");
    grid(configure, ".label_5") - in(".postprocess2") - column(6) - row(2);
    label(".label_6") - text("6");
    grid(configure, ".label_6") - in(".postprocess2") - column(7) - row(2);
    label(".label_all") - text("All");
    grid(configure, ".label_all") - in(".postprocess2") - column(8) - row(2);

    label(".label_video_for") - text("Video for camera:\t");
    grid(configure, ".label_video_for") - in(".postprocess2") - column(0) - row(3);
    checkbutton(".check_0") - variable(videofrom[0]) - state("disabled");
    grid(configure, ".check_0") - in(".postprocess2") - column(1) - row(3);
    checkbutton(".check_1") - variable(videofrom[1]) - state("disabled");
    grid(configure, ".check_1") - in(".postprocess2") - column(2) - row(3);
    checkbutton(".check_2") - variable(videofrom[2]) - state("disabled");
    grid(configure, ".check_2") - in(".postprocess2") - column(3) - row(3);
    checkbutton(".check_3") - variable(videofrom[3]) - state("disabled");
    grid(configure, ".check_3") - in(".postprocess2") - column(4) - row(3);
    checkbutton(".check_4") - variable(videofrom[4]) - state("disabled");
    grid(configure, ".check_4") - in(".postprocess2") - column(5) - row(3);
    checkbutton(".check_5") - variable(videofrom[5]) - state("disabled");
    grid(configure, ".check_5") - in(".postprocess2") - column(6) - row(3);
    checkbutton(".check_6") - variable(videofrom[6]) - state("disabled");
    grid(configure, ".check_6") - in(".postprocess2") - column(7) - row(3);
    checkbutton(".check_all") - variable(videofrom[7]) - command(toggle_all);
    grid(configure, ".check_all") - in(".postprocess2") - column(8) - row(3);

    pack(".postprocess2");

    //Buttons videos

    button(".button_batch") - text("Add to batch") - command(addBatch_button);
    grid(configure, ".button_batch") - in(".postprocess3") - column(0) - row(0);
    button(".button_see") - text("Open batch") - command(seeBatch);
    grid(configure, ".button_see") - in(".postprocess3") - column(0) - row(1);
    label(".label_empty4") - text("\t");
    grid(configure, ".label_empty4") - in(".postprocess3") - column(1) - row(0);
    button(".button_video") - text("Make video") - command(makeVideo);
    grid(configure, ".button_video") - in(".postprocess3") - column(2) - row(0);

    pack(".postprocess3");

    // Post Processing Images
    label(".label_postproc") - text("Getting images from temp file:") - justify("left")
                             - font("title");
    grid(configure, ".label_postproc") - in(".postprocessImages") - column(0) - row(0);

    label(".label_working_directory_postproc_images") - text("\tFolder Name:");
    grid(configure, ".label_working_directory_postproc_images") - in(".postprocessImages")
                                                                - column(0) - row(1);
    entry(".entry_working_directory_postproc_images") - width(30);
    grid(configure, ".entry_working_directory_postproc_images") - in(".postprocessImages")
                                                                - column(1) - row(1);
    button(".button_process_images") - text("Post Process Images") - command(process);
    grid(configure, ".button_process_images") - in(".postprocessImages") - columnspan(2)
                                              - column(0) - row(2);

    pack(".postprocessImages");

    // Putting it all together
    // grid(configure, ".roi_settings") -in(".config") -column(0) -row(0) -sticky("w")
    //                                  -columnspan(2);
    grid(configure, ".cameras") - in(".config") - column(0) - row(1) - sticky("w")
                                - columnspan(2);
    grid(configure, ".capture") - in(".config") - column(0) - row(2) - sticky("w");
    grid(configure, ".stimulus") - in(".config") - column(1) - row(2) - sticky("w");
    grid(configure, ".experiment") - in(".config") - column(0) - row(3) - sticky("w")
                                   - columnspan(2);
    grid(configure, ".folderdata") - in(".config") - column(0) - row(4) - sticky("w")
                                   - columnspan(2);
    grid(configure, ".postprocess0") - in(".config") - column(0) - row(5) - sticky("w")
                                     - columnspan(2);
    grid(configure, ".postprocess1") - in(".config") - column(0) - row(6) - sticky("w")
                                     - columnspan(2);
    grid(configure, ".postprocess2") - in(".config") - column(0) - row(7) - sticky("n")
                                     - columnspan(2);
    grid(configure, ".postprocess3") - in(".config") - column(0) - row(8) - sticky("s")
                                     - columnspan(2);
    grid(configure, ".postprocessImages") - in(".config") - column(0) - row(9) - sticky("w")
                                          - columnspan(2);

    pack(".config");
    grid(configure, ".config") - in(".main") - column(0) - row(0);
    pack(".main");
}

int main()
{
    setup();
    camsStatus();
    std::cout << "Setup Success!" << std::endl;
    updateFrameView();
    runEventLoop();
    cameras_disconnect();
    return 0;
}
