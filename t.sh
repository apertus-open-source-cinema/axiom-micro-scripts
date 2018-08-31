for lane3 in $(seq 0 31); do
    for lane1 in $(seq 0 31); do
        for lane2 in $(seq 0 31); do
            for lane0 in $(seq 0 31); do
                echo $lane0 $lane0 $lane0 $lane0
                timeout 5 ./train $lane0 $lane0 $lane0 $lane0
            done
        done
    done
done
