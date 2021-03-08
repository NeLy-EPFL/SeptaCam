import cv2
import numpy as np
import json
import sys
import os
from datetime import datetime

workLines = []

with open('/NIR_Imaging/Code_Python/getVideos/videosBatch.txt', 'r') as f:
    fileLines = f.readlines()
    if (fileLines[-1]) == 'now\n':
        workLines.append(fileLines[-2])
        del(fileLines[-1])
        del(fileLines[-2])  
    else:
        workLines = fileLines
        fileLines = ''

for line in workLines:
    cams = []
    data = line.split(',')
    folder = data[0]
    for i in range(1,len(data)):
        cams.append(int(data[i].strip()))

    with open(folder+'/capture_metadata.json', 'r') as f:
        imMetaData=json.load(f)

        fps = imMetaData["FPS"]
        numCams = imMetaData["Number of Cameras"]
        #numFrames = imMetaData["Number of Frames"]["0"]
        #imHeight = imMetaData["ROI"]["Height"]["0"]
        #imWidth = imMetaData["ROI"]["Width"]["0"]

    vidFromCam = []

    if (cams[-1] == 1):
        for i in range(0,numCams):
            vidFromCam.append(1)
    else:
        for i in range(0,numCams):
            vidFromCam.append(cams[i])
    for j in range(numCams,len(cams)-1):
        vidFromCam.append(0)

    folderVideos = folder.replace('images','videos')
    if os.path.exists(folderVideos)==False:
        os.makedirs(folderVideos)

#fourcc = cv2.VideoWriter_fourcc(*'XVID')
#fourcc = cv2.VideoWriter_fourcc(*'MJPG') # Good quality, big size, It's better ffmpeg directly
#fourcc = cv2.VideoWriter_fourcc(*'3IVD') # Medium quality, small size

    for cam in range(0,len(vidFromCam)):
        images = []
        if vidFromCam[cam] == 1:  
            vidName = folderVideos+'/camera_'+str(cam) +'.mp4'
            if os.path.exists(vidName)==True:
                datetimeNow = datetime.now()
                year = datetimeNow.year
                month = datetimeNow.month
                day = datetimeNow.day
                hour = datetimeNow.hour
                minute = datetimeNow.minute
                second = datetimeNow.second
                stringToInsert = '_' + str(year)+str(month)+str(day)+'-'+str(hour)+str(minute)+str(second)+'.mp4'
                vidName = vidName.replace('.mp4',stringToInsert)
        #outVid = cv2.VideoWriter(vidName, fourcc, fps, (imWidth,imHeight))
        #for frame in range(0,numFrames):
            imName = folder + '/camera_' + str(cam) + '_img_%d.jpg'
            #image = cv2.imread(imName)
            #outVid.write(image)
            os.system('ffmpeg -r ' + str(fps) + ' -i ' + imName + ' -pix_fmt yuv420p ' + vidName)
    #outVid.release()
        
with open('/NIR_Imaging/Code_Python/getVideos/videosBatch.txt', 'w') as f:
    for line in fileLines:
        f.write(line)
