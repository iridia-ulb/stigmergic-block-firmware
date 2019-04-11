# make
make

# upload in parallel 
parallel -j 8 avrdude -c arduino -p m328p -P {} -b 57600 -U flash:w:build/firmware.hex ::: /dev/ttyUSB*

# open picocom sessions in tmux windows
for device in /dev/ttyUSB*
do
   printf "Creating pane for: ${device}\n"
   if [ "${device: -1}" == "0" ]
   then
      tmux new-session -d -s blocks "picocom -b 57600 ${device}"
      tmux rename-window 'blocks'
   else
      tmux split-window -h "picocom -b 57600 ${device}"
   fi
done

tmux select-window -t blocks:0
tmux select-pane -t 0
tmux select-layout -t blocks tiled

tmux set -g mouse on

tmux attach-session -t blocks
