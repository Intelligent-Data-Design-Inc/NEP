#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>
#include <stdint.h>

#define TEST_FILE "data/imap_mag_l1b-calibration_20240229_v001.cdf"

int main() {
    int ncid, retval;
    int64_t fillval;
    
    // Open the NetCDF file
    if ((retval = nc_open(TEST_FILE, NC_NOWRITE, &ncid))) {
        fprintf(stderr, "Error opening file: %s\n", nc_strerror(retval));
        return 1;
    }
    
    // Get the variable ID for STARTVALIDITY
    int varid;
    if ((retval = nc_inq_varid(ncid, "STARTVALIDITY", &varid))) {
        fprintf(stderr, "Error getting variable ID: %s\n", nc_strerror(retval));
        nc_close(ncid);
        return 1;
    }
    
    // Get the FILLVAL attribute
    nc_type att_type;
    size_t att_len;
    if ((retval = nc_inq_att(ncid, varid, "FILLVAL", &att_type, &att_len))) {
        fprintf(stderr, "Error getting attribute info: %s\n", nc_strerror(retval));
        nc_close(ncid);
        return 1;
    }
    
    printf("FILLVAL attribute type: %d, length: %zu\n", att_type, att_len);
    
    // Read the FILLVAL attribute
    if ((retval = nc_get_att_longlong(ncid, varid, "FILLVAL", (long long*)&fillval))) {
        fprintf(stderr, "Error reading attribute: %s\n", nc_strerror(retval));
        nc_close(ncid);
        return 1;
    }
    
    printf("FILLVAL value as int64_t: %lld\n", (long long)fillval);
    
    // Try reading as double as well
    double fillval_double;
    if ((retval = nc_get_att_double(ncid, varid, "FILLVAL", &fillval_double))) {
        fprintf(stderr, "Error reading attribute as double: %s\n", nc_strerror(retval));
    } else {
        printf("FILLVAL value as double: %g\n", fillval_double);
    }
    
    // Get the actual variable data
    int64_t var_data;
    if ((retval = nc_get_var_longlong(ncid, varid, (long long*)&var_data))) {
        fprintf(stderr, "Error reading variable data: %s\n", nc_strerror(retval));
    } else {
        printf("STARTVALIDITY value: %lld\n", (long long)var_data);
    }
    
    // Close the file
    nc_close(ncid);
    
    return 0;
}
