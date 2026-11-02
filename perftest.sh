#!/bin/bash

for t in release/perf_*; do
    ./$t
done
