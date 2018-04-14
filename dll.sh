
./control.py

for i in $(seq 0 7); do
    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x0
    sleep .1
    i2ctransfer -y 1 w4@0x18 0x31 0xc0 0x0 $(($i))
    sleep .1
    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x1
    ./dma_test && mv dump.ram dump_lane_1_$i
done

for i in $(seq 0 7); do
    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x0
    sleep .1
    i2ctransfer -y 1 w4@0x18 0x31 0xc0 0x0 $(($i << 3))
    sleep .1
    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x1
    ./dma_test && mv dump.ram dump_lane_2_$i
done

for i in $(seq 0 7); do
    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x0
    sleep .1
    i2ctransfer -y 1 w4@0x18 0x31 0xc0 $(($i >> 2)) $((($i << 6) & 0xff))
    sleep .1
    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x1
    ./dma_test && mv dump.ram dump_lane_3_$i
done

for i in $(seq 0 7); do
    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x0
    sleep .1
    i2ctransfer -y 1 w4@0x18 0x31 0xc0 $(($i << 1)) 0x0
    sleep .1
    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x1
    ./dma_test && mv dump.ram dump_lane_4_$i
done


# for i in $(seq 0 7); do
#     for j in $(seq 0 7); do
# 	    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x0
# 	    sleep .1
# 	    i2ctransfer -y 1 w4@0x18 0x31 0xc0 $(($j << 4)) $(($i))
# 	    sleep .1
# 	    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x1
# 	    ./dma_test && mv dump.ram dump_lane_1_$i$j
#     done    
# done
# 
# for i in $(seq 0 7); do
#     for j in $(seq 0 7); do
# 	    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x0
# 	    sleep .1
# 	    i2ctransfer -y 1 w4@0x18 0x31 0xc0 $(($j << 4)) $(($i << 3))
# 	    sleep .1
# 	    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x1
# 	    ./dma_test && mv dump.ram dump_lane_2_$i$j
#     done    
# done
# 
# for i in $(seq 0 7); do
#     for j in $(seq 0 7); do
# 	    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x0
# 	    sleep .1
# 	    i2ctransfer -y 1 w4@0x18 0x31 0xc0 $((($j << 4) + ($i >> 2))) $((($i << 6) & 0xff))
# 	    sleep .1
# 	    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x1
# 	    ./dma_test && mv dump.ram dump_lane_3_$i$j
#     done    
# done
# 
# for i in $(seq 0 7); do
#     for j in $(seq 0 7); do
# 	    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x0
# 	    sleep .1
# 	    i2ctransfer -y 1 w4@0x18 0x31 0xc0 $((($j << 4) + ($i << 1))) 0x0
# 	    sleep .1
# 	    i2ctransfer -y 1 w3@0x18 0x30 0x1c 0x1
# 	    ./dma_test && mv dump.ram dump_lane_4_$i$j
#     done    
# done
