from __future__ import print_function
import serial
import time
import sys
import os
from datetime import datetime

class OpticFlowSensor(serial.Serial):

    def __init__(self, port):
        super(OpticFlowSensor,self).__init__(port=port,baudrate=115200) 
        #super(OpticFlowSensor,self).__init__(port=port,baudrate=1000000) 
        #print('Initializing sensor')
        time.sleep(4.0)
        #print('done')

    def sendData(self,dat):
        dataSended = dat+'*'
        #print(dataSended)
        self.write(dataSended)

    def stop(self):
        self.write('s')

    def readData(self):
        dataList = []
        stopFlag = False
        if self.inWaiting() > 0:
            rawData = self.readline()
            rawData = rawData.strip()
            rawData = rawData.split(',')
            #print ("data",rawData)
            #a = str(datetime.now())
            #print (a)  
            if(len(rawData)==1):
                stopFlag = True
            else:
                try:
                    list1 = [int(x) for x in rawData]
                    #list1.append(a)
                    #print(dataList)
		    dataList.append(list1)
                except:
                    pass
        return dataList, stopFlag


# -----------------------------------------------------------------------------
if __name__ == '__main__':


    port = '/dev/arduinoCams'
    stop = False
    fifo_path = "/tmp/guiData.fifo"

    while True:

        if os.path.exists(fifo_path):
            os.unlink(fifo_path)
        if not os.path.exists(fifo_path):
            os.mkfifo(fifo_path)

        fifo = open(fifo_path, "r")
        dataReceived = fifo.readline()
        dataReceived = dataReceived.split(',')
        fifo.close()

        mode = dataReceived[0]
        frameRate = dataReceived[1]
        totTime = dataReceived[2]
        dataDir = dataReceived[3] + '/OptFlowData/OptFlow.txt'
        
        if not (mode == '1' or mode == '5'):
            if os.path.exists(dataDir)==True:
                pathToModify = dataDir.split(os.sep)
                del(pathToModify[0])
                datetimeNow = datetime.now()
                year = datetimeNow.year
                month = datetimeNow.month
                day = datetimeNow.day
                hour = datetimeNow.hour
                minute = datetimeNow.minute
                second = datetimeNow.second
                lastElem = pathToModify[-1]
                del(pathToModify[-1])
                stringToInsert = str(year)+'Y-'+str(month)+'M-'+str(day)+'D-'+str(hour)+'H-'+str(minute)+'M-'+str(second)+'S-'+lastElem
                pathToModify.append(stringToInsert)
                pathToMod='/'.join(pathToModify)
                dataDir = '/'+pathToMod
            else:
                pathList = dataDir.split(os.sep)
                del(pathList[0])
                del(pathList[-1])
                pathToCreate = '/'+'/'.join(pathList)
                if os.path.exists(pathToCreate)==False:
	            os.makedirs(pathToCreate)
            #fileTest = open(dataDir,"w")

        #print(mode)
        #print(frameRate)
        #print(totTime)
        #print(dataDir)

        sensor = OpticFlowSensor(port)
        sensor.sendData(mode)
        time.sleep(0.5)
        sensor.sendData(totTime)
        time.sleep(0.5)
        if(frameRate > 0):
            sensor.sendData(frameRate)
        print("Data sent to Arduino")
        stop = False
        savedData = []
        while not stop:
            data, stop = sensor.readData()
            if len(data) > 0:
                savedData.append(data)
        #print(savedData)
        if not (mode == '1' or mode == '5'):
            fileTest = open(dataDir,"w")
            for vals in savedData:
                try:
                    #print(vals)
                    ret = fileTest.write(str(vals[0][0])+','+str(vals[0][1])+','+str(vals[0][2])+','+ str(vals[0][3])+','+str(vals[0][4])+'\n')
                except IndexError:
                    pass
            fileTest.close()
        print("Experiment Finished")
        #sensor.stop()
