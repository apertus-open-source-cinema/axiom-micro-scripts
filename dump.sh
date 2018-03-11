for msb in $(printf 0x%x'\n' $(seq 0x00 0xff)); do
    for lsb in $(printf 0x%x'\n' $(seq 0x00 0xff)); do
        i2ctransfer -y 1 w2@0x10 $msb $lsb r2
    done
done
#i2ctransfer -y 1 w2@0x10 0x00 0x00 r8192

