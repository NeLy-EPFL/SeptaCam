#!/bin/bash

basedir=$(realpath $(dirname ${BASH_SOURCE[0]}))

python $basedir/Code_Python/basler_optFlow/readOpFlData.py &
pid0=$!
python $basedir/Code_Python/stimulation/stim_fifo.py &
pid1=$!
python3 $basedir/Code_Python/getVideos/make_videos_online.py &
pid2=$!
python3 $basedir/Code_Python/flowrate_control/controller.py &
pid3=$!

$basedir/GUI_BASLER_m

echo "Terminating readOpFlData.py (PID $pid0), stim_fifo.py (PID $pid1), make_videos_online.py (PID $pid2), and MFC controller (PID $pid3)"
kill $pid0 $pid1 $pid2 $pid3
