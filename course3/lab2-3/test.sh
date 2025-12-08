#!/bin/bash

echo "=== Testing Synchronized List Implementation ==="
echo ""

sizes=(100 1000 10000 100000)
sync_types=(0 1 2)
sync_names=("mutex" "spinlock" "rwlock")

for size in "${sizes[@]}"; do
    echo "======================================"
    echo "List size: $size"
    echo "======================================"
    
    for sync_type in "${sync_types[@]}"; do
        echo ""
        echo "--- Testing with ${sync_names[$sync_type]} ---"
        
        timeout 10 ./lab2-3 $size $sync_type 2>&1 | tail -20
        
        echo ""
        sleep 1
    done
    
    echo ""
done

echo "=== All tests completed ==="
