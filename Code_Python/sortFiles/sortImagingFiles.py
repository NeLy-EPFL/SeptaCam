import os, sys
from stat import *
from datetime import datetime

dateStr = datetime.now().strftime("%y%m%d")
datetimeNow = datetime.now()
year = datetimeNow.year
month = datetimeNow.month
day = datetimeNow.day
timeStampToday = (year-1970) * 31536000 + (month-1) * 2592000 + (day-1) * 86400
src = '/data'

def getModifiedFolders(path):
    listOfFiles = os.listdir(path)
    foldersChanged = []
    for f in listOfFiles:
        if not f.startswith('.'):
            expPath = path + "/" + f
            mode = os.stat(expPath).st_mode
            if S_ISDIR(mode):
                info = os.lstat(expPath)
                if(info.st_mtime > timeStampToday):
                    foldersChanged.append(expPath)
    return foldersChanged

personFolders = getModifiedFolders(src)
for folder in personFolders:
    experimentFolders = getModifiedFolders(folder)
    for experiment in experimentFolders:
        if experiment.startswith(folder+'/'+dateStr):
            fliesFolders = os.listdir(experiment)
            for fly in fliesFolders:
                pathFly = experiment + '/' + fly
                dataFolders = os.listdir(pathFly)
                for f in dataFolders:
                    if not f.startswith('0'):
                        typeIm_trial = f.split('_')
                        orig = pathFly + '/' + f
                        dst = pathFly + '/' + typeIm_trial[-1] + '_' + typeIm_trial[0]
                        thImgFolder = dst + '/ThorImage' 
                        thSyncFolder = dst + '/ThorSync' 
                        if len(typeIm_trial)>2:
                            os.renames(orig,thSyncFolder)
                        else:
                            os.renames(orig,thImgFolder)    
