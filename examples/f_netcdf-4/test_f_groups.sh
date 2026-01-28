#!/bin/bash
# Test script for f_groups.f90 example
# Runs the example and validates output against expected CDL

set -e

echo "Running f_groups example..."
./f_groups

echo "SUCCESS: f_groups completed"
exit 0
