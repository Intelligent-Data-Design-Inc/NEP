#!/bin/bash
# Validate NetCDF output against expected CDL
# Edward Hartnett 1/26/26

EXAMPLE_NAME=$1
OUTPUT_FILE=$2
EXPECTED_CDL=$3

if [ $# -ne 3 ]; then
    echo "Usage: $0 <example_name> <output_file> <expected_cdl>"
    exit 1
fi

if [ ! -f "$OUTPUT_FILE" ]; then
    echo "ERROR: Output file not found: $OUTPUT_FILE"
    exit 1
fi

if [ ! -f "$EXPECTED_CDL" ]; then
    echo "ERROR: Expected CDL file not found: $EXPECTED_CDL"
    exit 1
fi

# Find ncdump from the NetCDF-C library being used
# Try nc-config first (preferred method)
if command -v nc-config >/dev/null 2>&1; then
    NETCDF_BIN=$(nc-config --prefix)/bin
    NCDUMP="${NETCDF_BIN}/ncdump"
else
    # Fall back to PATH
    NCDUMP="ncdump"
fi

# Verify ncdump is available
if ! command -v "$NCDUMP" >/dev/null 2>&1; then
    echo "ERROR: ncdump not found. Tried: $NCDUMP"
    exit 1
fi

# Run ncdump on generated file
"$NCDUMP" "$OUTPUT_FILE" > "${OUTPUT_FILE}.cdl" 2>&1
if [ $? -ne 0 ]; then
    echo "ERROR: ncdump failed on $OUTPUT_FILE"
    cat "${OUTPUT_FILE}.cdl"
    exit 1
fi

# Compare with expected CDL
diff -u "$EXPECTED_CDL" "${OUTPUT_FILE}.cdl"
if [ $? -ne 0 ]; then
    echo "ERROR: Output differs from expected for $EXAMPLE_NAME"
    echo "Expected: $EXPECTED_CDL"
    echo "Generated: ${OUTPUT_FILE}.cdl"
    exit 1
fi

echo "PASS: $EXAMPLE_NAME output matches expected CDL"
rm -f "${OUTPUT_FILE}.cdl"
exit 0
