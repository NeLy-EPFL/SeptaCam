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
    fifo_path = "/tmp/stimData.fifo"

    while True:
        controlSeq = []
        valuesSeq = []
        dataStr = ''

        if os.path.exists(fifo_path):
            os.unlink(fifo_path)
        if not os.path.exists(fifo_path):
            os.mkfifo(fifo_path)

        fifo = open(fifo_path, "r")
        dataReceived = fifo.readline()
        dataReceived = dataReceived.split(',')
        fifo.close()

        startCapture = dataReceived[0]
        totTime = dataReceived[1]
        dataDir = dataReceived[2] + '/StimulusData/stimulusSequence.txt'

        if (startCapture):
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

	with open('/NIR_Imaging/Code_Python/stimulation/seqFile.txt', 'r') as f:
	    fileLines = f.readlines()
	for line in fileLines:
	    if(len(line) > 1):
	        lineReceived = line.split(',')
	        controlSeq.append(lineReceived[0].strip())
	        valuesSeq.append(lineReceived[1].strip())

        fileTest = open(dataDir,"w")

        try:
            ret = fileTest.write('Total time of the experiment: ' + totTime + ' s\nStimulus Sequence:\n')
            for i in range(len(valuesSeq)):
                ret2 = fileTest.write(controlSeq[i]+', ' +valuesSeq[i]+'\n')
        except IndexError:
            pass
            
        fileTest.close()

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

	print('Stimulus: Time sequence = ' + dataStr)
	print('Stimulus: Number of loops = ' + loopsStr)
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

