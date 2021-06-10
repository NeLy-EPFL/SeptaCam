#!/bin/bash

basedir=$(realpath $(dirname ${BASH_SOURCE[0]}))
python $basedir/Code_Python/basler_optFlow/readOpFlData.py &
pid0=$!
python $basedir/Code_Python/stimulation/stim_fifo.py &
pid1=$!
#valgrind $basedir/GUI_BASLER_m
$basedir/GUI_BASLER_m
echo "Sending SIGTERM to readOpFlData.py (PID $pid0) and stim_fifo.py (PID $pid1)"
kill $pid0 $pid1
