make
for filename in ../traces/*_debug.bz2; do
    echo "$filename"
    bunzip2 -kc "$filename" | ./cache --icache=256:1:2 --dcache=256:1:2 --l2cache=512:8:10 --blocksize=64 --memspeed=100 --inclusive
    bunzip2 -kc "$filename" | ./cache --icache=128:2:2 --dcache=128:4:2 --l2cache=256:8:10 --blocksize=64 --memspeed=100
    bunzip2 -kc "$filename" | ./cache --icache=128:2:2 --dcache=64:4:2 --l2cache=128:8:50 --blocksize=128 --memspeed=100 --inclusive
    bunzip2 -kc "$filename" | ./cache --icache=512:2:2 --dcache=256:4:2 --l2cache=16384:8:50 --blocksize=64 --memspeed=100 --inclusive
    bunzip2 -kc "$filename" | ./cache --icache=0:0:0 --dcache=0:0:0 --l2cache=8:1:50 --blocksize=128 --memspeed=100
done
make clean