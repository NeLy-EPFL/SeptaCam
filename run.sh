#!/bin/bash

basedir=$(realpath $(dirname ${BASH_SOURCE[0]}))

python $basedir/Code_Python/basler_optFlow/readOpFlData.py &
pid0=$!
python $basedir/Code_Python/stimulation/stim_fifo.py &
pid1=$!
python3 Code_Python/getVideos/make_videos_online.py &
pid2=$!

$basedir/GUI_BASLER_m

echo "Terminating readOpFlData.py (PID $pid0), stim_fifo.py (PID $pid1), and make_videos_online.py (PID $pid2)"
kill $pid0 $pid1 $pid2
reset
