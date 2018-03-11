while true; do devmem2 0x41200000 w 0x0; sleep .1; devmem2 0x41200000 w 0xff; sleep .1; done
