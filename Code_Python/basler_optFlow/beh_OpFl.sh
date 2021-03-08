#!/bin/bash

python /NIR_Imaging/Code_Python/basler_optFlow/readOpFlData.py &
python /NIR_Imaging/Code_Python/stimulation/stim_fifo.py &
/NIR_Imaging/GUI_BASLER_m  && kill $!
