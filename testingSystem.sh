
start_time=$(date +%s.%3N)

for i in {1..100}
do
   ./bin/faktorial > /dev/null 2>&1
done

end_time=$(date +%s.%3N)

# elapsed time with second resolution
elapsed=$(echo "scale=3; $end_time - $start_time" | bc)
echo "Elapsed time (ms):"
echo $elapsed

cd ./language

start_time=$(date +%s.%3N)

for i in {1..100}
do
   ./CPU > /dev/null 2>&1
done

end_time=$(date +%s.%3N)

# elapsed time with second resolution
elapsed=$(echo "scale=3; $end_time - $start_time" | bc)
echo "Elapsed time (ms):"
echo $elapsed
