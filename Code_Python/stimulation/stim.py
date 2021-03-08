from __future__ import print_function
import serial
import time
import sys
import os
from datetime import datetime

class serialCom(serial.Serial):

    def __init__(self, port):
        super(serialCom,self).__init__(port=port,baudrate=115200)
        time.sleep(4.0)

    def sendData(self,dat):
        dataSended = dat+'*'
        self.write(dataSended)

    def closePort(self):
        self.close()

    def readData(self):
        dataList = []
        stopFlag = False
        while self.inWaiting() > 0:
            rawData = self.readline()
            #print ("data",rawData)
            rawData = rawData.strip()
            rawData = rawData.split(',')
            a = str(datetime.now())
            #print (a)  
            if(len(rawData)==1):
                stopFlag = True
            else:
                try:
                    list1 = [int(x) for x in rawData]
                    list1.append(a)
                    dataList.append(list1)
                except:
                    pass
        return dataList, stopFlag


# -----------------------------------------------------------------------------
if __name__ == '__main__':


    port = '/dev/arduinoStim'
    #port = '/dev/ttyACM1'
    stop = False
    controlSeq = []
    valuesSeq = []
    dataStr = ''

    with open('seqFile.txt', 'r') as f:
        fileLines = f.readlines()
    for line in fileLines:
        if(len(line) > 1):
            lineReceived = line.split(',')
            controlSeq.append(lineReceived[0].strip())
            valuesSeq.append(lineReceived[1].strip())
    try:
        loopIndex = controlSeq.index('*')
        numLoops = valuesSeq[loopIndex]
        del valuesSeq[loopIndex]
    except ValueError:
        numLoops = 1

    if controlSeq[0]=='on':
        valuesSeq.insert(0,'0')
        
    for i in range(0,(len(valuesSeq)-1)):
        dataStr += valuesSeq[i] + ','

    dataStr +=  valuesSeq[len(valuesSeq)-1]
    loopsStr = str(numLoops)
    numValuesStr = str(len(valuesSeq))
    
    print('Time sequence = ' + dataStr)
    print('Number of loops = ' + loopsStr)
    #print('Number of steps = ' + numValuesStr)

    ardStimulus = serialCom(port)
    ardStimulus.sendData(loopsStr)
    time.sleep(0.2)
    ardStimulus.sendData(numValuesStr)
    time.sleep(0.2)
    ardStimulus.sendData(dataStr)

    ardStimulus.closePort()

    

    #while not stop:
    #    data, stop = ardStimulus.readData()
    #    #print (data)

