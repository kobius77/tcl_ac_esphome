#!/usr/bin/env bash
set -euo pipefail

CSV="ramp_$(date +%Y%m%d_%H%M%S).csv"
echo "uptime_s,step,ramp_target,ext_temp,int_temp,power_w,field,raw_hex" > "$CSV"

echo "Capturing to $CSV — Ctrl+C to stop"

esphome logs veskaac.yml --device 192.168.1.78 2>&1 \
  | grep --line-buffered "RAMP," | sed -u 's/^.*RAMP,//' >> "$CSV"

echo ""
echo "Done — $CSV ready"
wc -l "$CSV"
