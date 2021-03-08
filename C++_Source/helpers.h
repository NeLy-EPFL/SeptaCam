#include <iostream>

#include <string.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


using namespace std;

void assignPriority(int tPriority)
{
    int  policy;
    struct sched_param param;

    pthread_getschedparam(pthread_self(), &policy, &param);
    param.sched_priority = tPriority;

    if(pthread_setschedparam(pthread_self(), SCHED_RR, &param))
    {
         cout << "error while setting thread priority to " << tPriority << endl;
    }
}


int64_t Adjust(int64_t val, int64_t minimum, int64_t maximum, int64_t inc)
{
    if (inc <= 0)
    {
        throw LOGICAL_ERROR_EXCEPTION("Unexpected increment %d", inc);
    }
    if (minimum > maximum)
    {
        throw LOGICAL_ERROR_EXCEPTION("minimum bigger than maximum.");
    }
    if (val < minimum)
    {
        return minimum;
    }
    if (val > maximum)
    {
        return maximum;
    }
    if (inc == 1)
    {
        return val;
    }
    else
    {
        return minimum + ( ((val - minimum) / inc) * inc );
    }
}

void createPreview(int numCameras, int numFrames, std::string path)
{

	return;

	ostringstream sstream1, sstream2, sstream3, sstream4; 
	string command1, command2, command3, command4;

	sstream1 << "cd " << path.c_str(); 
	system(sstream1.str().c_str());


	for (int i=0; i<numFrames;i++)
	{
		stringstream sstream; 
		string command;
		sstream << "ffmpeg -y -hide_banner -loglevel panic -i camera_%d_img_"            
				<< to_string(i) << ".png -filter_complex tile=" << to_string(numCameras) 
				<< "x1:margin=4:padding=4 merged_frame_" << to_string(i) << ".png" ;
		system(sstream.str().c_str());

	}

	sstream2 << "ffmpeg -y -hide_banner -loglevel panic -framerate 30 -start_number 0 -i merged_frame_%d.png preview.mp4"; 
	system(sstream2.str().c_str());

	sstream3 << "rm merged_frame_*.png";
	system(sstream3.str().c_str());

	sstream4 << "cd .."; 
	system(sstream4.str().c_str());
}

void primeTrigger(int fps)
{

	// Scan /dev/serial/by-id/ to find USB arduino

	string data;
	FILE * stream;
	int max_buffer = 1024;
	char buf[max_buffer];

	string cmd("ll /dev/serial/by-id/"); 
	cmd.append(" 2>&1");
	stream = popen(cmd.c_str(), "r");
	
	if (stream) 
	{
		while (!feof(stream))
		{
			if (fgets(buf, max_buffer, stream) != NULL) 
			{
				data.append(buf);
			}	
		}
	}
	pclose(stream);

	size_t   pos_name      = data.find("Arduino"); 
	data.erase(0,pos_name); 
	size_t   end_of_line   = data.find("\n"); 
	size_t 	 start_of_name = data.rfind("/");

	string usb_name = data.substr(start_of_name+1, end_of_line-start_of_name-1);  
	usb_name.insert(0, "/dev/");

	// Once USB Arduino is found, connect to it 

	cout << "Connecting to:" << usb_name.c_str() << endl;

	int USB = open(usb_name.c_str(), O_RDWR & ~O_NONBLOCK);
	if ( USB < 0 )
	{
		cout << "Error connecting to Arduino Board" << endl;
		return;  
	}

	struct termios SerialPortSettings;
	tcgetattr(USB, &SerialPortSettings);
	cfsetispeed(&SerialPortSettings,B9600);
	cfsetospeed(&SerialPortSettings,B9600);
	SerialPortSettings.c_cflag &= ~PARENB;
	SerialPortSettings.c_cflag &= ~CSTOPB; 
	SerialPortSettings.c_cflag &= ~CSIZE; 
	SerialPortSettings.c_cflag |=  CS8;
	SerialPortSettings.c_cflag &= ~CRTSCTS;
	SerialPortSettings.c_cflag |= CREAD | CLOCAL;
	SerialPortSettings.c_cc[VMIN]  =   0; 
	SerialPortSettings.c_cc[VTIME] =   0;
	tcflush( USB, TCIFLUSH );
	tcsetattr(USB,TCSANOW,&SerialPortSettings);

	string fps_string =  string(to_string((int)fps));
	char* buffer = (char*) malloc(fps_string.length()); 
	
	strncpy(buffer, fps_string.c_str(), fps_string.length());

	if (fps_string.length()!=write( USB, buffer, fps_string.length()))
	{
		cerr << "Failure on writing to USB - Err: "
			 << strerror(errno)  << endl; 
		return;

	}
	free(buffer);

	char ret;

	while (read(USB, (void*)&ret,1)!= 0)
	{
    	fprintf(stderr,"%c",ret);
	}
	fprintf(stderr, "\n");

	close(USB);

	cout << "Trigger Primed :" << buffer <<" "
		 << fps_string.length() << endl;

	return; 
}

