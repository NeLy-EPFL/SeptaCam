#include "analyzeImgs.h"
#include <opencv2/opencv.hpp>

analyzeImgs::analyzeImgs()
{
  cout<<"Ready to analyze images"<<endl;
}

void analyzeImgs::setFPS(float fps)
{
  nFrames = (int)fps/5;
}

void analyzeImgs::setNumCams(int num)
{
  nCams = num;
  cv::Mat M;
  for (int i=0;i<nCams;i++){
     firstIm.push_back(true);
     contT.push_back(0);
     contF.push_back(0);
     prevImBW.push_back(M);
  }
}

void analyzeImgs::getROI(cv::Mat img)
{
  //cout<<"getROI"<<endl;
  
  //cvtColor(img, im_gray, CV_RGB2GRAY);
  h = img.rows;
  w = img.cols;

  cv::Rect roi;
  roi.x = 0;
  roi.y = (int) (h/4 - 1);
  roi.width = w;
  roi.height = (int) 3*h/4;

  //cout <<"X:"<<roi.x<<"; Y:"<<roi.y<<"; W:"<<roi.width<<"; H:"<<roi.height<<endl;
  im_roi = img(roi);
}

cv::Mat analyzeImgs::binarize(cv::Mat img)
{
  //cout<<"binarize"<<endl;
  cv::Mat th1;
  cv::Mat th2;
  cv::Mat im_bw;

  cv::threshold( img, th1, threshold1, 255, 1); // 1 is for Binary Inverted mode
  cv::threshold( img, th2, threshold2, 255, 0); // 0 is for Binary mode
    
  cv::bitwise_and(th1,th2,im_bw);

  return im_bw;
                
}

cv::Mat analyzeImgs::getMask(cv::Mat img)
{
  //cout<<"getMask"<<endl;
   cv::Mat img_inv;
   cv::Mat im_mask;
   cv::bitwise_not(img, img_inv);

   cv::Mat kernel = getStructuringElement( cv::MORPH_ELLIPSE,
                                       cv::Size( 6, 6 ),
                                       cv::Point( 1, 1 ) );
   
   cv::erode(img_inv, im_mask, kernel);  
   
   return im_mask;
  
}

void analyzeImgs::compareImgs(cv::Mat pImg, cv::Mat nImg)
{
  //cout<<"compareImgs"<<endl;
  h = nImg.rows;
  w = nImg.cols;

  cv::bitwise_and(pImg,nImg,im_move);
  

  //cv::imshow("Previous Image",pImg);
  //cv::imshow("New Image",nImg);
  //cv::imshow("Comparison",im_move);
  
  //cv::waitKey(1);
}

bool analyzeImgs::connectAnalysis(cv::Mat img,int camera)
{
  //cout<<"connectAnalysis"<<endl;
  cv::Mat labelIm, stats, centroids; 
  int nLabels = cv::connectedComponentsWithStats(img, labelIm, stats, centroids, 8, CV_32S);
  int numComp = 0;
  for (int i=0;i<nLabels;i++){
      if (stats.at<int>(i,4)>100){
          numComp += 1;
      }
  }
  //cout<<numComp<<endl;
  if (numComp > 1){
     contT[camera] += 1;
     if (contF[camera] > 0) contF[camera] = 0;
     if (contT[camera] > nFrames/2 ){//&& contF < nFrames){
        flag = true;
        contT[camera] = 0;
     }
  }
  else{
     contF[camera] += 1;
     if (contT[camera] > 0) contT[camera] = 0;
     if (contF[camera] > nFrames ){//&& contT < nFrames/2){
        flag = false;
        contF[camera] = 0;
     }
  }  
  //cout<<"Count TRUE = "<<contT<<endl;
  //cout<<"Count FALSE = "<<contF<<endl;

  return flag;
}

bool analyzeImgs::isMoving(cv::Mat img, int camera)
{
 //cout<<"Entering"<<endl;
 if(!firstIm[camera]){
    //cout<<"Entering IF"<<endl;
    getROI(img);
    newIm = binarize(im_roi);
    prevIm = getMask(prevImBW[camera]);
    compareImgs(prevIm,newIm);
    moving = connectAnalysis(im_move, camera);
    prevImBW[camera] = newIm;
    image=im_move;
 }
 else{
    //cout<<"Entering ELSE"<<endl;
    getROI(img);
    prevImBW[camera] = binarize(im_roi);
    firstIm[camera] = false;
    moving = false;
    image=img;
 }

 return moving;
}


















