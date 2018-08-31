echo log.bit.bin > /sys/class/fpga_manager/fpga0/firmware
modprobe i2c-dev
./train_setup.py
./train 7 7 7 7
./frame_server 8080
