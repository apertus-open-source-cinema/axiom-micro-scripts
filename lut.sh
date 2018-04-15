#!/usr/bin/env bash

for i in $(seq 0 4095); do
#    test_data_red:
#        address: 0x3072
#
#    test_data_greenr:
#        address: 0x3074
#
#    test_data_blue:
#        address: 0x3076
#
#    test_data_greenb:
#        address: 0x3078

    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x0

    i2ctransfer -y 1 w4@0x18 0x30 0x72 $((($i >> 8) & 0xff)) $(($i & 0xff))
    i2ctransfer -y 1 w4@0x18 0x30 0x74 $((($i >> 8) & 0xff)) $(($i & 0xff))
    i2ctransfer -y 1 w4@0x18 0x30 0x76 $((($i >> 8) & 0xff)) $(($i & 0xff))
    i2ctransfer -y 1 w4@0x18 0x30 0x78 $((($i >> 8) & 0xff)) $(($i & 0xff))

    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x1
    
    ./train $i
done
