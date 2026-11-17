#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <benchmark-executable>"
    exit 1
fi

BENCHMARK="$1"

if [[ ! -x "$BENCHMARK" ]]; then
    echo "Error: Benchmark '$BENCHMARK' does not exist or is not executable."
    exit 1
fi

CPU=cpu2
CPUFREQ="/sys/devices/system/cpu/${CPU}/cpufreq"

if [[ ! -d "$CPUFREQ" ]]; then
    echo "Error: cpufreq not available for $CPU"
    exit 1
fi

MAX=$(cat "$CPUFREQ/scaling_max_freq")

echo "$MAX" | sudo tee "$CPUFREQ/scaling_min_freq"

echo "$CPU minimum frequency set to ${MAX}"

taskset -c 2 "BENCHMARK" \
    --benchmark_min_warmup_time=0.5 \
    --benchmark_min_time=2 \
    --benchmark_repetitions=5 \
    --benchmark_out=benchmark_result.csv \
    --benchmark_out_format=csv \
    --benchmark_report_aggregates_only=true
