#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbInstantCamera.h>
#include <pylon/usb/BaslerUsbInstantCameraArray.h>

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include "NIR_Imaging.h"

using namespace std;
using namespace Pylon;
using namespace Basler_UsbCameraParams;
using namespace GenApi;


static int numDevices; 
static volatile int height;
static volatile int width; 
static volatile float fps;

static const float displayFPS = 10.0; 

const int ciMaxImageSize = 2048 * 2048;
static const int maxNumDevices = 10; 
static bool performanceMode = false; 
static void** display_data = NULL;

static const int window_height = 400;

static CBaslerUsbInstantCameraArray cameras;  
static const char defaultConfig[] = "C++_Source/Basler_Default_Config.pfs";

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

void setPerformanceMode(bool onOff)
{	
	performanceMode = onOff; 
	cout << "Performance Mode :" << onOff << endl; 
}

void setMetadata(bool onOff)
{
	try 
	{
		for (int i=0; i<numDevices; i++)
		{
			INodeMap& nodemap = cameras[i].GetNodeMap(); 
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
	catch (const GenericException & e)
	{
		cerr << "Exeception on setFPS(). Reason: "
 		 << e.GetDescription() << endl;
	}
	return; 
}

int cameras_connect()
{
	// Initialize Plyon Library
	try{
		PylonInitialize();
	  
 	 	DeviceInfoList_t lstDevices;
	  	CTlFactory& TlFactory = CTlFactory::GetInstance();
		numDevices = TlFactory.EnumerateDevices(lstDevices);

		cameras.Initialize(numDevices);  
		cout << numDevices << " cameras detected " << endl; 
		display_data = (void**) calloc(numDevices,sizeof(void*));
	
		// Set Cameras to default Configuation, create viewing windows
		for (size_t i=0; i < numDevices ; ++i)
		{
			display_data[i] = malloc(ciMaxImageSize);
			if ( display_data[i] == NULL ) 
			{
				cout << "Malloc Error" << endl; 
				return -1; 
			}

			cameras[i].Attach(TlFactory.CreateDevice(lstDevices[i]));
			cameras[i].Open();
			CFeaturePersistence::Load(defaultConfig, &cameras[i].GetNodeMap(), true); 

			ostringstream intString;
    		intString << "Camera_" << i;
    		cv::namedWindow(intString.str(), cv::WINDOW_NORMAL);
		}
	
		// Set Static State Variables 
		CIntegerPtr _width (cameras[0].GetNodeMap().GetNode("Width"));
		CIntegerPtr _height(cameras[0].GetNodeMap().GetNode("Height"));
		width = _width->GetValue(); 
		height = _height->GetValue();
	}
	catch  (const GenericException & e)
	{
		cerr << "Exeception on connect(). Reason: "
 		 << e.GetDescription() << endl;
	}

	//setSynchronization(false,false);
	setPerformanceMode(false);
	
	return numDevices; 
}

void cameras_disconnect() 
{
	try
	{
		cameras.Close();
		PylonTerminate(); 
	}
	catch (const GenericException & e)
	{
		cerr << "Exeception on disconnect(). Reason: "
 		 << e.GetDescription() << endl;
	}
}

void cameras_start()
{
	try 
	{	
		for (int i=0; i<1; i++)
		{
			if (performanceMode)
			{
				cameras.StartGrabbing( GrabStrategy_OneByOne); 
			}
			else 
			{
				cameras.StartGrabbing( GrabStrategy_LatestImageOnly);	
			}
		}	
	}
	catch (const GenericException & e)
	{
		cerr << "Exeception on start(). Reason: "
 		 << e.GetDescription() << endl;
	}

	cout << "Cameras Started" << endl;
}

void cameras_stop()
{
	try 
	{
		for (int i=0; i<1; i++)
		{
			cameras.StopGrabbing(); 
		}
	}
	catch (const GenericException & e)
	{
		cerr << "Exeception on stop(). Reason: "
 		 << e.GetDescription() << endl;
	}

	cout << "Cameras Stopped" << endl;
}

void cameras_grab(std::string path, int numImages, bool directSave)
{	
	try
	{
		CBaslerUsbGrabResultPtr ptrGrabResult; 

		// We want to save a determined number of frames
		if (numImages != 0)
		{	

			int frameCountArr[numDevices];
			int missedFrameAcc = 0;

			unsigned int tickArr[numDevices]; 
			float intervalAcc = 0; 

			cameras_stop();
			setPerformanceMode(true);
			setMetadata(true);
			cameras_start();


			if (!directSave)
			{
				FILE** arhFile =(FILE**) malloc(sizeof(FILE*)*numDevices);
	
				for (int i=0; i<numDevices;i++)
				{
					stringstream sstream; 
					string tempFilename; 
					sstream << path << "/camera_" << i << ".tmp";
					sstream >> tempFilename; 
					arhFile[i] = fopen(tempFilename.c_str(), "w+");
				}
	
				for (int i =0; i < numImages; i++ )
				{
					for (int j=0; j<numDevices ; j++)
					{
						if( cameras[j].RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException)
						   && ptrGrabResult->GrabSucceeded() )
						{	
							void* data = ptrGrabResult->GetBuffer();
	 
							if (data == NULL)
							{
								cerr << "Null pointer for grabbed data" << endl; 
								for (int ind=0; ind<numDevices;ind++)
								{
									fclose(arhFile[ind]);
								}
								free(arhFile);
								return;
							}
	
							//Check Metatdata
							unsigned int ticks   =  ptrGrabResult->ChunkTimestamp.GetValue();
							int frameCount       =  ptrGrabResult->GetBlockID();
	
							if (i != 0)
							{
								intervalAcc += ((float)(ticks - tickArr[j]))/1000000000;
								if (frameCount == UINT64_MAX )
								{
									cerr << "bad count" << endl;
								}

								if ((frameCount - frameCountArr[j])!=1)
								{
									missedFrameAcc += 1; 
								}
							}

							tickArr[j]        = ticks;
							frameCountArr[j]  = frameCount; 
							
							int written = fwrite(data,1,height*width,arhFile[j]); 
	
							if (written != (height * width)) 
							{
								cerr << "Failure on writing to disk buffer" << endl; 
								for (int ind=0; ind<numDevices;ind++)
								{
									fclose(arhFile[ind]);
								}
								free(arhFile);
								return; 
							}
	    	
							if( fps <= displayFPS || i %((int)(fps/displayFPS)) == 0)
							{
								memcpy(display_data[j],data, height*width);
								const cv::Mat M(height,width,CV_8U,display_data[j]);
								ostringstream intString;
		    					intString << "Camera_" << j;
								cv::imshow(intString.str(),M); 
								cv::resizeWindow(intString.str(), (width*window_height)/height,window_height);
								cv::waitKey(1);
							}		
						}	
						else
						{
							cerr << "Failure on Grab" << endl; 
							for (int ind=0; ind<numDevices;ind++)
							{
								fclose(arhFile[ind]);
							}
							free(arhFile);
							return; 
						}
					}
				}

				for (int i=0; i<numDevices;i++)
				{
					fclose(arhFile[i]);
				}
				free(arhFile);
			}

			// Direct Save Enabled
			else
			{
				for (int i =0; i < numImages; i++ )
				{
					for (int j =0; j < numDevices; j++)
					{
						if( cameras[j].RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException)
						   && ptrGrabResult->GrabSucceeded() )
						{	
							void* data = ptrGrabResult->GetBuffer();

							// Check Metadata
							unsigned int ticks   =  ptrGrabResult->ChunkTimestamp.GetValue();
							int frameCount       =  ptrGrabResult->GetBlockID();
	
							if (i != 0)
							{
								intervalAcc += ((float)(ticks - tickArr[j]))/1000000000;
								if ((frameCount - frameCountArr[j])!=1)
								{
									missedFrameAcc += 1; 
								}
							}
							tickArr[j]        = ticks;
							frameCountArr[j]  = frameCount; 
							
	
							stringstream sstream; 
							string tempFilename; 
							sstream << path << "/camera_" << j << "_img_" << i << ".png";
							sstream >> tempFilename; 
					
							CImagePersistence::Save(ImageFileFormat_Png,tempFilename.c_str(), 
								(const void*)data,height*width,PixelType_Mono8,width,height,0, 
								ImageOrientation_TopDown);
							
							if( fps <= displayFPS || i %((int)(fps/displayFPS)) == 0)
							{
								memcpy(display_data[j],data, height*width);
								const cv::Mat M(height,width,CV_8U,display_data[j]);
								ostringstream intString;
			    				intString << "Camera_" << j;
								cv::imshow(intString.str(),M); 
								cv::resizeWindow(intString.str(), (width*window_height)/height,window_height);
								cv::waitKey(1);
							}
						}
						else
						{
							cerr << "Failure on Grab" << endl; 
							return; 
						}
					}
				}
			}

			cameras_stop();
			cout << "Total interval :"<< intervalAcc << endl ;
			cout << "Average FPS: " << 1/((intervalAcc/numDevices)/(numImages-1)) << endl;
			cout << "We have missed a total of "<< missedFrameAcc << " frames" <<endl; 
			setPerformanceMode(false);
			setMetadata(false);
			cameras_start();

		}
		// We only want to view frames here, not save them
		else 
		{
			for (int i =0; i<numDevices ; i++)
			{	
				if( cameras.RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException)
					   && ptrGrabResult->GrabSucceeded() )
				{	
					void* data = ptrGrabResult->GetBuffer();
					memcpy(display_data[i],data, height*width);
					const cv::Mat M(height,width,CV_8U,display_data[i]);
					ostringstream intString;
		    		intString << "Camera_" << i;
					cv::imshow(intString.str(),M);
					cv::resizeWindow(intString.str(), (width*window_height)/height,window_height);
					cv::waitKey(1);
				}
				else
				{
					cerr << "Failure on Grab" << endl; 
					return; 
				}
			}
		}
	}
	catch (const GenericException & e)
	{
		cerr << "Exeception on grabAndSave(). Reason: "
 		 << e.GetDescription() << endl;

	}
	return;
}

