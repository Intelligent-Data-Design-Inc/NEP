#!/bin/bash
# Test script for quickstart.c
# Runs the example and validates output matches expected CDL

set -e

EXAMPLE_NAME="quickstart"
OUTPUT_FILE="quickstart.nc"
EXPECTED_CDL="../expected_output/quickstart_expected.cdl"

# Run the example
./${EXAMPLE_NAME}

# Validate the output file was created
if [ ! -f ${OUTPUT_FILE} ]; then
    echo "Error: ${OUTPUT_FILE} not created"
    exit 1
fi

# Generate CDL from output
ncdump ${OUTPUT_FILE} > ${EXAMPLE_NAME}.cdl

# Validate against expected CDL if it exists
if [ -f ${EXPECTED_CDL} ]; then
    # Compare CDL files (ignore filename in header)
    if diff -I "^netcdf" ${EXPECTED_CDL} ${EXAMPLE_NAME}.cdl > /dev/null 2>&1; then
        echo "${EXAMPLE_NAME} test passed (CDL validated)"
    else
        echo "Error: ${EXAMPLE_NAME} CDL does not match expected output"
        echo "Expected:"
        cat ${EXPECTED_CDL}
        echo ""
        echo "Got:"
        cat ${EXAMPLE_NAME}.cdl
        exit 1
    fi
else
    echo "${EXAMPLE_NAME} test passed (no CDL validation)"
fi

exit 0
