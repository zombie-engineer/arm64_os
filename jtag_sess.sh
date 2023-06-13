#!/bin/bash

tmux send-keys -t 0.0 C-c
sleep 0.3
tmux send-keys -t 0.0 'make jtag' Enter
sleep 0.3
make jtagd
