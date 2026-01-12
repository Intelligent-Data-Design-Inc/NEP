#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>

#define ERR_CHECK(ret) do { if ((ret) != NC_NOERR) { \
    printf("Error at line %d: %s\n", __LINE__, nc_strerror(ret)); \
    return 1; \
} } while(0)

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <geotiff_file>\n", argv[0]);
        return 1;
    }
    
    printf("Testing CRS extraction with: %s\n", argv[1]);
    
    int ncid;
    int ret = nc_open(argv[1], NC_NOWRITE, &ncid);
    if (ret != NC_NOERR) {
        printf("Could not open file: %s\n", nc_strerror(ret));
        return 1;
    }
    
    printf("File opened successfully\n");
    
    // Check global attributes for CRS information
    int natts;
    ret = nc_inq_natts(ncid, &natts);
    ERR_CHECK(ret);
    
    printf("Number of global attributes: %d\n", natts);
    
    char att_name[NC_MAX_NAME + 1];
    nc_type att_type;
    size_t att_len;
    int found_crs_atts = 0;
    
    for (int i = 0; i < natts; i++) {
        ret = nc_inq_attname(ncid, NC_GLOBAL, i, att_name);
        ERR_CHECK(ret);
        
        ret = nc_inq_att(ncid, NC_GLOBAL, att_name, &att_type, &att_len);
        ERR_CHECK(ret);
        
        if (strncmp(att_name, "geotiff_", 9) == 0) {
            found_crs_atts++;
            printf("  CRS Attribute: %s (type=%d, len=%zu)\n", att_name, att_type, att_len);
            
            // Print the value
            if (att_type == NC_CHAR) {
                char *value = malloc(att_len + 1);
                if (!value) {
                    printf("Memory allocation failed\n");
                    nc_close(ncid);
                    return 1;
                }
                nc_get_att_text(ncid, NC_GLOBAL, att_name, value);
                value[att_len] = '\0';
                printf("    Value: %s\n", value);
                free(value);
            } else if (att_type == NC_INT) {
                int value;
                nc_get_att_int(ncid, NC_GLOBAL, att_name, &value);
                printf("    Value: %d\n", value);
            } else if (att_type == NC_DOUBLE) {
                double value;
                nc_get_att_double(ncid, NC_GLOBAL, att_name, &value);
                printf("    Value: %.6f\n", value);
            }
        }
    }
    
    if (found_crs_atts == 0) {
        printf("No CRS attributes found (this is expected for files without CRS)\n");
    } else {
        printf("Found %d CRS attributes\n", found_crs_atts);
    }
    
    nc_close(ncid);
    printf("Test completed successfully\n");
    return 0;
}
