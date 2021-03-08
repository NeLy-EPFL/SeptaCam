#include "gui.h"

/** GUI helper function for displaying messages 
 *  @param msg The message we wish to display in the popup window
 */
void gui::alert_error(std::string msg)
{
    tk_messageBox() -defaultbutton("ok") -messagetext(msg) -messagetype(ok);
}

/** GUI helper function for displaying converting time read from GUI into seconds
 *  @param t Numerical value for time 
 *  @param param Numerical index for determining units of first argument  
 */
float gui::convert_time(float t, int param)
{
    float secTime = 0;
    switch(param)
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

/** GUI update function. Periodically calls f_grab.cameras_grab for getting a single 
 *    set of frames. Frequency of calls depends on DISPLAY_FPS constant
 *  
 * @see cameras_grab
 * @see DISPLAY_FPS 
 * 
 */
void gui::update_frame_view()
{
    if (!this->capture_running)
    {
        this->f_grab->cameras_grab("Not Applicable",0,0,false,false,false,false);
    }
    after(1.0/DISPLAY_FPS*1000-5,this->update_frame_view);
    return;
} 

/** GUI's wrapper for calling frame_grabber.cameras_grab method 
 *
 *  @see start_capture_t
 *  @see frame_grabber.cameras_grab
 *  @param arg Not used - serves to match the (void*)f(void* arg) 
 *             pattern needed by pthread_create
 *  @return NULL
 *
 */
void* gui::start_capture(void* arg)
{
    capture_running = true; 

    std::string string_time_rec(".entry_time_rec" << Tk::get());
    std::string string_time_obs(".entry_time_obs" << Tk::get());
    std::string string_fps(".entry_FPS"<< Tk::get() );
    std::string working_dir(".entry_working_directory" << Tk::get());

    working_dir.insert(0,OUTPUT_DIR);

    float fps          = std::stof(string_fps);
    float fl_rec_time  = motion_activation ? std::stof(string_time_rec):std::stof(string_time_obs); 
    float fl_obs_time  = std::stof(string_time_obs); 
    float rec_time_sec = this->convert_time(fl_rec_time, rec_scale);
    float obs_time_sec = this->convert_time(fl_obs_time, obs_scale);
    int rec_frames     = (int)(fps*rec_time_sec);
    int obs_frames     = (int)(fps*obs_time_sec);

    if (obs_frames < 1)
    {
        alert_error("Invalid Parameters");
        return NULL; 
    }

    this->f_grab->cameras_stop(); 
    this->f_grab->cameras_setFPS(fps, trigger); 
    this->f_grab->cameras_start();
    this->f_grab->cameras_grab(working_dir,obs_frames,rec_frames, motion_activation, seperate_images, trigger,strobe); 
    this->f_grab->cameras_stop(); 
    this->f_grab->cameras_setFPS(30,false); 
    this->f_grab->cameras_start();

    alert_error("Capture Completed");
    capture_running = false;
    return NULL;
}

/** GUI's wrapper for spawing new thread executing image capture 
 *  
 *  @see process
 *  @see cameras_save
 *  
 */
void gui::start_capture_t()
{
    if (this->capture_running)
    {
        alert_error("Capture is running");
        return;
    }
    if (this->postprocess_running)
    {
        alert_error("Postprocessing Running");
        return; 
    }

    pthread_t t; 
    pthread_create(&t, NULL, this->start_capture, NULL);
    return; 
}

/** GUI's wrapper for calling f_grab.cameras_save method 
 *  
 *  @see process_t
 *  @see cameras_save
 *  @param arg Not used - serves to match the (void*)f(void* arg) 
 *             pattern needed by pthread_create
 *  @return NULL
 */
void* gui::process(void* arg)
{
    
    std::string working_dir(".entry_working_directory_postproc" << Tk::get());
    working_dir.insert(0,OUTPUT_DIR);
    this->f_grab->cameras_save(working_dir);
    alert_error("Post-Processing Completed");
    return NULL; 
}

/** GUI's wrapper for spawing new thread executing postprocessing
 *  
 *  @see process
 *  @see frame_grabber.cameras_save
 *  
 */
void gui::process_t()
{
    if(this->capture_running)
    {
        alert_error("Capture is Running!");
        return; 
    }
    if(this->postprocess_running)
    {
        alert_error("Postprocessing Running");
        return; 
    }
    pthread_t t; 
    pthread_create(&t, NULL, process, NULL);
    return; 
}


/** GUI method for deactivating the "Recording Time" field  
 *  This method is called when the motion_activation variable is toggled
 *  When motion_activation is deactivated, the recording time is  equal 
 *  to the recording time 
 *
 * @see motion_activation
 *
 */ 
void gui::toggle_motion_activation()
{
    if (this->capture_running)
    {
        alert_error("Capture is running");
        return;
    }
    if (this->postprocess_running)
    {
        alert_error("Postprocessing Running");
        return; 
    }
    if(motion_activation)
    {
        ".entry_time_rec" << configure() -state("normal"); 
    }
    else 
    {
        ".entry_time_rec" << configure() -state("disabled");
    }
}



void gui::set_all_ROI()
{ 
    if(this->capture_running)
    {
        alert_error("Capture is Running!");
        return; 
    }

    std::string height(".entry_height" << Tk::get());
    std::string width(".entry_width" << Tk::get());
    std::string offsetX(".entry_Xoff" << Tk::get());
    std::string offsetY(".entry_Yoff" << Tk::get()); 
    this->f_grab->cameras_stop();
    this->f_grab->cameras_setROI(std::stoi(width),std::stoi(height),
             std::stoi(offsetX),std::stoi(offsetY));
    this->f_grab->cameras_start();
    return;
}

void gui::set_single_ROI()
{ 
     
    if(this->capture_running)
    {
        alert_error("Capture is Running!");
        return; 
    }

    std::string height(".entry_height" << Tk::get());
    std::string width(".entry_width"   << Tk::get());
    std::string offsetX(".entry_Xoff"  << Tk::get());
    std::string offsetY(".entry_Yoff"  << Tk::get()); 
    std::string index(".entry_ROI_single" << Tk::get());

    if (std::stoi(index)>= this->f_grab->num_devices)
    {
        alert_error("Invalid Camera Index"); 
    }

    this->f_grab->cameras_stop();
    this->f_grab->cameras_setROI(std::stoi(width),std::stoi(height),
             std::stoi(offsetX),std::stoi(offsetY),std::stoi(index));
    this->f_grab->cameras_start();
    return;
}

void gui::set_default_ROI()
{
    Json::Value        default_ROI;
    Json::StyledWriter styled_writer;

    for (int i=0; i<f_grab->num_devices; i++)
    { 
        std::array<long int,4> roi = this->f_grab->cameras_getROI(i); 
        std::string guid = this->f_grab->cameras_getGUID(i); 

        default_ROI[guid.c_str()]["Width"]  = (Json::Value::Int64) roi[0];
        default_ROI[guid.c_str()]["Height"] = (Json::Value::Int64) roi[1];
        default_ROI[guid.c_str()]["OffX"]   = (Json::Value::Int64) roi[2];
        default_ROI[guid.c_str()]["OffY"]   = (Json::Value::Int64) roi[3];
    }

    std::ofstream  roi_file;
    std::string    roi_file_name("Config/defaultROI.json");
    roi_file_name.insert(0,INSTALL_DIR);
    roi_file.open(roi_file_name.c_str());
    roi_file << styled_writer.write(default_ROI); 
    roi_file.close(); 

    return; 
}

void gui::get_single_ROI()
{
    if(this->capture_running)
    {
        alert_error("Capture is Running!");
        return; 
    }

    string index(".entry_ROI_single" << Tk::get());

    if (stoi(index) >= this->f_grab->num_devices || stoi(index) < 0 ){
        alert_error("Invalid Camera Index"); 
        return;
    }

    array<long int,4> res = f_grab->cameras_getROI(std::stoi(index)); 

    ".entry_width"  << deletetext(0,Tk::end);
    ".entry_width"  << insert(0, std::to_string(res[0]));
    ".entry_height" << deletetext(0,Tk::end);
    ".entry_height"  << insert(0, std::to_string(res[1]));
    ".entry_Xoff"   << deletetext(0,Tk::end);
    ".entry_Xoff"   << insert(0, std::to_string(res[2])); 
    ".entry_Yoff"   << deletetext(0,Tk::end);
    ".entry_Yoff"   << insert(0, std::to_string(res[3])); 

    return; 
}
 

gui::gui()
{
    capture_running     = false;
    postprocess_running = false;
    trigger             = false; 
    strobe              = false;
    seperate_images     = false; 
    motion_activation   = true; 
    obs_scale           = 1;
    rec_scale           = 1;

    this->f_grab = new frame_grabber(); 
    this->f_grab->cameras_start();

    frame(".main");
    frame(".config");
    frame(".roi_settings") -bd(20);
    frame(".capture")      -bd(20);
    frame(".postprocess")  -bd(20);     

    // ROI configuration
    label(".label_ROI") -text("Set ROI Here:"); 
    grid(configure,".label_ROI") -in(".roi_settings")  -row(0) -column(0) -columnspan(2);
    label(".label_width") -text("Width");
    entry(".entry_width");
    grid(configure,".label_width") -in(".roi_settings") -column(0) -row(1); 
    grid(configure,".entry_width") -in(".roi_settings") -column(1) -row(1);
    label(".label_height") -text("Height");
    entry(".entry_height");
    grid(configure,".label_height") -in(".roi_settings") -column(0) -row(2);
    grid(configure,".entry_height") -in(".roi_settings") -column(1) -row(2);
    label(".label_Xoff") -text("X Offset"); 
    entry(".entry_Xoff");
    grid(configure,".label_Xoff") -in(".roi_settings") -column(0) -row(3);
    grid(configure,".entry_Xoff") -in(".roi_settings") -column(1) -row(3);
    label(".label_Yoff") -text("Y Offset");  
    entry(".entry_Yoff"); 
    grid(configure,".label_Yoff") -in(".roi_settings") -column(0) -row(4);
    grid(configure,".entry_Yoff") -in(".roi_settings") -column(1) -row(4);
    button(".button_apply_roi") -text("Apply ROI to All") -command(this->set_all_ROI);
    grid(configure,".button_apply_roi") -in(".roi_settings")  -row(5) -column(2);

    button(".button_set_default_roi") -text("Save ROI setup as default") -command(this->set_default_ROI); 
    grid(configure, ".button_set_default_roi") -in(".roi_settings") -row(5) -column(3); 

    label(".label_ROI_single") -text("Enter Camera Index:");
    entry(".entry_ROI_single");
    grid(configure,".label_ROI_single") -in(".roi_settings") -row(6) -column(0);
    grid(configure,".entry_ROI_single") -in(".roi_settings") -row(6) -column(1);
    button(".button_apply_roi_single") -text("Apply ROI to one camera") -command(this->set_single_ROI); 
    button(".button_read_roi")  -text("Read ROI from one camera") -command(this->get_single_ROI); 
    grid(configure,".button_apply_roi_single") -in(".roi_settings") -row(6) -column(2);
    grid(configure,".button_read_roi")  -in(".roi_settings") -row(6) -column(3);
    pack(".roi_settings");   
        
    // Capture Settings
    label(".label_capture") -text("Set Capture Settings Here: "); 
    grid(configure,".label_capture") -in(".capture") -columnspan(2) -column(0) -row(0); 

    label(".label_trigger") -text("Enable Trigger"); 
    grid(configure,".label_trigger") -in(".capture") -column(0) -row(1);
    checkbutton(".check_trigger") -variable(this->trigger);
    grid(configure,".check_trigger") -in(".capture") -column(1) -row(1);
    label(".label_strobe") -text("Enable Strobe"); 
    grid(configure,".label_strobe") -in(".capture") -column(2) -row(1); 
    checkbutton(".check_strobe") -variable(this->strobe);
    grid(configure,".check_strobe") -in(".capture") -column(3) -row(1); 
    label(".label_motion_activation") -text("Enable Motion Actived Recording");

    grid(configure,".label_motion_activation") -in(".capture") -column(0) -columnspan(3) -row(2);
    checkbutton(".check_motion_activation") -variable(this->motion_activation) -command(this->toggle_motion_activation);
    grid(configure,".check_motion_activation") -in(".capture") -column(3) -row(2);

    label(".label_time_obs") -text("Max. Observation Time");
    grid(configure,".label_time_obs") -in(".capture") -column(0) -row(3); 
    entry(".entry_time_obs") ;
    grid(configure,".entry_time_obs") -in(".capture") -column(1) -row(3); 
    radiobutton(".radio_sec_obs") -text("sec") -variable(this->obs_scale) -value(1);
    grid(configure,".radio_sec_obs") -in(".capture") -column(2) -row(3);
    radiobutton(".radio_min_obs") -text("min") -variable(this->obs_scale) -value(2);
    grid(configure,".radio_min_obs") -in(".capture") -column(3) -row(3);
    radiobutton(".radio_hrs_obs") -text("hrs") -variable(this->obs_scale) -value(3);
    grid(configure,".radio_hrs_obs") -in(".capture") -column(4) -row(3);
    
    label(".label_time_rec") -text("Max. Recording Time"); 
    grid(configure,".label_time_rec") -in(".capture") -column(0) -row(4);
    entry(".entry_time_rec");
    grid(configure,".entry_time_rec") -in(".capture") -column(1) -row(4);
    radiobutton(".radio_sec_rec") -text("sec") -variable(this->rec_scale) -value(1);
    grid(configure,".radio_sec_rec") -in(".capture") -column(2) -row(4);
    radiobutton(".radio_min_rec") -text("min") -variable(this->rec_scale) -value(2);
    grid(configure,".radio_min_rec") -in(".capture") -column(3) -row(4);
    radiobutton(".radio_hrs_rec") -text("hrs") -variable(this->rec_scale) -value(3);
    grid(configure,".radio_hrs_rec") -in(".capture") -column(4) -row(4);


    label(".label_FPS") -text("Frames per Second"); 
    grid(configure,".label_FPS") -in(".capture") -column(0) -row(5);
    entry(".entry_FPS");
    grid(configure,".entry_FPS") -in(".capture") -column(1) -row(5);

    label(".label_working_directory") -text("Folder Name"); 
    grid(configure,".label_working_directory") -in(".capture") -column(0) -row(6); 
    entry(".entry_working_directory"); 
    grid(configure,".entry_working_directory") -in(".capture") -column(1) -row(6);

    button(".button_start_capture") -text("Start Capture") -command(this->start_capture_t); 
    grid(configure,".button_start_capture") -in(".capture") -columnspan(3) -column(2) -row(7);
    label(".label_seperate_images") -text("Seperate Images"); 
    grid(configure,".label_seperate_images") -in(".capture") -column(0) -row(7);
    checkbutton(".check_seperate_images") -variable(this->seperate_images);
    grid(configure,".check_seperate_images") -in(".capture") -column(1) -row(7);

    pack(".capture");

    // Post Processing 
    label(".label_postproc") -text("Set Post-Processing Settings Here:");
    grid(configure, ".label_postproc") -in(".postprocess")  -columnspan(2) -column(0) -row(0); 
    label(".label_working_directory_postproc") -text("Folder Name");
    grid(configure, ".label_working_directory_postproc") -in(".postprocess") -column(0) -row(4); 
    entry(".entry_working_directory_postproc"); 
    grid(configure, ".entry_working_directory_postproc") -in(".postprocess") -column(1) -row(4); 
    button(".button_process_images") -text("Post Process") -command(this->process_t);  
    grid(configure,".button_process_images") -in(".postprocess") -columnspan(2) -column(0) -row(5);

    pack(".postprocess");

    // Putting it all together
    grid(configure, ".roi_settings") -in(".config") -column(0) -row(0);
    grid(configure, ".capture")      -in(".config") -column(0) -row(1);
    grid(configure, ".postprocess")  -in(".config") -column(0) -row(2);

    pack(".config"); 
    grid(configure,".config") -in(".main") -column(0) -row(0);
    pack(".main");

    std::cout << "Setup Success!" << std::endl;

    update_frame_view(); 
    runEventLoop();
}

gui::~gui()
{
    delete f_grab;
}

int main()
{
    gui my_gui();  
    return 0; 
}