void cameras_save(std::string path, int numImages, int s_width, int s_height)
{
	
	try
	{
		char* buffer = (char*) malloc(s_height*s_width); 
		if (buffer == NULL)
		{
			cerr << "Malloc Failure" << endl;
			return;
		}

		cout << "imsize = " << s_height*s_width << endl;

		int j = 0; 
		bool presence = true;
		int step_size = 1000;
		while (presence)
		{
			stringstream sstream;
			string tempFilename;
			FILE* tempFile;

			sstream << path << "/camera_" << j << ".tmp";
			sstream >> tempFilename; 
			
			// If the temporary data file for camera j exists... 
			if(access(tempFilename.c_str(),F_OK ) != -1)
			{
				for (int i =0; i<numImages; i=i+step_size)
				{
					if (i == 0)
					{
						tempFile = fopen(tempFilename.c_str(),"r"); 
						if (tempFile == NULL)
						{
							cerr << "Failure on opening disk buffer - Err: "
							     << strerror(errno)  << endl; 
							return; 
						}
					}

					for (int ss_counter=0; ss_counter<step_size && i+ss_counter<numImages; ss_counter++)
					{
					    int read = fread((void*)buffer,(s_height*s_width*step_size),1,tempFile);
				
					    if (read != 1)
					    {
					       cerr << "Failure on reading from disk buffer: - ErrNum: " 
							 << ferror(tempFile) << " - Obj Read: " << read 
							 <<  endl; 
						perror("Error : ");
						cerr << feof(tempFile) << endl;
						return; 
					    }

					    stringstream sstream_img; 
					    string imgFilename; 
	
					    sstream_img << path <<"/camera_" << j << "_img_"<< i+ss_counter << ".png";
					    sstream_img >> imgFilename;
	
					    CImagePersistence::Save(ImageFileFormat_Png,imgFilename.c_str(), 
						(const void*)buffer,s_height*s_width,PixelType_Mono8,s_width,s_height,0, 
						ImageOrientation_TopDown);
					  
					    if (i+ss_counter == numImages-1)
					    {
					       fclose(tempFile); 
					    }
					}
				}

				j+=1;

			}
			else
			{
				presence = false; 
			}
		}
		free(buffer);
	}
	catch (const GenericException & e)
	{
		cerr << "Exeception on grabAndSave(). Reason: "
 		 << e.GetDescription() << endl;
	}

	cout << "Image Seperation done" << endl; 
	return;
}

