#!/usr/bin/env python3
"""
Generate synthetic TIFF test files for GeoTIFF detection testing.

This script creates minimal TIFF files with and without GeoTIFF tags
for testing the format detection logic.
"""

import struct
import os

def write_le_geotiff(filename):
    """Create a minimal little-endian GeoTIFF file."""
    with open(filename, 'wb') as f:
        # TIFF header (little-endian)
        f.write(b'II')  # Magic number (little-endian)
        f.write(struct.pack('<H', 42))  # Version (classic TIFF)
        f.write(struct.pack('<I', 8))  # IFD offset
        
        # IFD (Image File Directory)
        f.write(struct.pack('<H', 2))  # Number of entries
        
        # Entry 1: ImageWidth (tag 256)
        f.write(struct.pack('<H', 256))  # Tag
        f.write(struct.pack('<H', 3))    # Type (SHORT)
        f.write(struct.pack('<I', 1))    # Count
        f.write(struct.pack('<I', 100))  # Value
        
        # Entry 2: GeoKeyDirectoryTag (tag 34735) - This makes it a GeoTIFF
        f.write(struct.pack('<H', 34735))  # Tag
        f.write(struct.pack('<H', 3))      # Type (SHORT)
        f.write(struct.pack('<I', 4))      # Count
        f.write(struct.pack('<I', 100))    # Offset to data
        
        # Next IFD offset (0 = no more IFDs)
        f.write(struct.pack('<I', 0))
        
        # GeoKey data
        f.write(struct.pack('<HHHH', 1, 1, 0, 0))

def write_be_geotiff(filename):
    """Create a minimal big-endian GeoTIFF file."""
    with open(filename, 'wb') as f:
        # TIFF header (big-endian)
        f.write(b'MM')  # Magic number (big-endian)
        f.write(struct.pack('>H', 42))  # Version (classic TIFF)
        f.write(struct.pack('>I', 8))   # IFD offset
        
        # IFD (Image File Directory)
        f.write(struct.pack('>H', 2))  # Number of entries
        
        # Entry 1: ImageWidth (tag 256)
        f.write(struct.pack('>H', 256))  # Tag
        f.write(struct.pack('>H', 3))    # Type (SHORT)
        f.write(struct.pack('>I', 1))    # Count
        f.write(struct.pack('>I', 100))  # Value
        
        # Entry 2: GeoKeyDirectoryTag (tag 34735) - This makes it a GeoTIFF
        f.write(struct.pack('>H', 34735))  # Tag
        f.write(struct.pack('>H', 3))      # Type (SHORT)
        f.write(struct.pack('>I', 4))      # Count
        f.write(struct.pack('>I', 100))    # Offset to data
        
        # Next IFD offset (0 = no more IFDs)
        f.write(struct.pack('>I', 0))
        
        # GeoKey data
        f.write(struct.pack('>HHHH', 1, 1, 0, 0))

def write_regular_tiff(filename):
    """Create a minimal regular TIFF file without GeoTIFF tags."""
    with open(filename, 'wb') as f:
        # TIFF header (little-endian)
        f.write(b'II')  # Magic number
        f.write(struct.pack('<H', 42))  # Version
        f.write(struct.pack('<I', 8))   # IFD offset
        
        # IFD with only standard TIFF tags (no GeoTIFF tags)
        f.write(struct.pack('<H', 1))  # Number of entries
        
        # Entry 1: ImageWidth (tag 256)
        f.write(struct.pack('<H', 256))  # Tag
        f.write(struct.pack('<H', 3))    # Type (SHORT)
        f.write(struct.pack('<I', 1))    # Count
        f.write(struct.pack('<I', 100))  # Value
        
        # Next IFD offset
        f.write(struct.pack('<I', 0))

def write_corrupted_tiff(filename):
    """Create a file with corrupted TIFF header."""
    with open(filename, 'wb') as f:
        # Invalid magic number
        f.write(b'XX')
        f.write(struct.pack('<H', 42))
        f.write(struct.pack('<I', 8))

def write_truncated_tiff(filename):
    """Create a truncated TIFF file (< 8 bytes)."""
    with open(filename, 'wb') as f:
        # Only 4 bytes (incomplete header)
        f.write(b'II')
        f.write(struct.pack('<H', 42))

def write_non_tiff(filename):
    """Create a non-TIFF file."""
    with open(filename, 'w') as f:
        f.write("This is not a TIFF file.\n")

def main():
    """Generate all test files."""
    data_dir = 'data'
    os.makedirs(data_dir, exist_ok=True)
    
    print("Generating test files...")
    
    write_le_geotiff(os.path.join(data_dir, 'le_geotiff.tif'))
    print("  Created le_geotiff.tif (little-endian GeoTIFF)")
    
    write_be_geotiff(os.path.join(data_dir, 'be_geotiff.tif'))
    print("  Created be_geotiff.tif (big-endian GeoTIFF)")
    
    write_regular_tiff(os.path.join(data_dir, 'regular.tif'))
    print("  Created regular.tif (regular TIFF without GeoTIFF tags)")
    
    write_corrupted_tiff(os.path.join(data_dir, 'corrupted.tif'))
    print("  Created corrupted.tif (corrupted header)")
    
    write_truncated_tiff(os.path.join(data_dir, 'truncated.tif'))
    print("  Created truncated.tif (truncated file)")
    
    write_non_tiff(os.path.join(data_dir, 'not_tiff.txt'))
    print("  Created not_tiff.txt (non-TIFF file)")
    
    print("All test files generated successfully!")

if __name__ == '__main__':
    main()
