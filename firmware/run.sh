#!/bin/bash

if [ "${1}" == "load" ]; then
   # make
   make || exit 1
fi

# upload code and open picocom sessions in tmux windows
for device in /dev/ttyUSB*
do
   printf "Creating pane for: ${device}\n"
   tmuxcmd="picocom -q -b 57600 ${device}"
   if [ "${1}" == "load" ];
   then
      tmuxcmd="avrdude -q -c arduino -p m328p -P ${device} -b 57600 -U flash:w:${2} && ${tmuxcmd}"
   fi
   if [ "${device: -1}" == "0" ]
   then
      tmux new-session -d -s blocks "${tmuxcmd}"
      tmux rename-window "blocks"
   else
      tmux split-window -h "${tmuxcmd}"
   fi
done

tmux select-window -t blocks:0
tmux select-pane -t 0
tmux select-layout -t blocks even-horizontal
tmux set -g mouse on

# key bindings
tmux unbind-key C-c
tmux unbind-key C-r
# exit
tmux bind-key -n C-x setw synchronize-panes on\\\; send-keys C-a\\\; send-keys C-x\\\; send-keys C-c\\\;
# reset microcontrollers
tmux bind-key -n C-r setw synchronize-panes on\\\; send-keys C-a\\\; send-keys C-p\\\; setw synchronize-panes off\\\;
# show the session
tmux attach-session -t blocks