void cameras_setFPS(float new_fps)
{
	try
	{	
		for (int i=0; i<numDevices; i++)
		{
			INodeMap& nodemap = cameras[i].GetNodeMap();
	        CFloatPtr _fps( nodemap.GetNode( "AcquisitionFrameRate"));	 
	        CBooleanPtr _enable(nodemap.GetNode("AcquisitionFrameRateEnable"));
	        _fps->SetValue(new_fps); 
	        _enable->SetValue(true); 
	   	}
	   	fps = new_fps;
	   	cout << "FPS set to: " << fps << endl;
	}
	catch (const GenericException & e)
	{
		cerr << "Exeception on setFPS(). Reason: "
 		 << e.GetDescription() << endl;
	}
}

void cameras_setROI(unsigned int newWidth,unsigned int newHeight,
			unsigned int newOffsetX, unsigned int newOffsetY)
{
	try
	{	
		for (int i=0; i<numDevices; i++)
		{
			INodeMap& nodemap = cameras[i].GetNodeMap();
	        CIntegerPtr _offsetX( nodemap.GetNode( "OffsetX"));
	        CIntegerPtr _offsetY( nodemap.GetNode( "OffsetY"));
	        CIntegerPtr _width( nodemap.GetNode( "Width"));
	        CIntegerPtr _height( nodemap.GetNode( "Height"));
	
	        _offsetX->SetValue(Adjust(newOffsetX, _offsetX->GetMin(),
	        				  _offsetX->GetMax(), _offsetX->GetInc()));
	        _offsetY->SetValue(Adjust(newOffsetY, _offsetY->GetMin(), 
	        				  _offsetY->GetMax(), _offsetY->GetInc())); 
	        
	        width = Adjust(newWidth, _width->GetMin(), 
	        				  _width->GetMax(), _width->GetInc());
	        _width->SetValue(width); 
	
	        height = Adjust(newHeight, _height->GetMin(), 
	        				  _height->GetMax(), _height->GetInc()); 
	        _height->SetValue(height); 
	   	}
	}
	catch (const GenericException & e)
	{
		cerr << "Exeception on setROI(). Reason: "
 		 << e.GetDescription() << endl;
	}
	cout << "ROI Set" << endl;
}

#ifdef USE_MAIN
int main(){
	cameras_connect(); 
	cout << "Connected" << endl;
	cameras_start();
	stringstream sstream; 
	string tempFilename; 
	sstream << "data";
	sstream >> tempFilename;
	cameras_grab(tempFilename,10,true);
	cout << "Grabbed" << endl;
	cameras_stop(); 
	cout << "Stopped" << endl;
	cameras_disconnect(); 
}
#endif 
