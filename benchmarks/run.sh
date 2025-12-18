if [ ! -d "picohttpparser" ] || [ ! -e "httppv1.h" ] || [ ! -e "httppv2.h" ]; then
    git clone https://github.com/h2o/picohttpparser.git picohttpparser
    wget -O httppv1.h https://github.com/cebem1nt/httpp/releases/download/1.0.0/httpp.h
fi

if [ -n "$1" ]; then
    OPT=$1
else
    OPT="-O3"
fi

ITERATIONS=10000000
RUNS=5

gcc $OPT -DITERATIONS=$ITERATIONS -DRUNS=$RUNS picohttpparser/picohttpparser.c bench-pico.c -o picohttpparser.out
gcc $OPT -DITERATIONS=$ITERATIONS -DRUNS=$RUNS bench-httppv1.c -o httppv1.out
gcc $OPT -DITERATIONS=$ITERATIONS -DRUNS=$RUNS bench-httppv2.c -o httppv2.out

echo "Benchmarking httpp (1.0.0)..."
./httppv1.out

sleep 1

echo "Benchmarking httpp (2.0.0)..."
./httppv2.out

sleep 1

echo "Benchmarking picohttpparser..."
./picohttpparser.out
