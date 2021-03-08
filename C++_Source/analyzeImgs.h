#include <opencv2/opencv.hpp>
#include <string>


using namespace std;

class analyzeImgs
{
 public:
 analyzeImgs(void);
 void setFPS(float fps);
 void setNumCams(int num);
 void getROI(cv::Mat img);
 cv::Mat binarize(cv::Mat img);
 cv::Mat getMask(cv::Mat img);
 void compareImgs(cv::Mat prevIm,cv::Mat newIm);
 bool connectAnalysis(cv::Mat img, int camera);

 bool isMoving(cv::Mat img, int camera);

 cv::Mat image;

 private:

   cv::Mat im_gray;
   cv::Mat im_roi;
   cv::Mat im_move;
   cv::Mat prevIm;
   cv::Mat newIm;

   vector <bool> firstIm;
   vector <int> contT;
   vector <int> contF;
   vector <cv::Mat> prevImBW;

   int h, w;
   int threshold1 = 110;
   int threshold2 = 10;

   int nFrames;
   int nCams;
   bool flag = false;
   bool moving = false;
};
