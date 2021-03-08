import cv2
import numpy as np
import os, sys    #Importing the System Library
#from matplotlib import pyplot as plt
import time


def getROI(img):
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    h, w = gray.shape
    roi = np.zeros((h, w), np.uint8)
    roi = gray[h/2:,:]
    return roi
    
def binarize(img):    
    ret,th1 = cv2.threshold(img,110,255,cv2.THRESH_BINARY_INV)
    ret,th2 = cv2.threshold(img,10,255,cv2.THRESH_BINARY)
    
    bw = cv2.bitwise_and(th1,th2)
                
    return bw
    
def getMask(img):
    img_inv = cv2.bitwise_not(img)
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE,(6,6))    
    erode = cv2.erode(img_inv,kernel,iterations = 1)    
    
    return erode
    
def compareImgs(prev,new):
    h, w = new.shape
    im_comp = np.zeros((h, w), np.uint8)
    cv2.bitwise_and(prev,new,im_comp)
    #for i in range(h):
    #    for j in range(w):
    #        if new[i,j] == 255 and prev[i,j] == 0:
    #            im_comp[i,j] = 255
    
    return im_comp
    
def connectAnalysis(img):
    output = cv2.connectedComponentsWithStats(img, 4, cv2.CV_32S)
    numComp = 0
    for comp in output[2]:
        if (comp[4] > 100):
            numComp += 1
    if(numComp>1):
        print('Moving')
    else:
        print('Resting')
        
def cleanImg(img):
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE,(4,4))
    clean = cv2.morphologyEx(img, cv2.MORPH_OPEN, kernel)
    
    #clean = cv2.erode(img,kernel,iterations = 1)    
    
    return clean

def getEdgesIm(gray):
    #gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    
    v = np.median(gray)
    sigma = 0.33

    #---- apply optimal Canny edge detection using the computed median----
    lower_thresh = int(max(0, (1.0 - sigma) * v))
    upper_thresh = int(min(255, (1.0 + sigma) * v))
    im_edges = cv2.Canny(gray, lower_thresh,upper_thresh)

    return im_edges
    
def fillingHoles(img):
    im_floodfill = img.copy()
 
    # Mask used to flood filling.
    # Notice the size needs to be 2 pixels than the image.
    h, w = img.shape[:2]
    mask = np.zeros((h+2, w+2), np.uint8)
 
    # Floodfill from point (0, 0)
    cv2.floodFill(im_floodfill, mask, (0,0), 255);
 
    # Invert floodfilled image
    im_floodfill_inv = cv2.bitwise_not(im_floodfill)
 
    # Combine the two images to get the foreground.
    im_filled = img | im_floodfill_inv
 
    return im_filled


def main(args):
    print('Starting triggering')
    cam0=[]
    cam3=[]
    for i in range(1400,1700):
        cam0.append(cv2.imread('examples/camera_0_img_' + str(i) + '.png'))
        #cam3.append(cv2.imread('camera_3_img_' + str(i) + '.png'))
    im_roi = getROI(cam0[0])
    im_prev = binarize(im_roi)
    del cam0[0]
    cont=0
    timeElapsed=[]
    for img in cam0:
        initTime = time.time()
        print(cont)
        cont+=1
        im_roi = getROI(img)
        
        im_new = binarize(im_roi) 
               
        im_mask = getMask(im_prev)        
        im_move = compareImgs(im_mask,im_new)
        connectAnalysis(im_move)
        
        im_prev = im_new
        
        endTime = time.time()
        
        timeElapsed.append(endTime-initTime)
        
        print('Total time: ' + str(endTime-initTime))
        
        
        #im_edges = getEdgesIm(im_roi)
        #im_clean = cleanImg(im_move)
        #im_seg = fillingHoles(im_clean)
        
        cv2.imshow("ROI", im_roi)
        #cv2.imshow("Prev", im_mask)
        #cv2.imshow("New", im_new)
        cv2.imshow("Move", im_move)
        #cv2.imshow("Edges Image", im_edges)
        #cv2.imshow("Dilate Image", im_clean)
        #cv2.imshow("Fly Segmented", im_seg)
        cv2.waitKey(0)
        
    print('Max time: ' + str(max(timeElapsed)))
    print('Mean time: ' + str(np.mean(timeElapsed)))
    

if __name__ == '__main__':
    main(sys.argv)
