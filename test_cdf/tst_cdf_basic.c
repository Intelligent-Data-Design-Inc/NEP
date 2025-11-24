/**
 * @file tst_cdf_basic.c
 * @brief Basic test for NASA CDF library integration
 * 
 * This test validates that the NASA CDF library is properly installed
 * and functional. It creates a CDF test file, then reads it back to
 * verify library integration.
 * 
 * This test is part of v1.3.0 Sprint 2 and validates CDF library
 * detection only. UDF implementation comes in Sprint 3.
 * 
 * @author Edward Hartnett
 * @date 2025-11-23
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "cdf.h"

#define TEST_FILE "tst_cdf_simple.cdf"
#define EXPECTED_VAR_NAME "temperature"
#define DIM_SIZE 10

/**
 * @brief Create test CDF file
 * 
 * Creates a minimal CDF file with test data for validation.
 * 
 * @return 0 on success, non-zero on failure
 */
static int create_test_file(void)
{
    CDFid id;
    CDFstatus status;
    long dimSizes[1] = {DIM_SIZE};
    long varNum;
    long attrNum;
    long recNum = 0;
    long indices[1] = {0};
    long intervals[1] = {1};
    long counts[1] = {DIM_SIZE};
    float data[DIM_SIZE];
    int i;
    char error_text[CDF_STATUSTEXT_LEN+1];

    printf("Creating test CDF file: %s\n", TEST_FILE);

    /* Remove existing file if it exists */
    remove(TEST_FILE);

    /* Initialize test data */
    for (i = 0; i < DIM_SIZE; i++) {
        data[i] = 20.0f + (float)i * 0.5f;  /* Temperature values 20.0 to 24.5 */
    }

    /* Create the CDF file */
    status = CDFcreateCDF(TEST_FILE, &id);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to create CDF: %s\n", error_text);
        return 1;
    }

    /* Add global attribute: title */
    status = CDFcreateAttr(id, "title", GLOBAL_SCOPE, &attrNum);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to create title attribute: %s\n", error_text);
        CDFcloseCDF(id);
        return 1;
    }
    
    const char *title = "NEP CDF Test File";
    status = CDFputAttrgEntry(id, attrNum, 0, CDF_CHAR, strlen(title), (void *)title);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to write title attribute: %s\n", error_text);
        CDFcloseCDF(id);
        return 1;
    }

    /* Add global attribute: institution */
    status = CDFcreateAttr(id, "institution", GLOBAL_SCOPE, &attrNum);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to create institution attribute: %s\n", error_text);
        CDFcloseCDF(id);
        return 1;
    }
    
    const char *institution = "Intelligent Data Design, Inc.";
    status = CDFputAttrgEntry(id, attrNum, 0, CDF_CHAR, strlen(institution), (void *)institution);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to write institution attribute: %s\n", error_text);
        CDFcloseCDF(id);
        return 1;
    }

    /* Create a zVariable (temperature) */
    status = CDFcreatezVar(id, EXPECTED_VAR_NAME, CDF_FLOAT, 1, 1L, dimSizes, 
                           VARY, dimSizes, &varNum);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to create zVariable: %s\n", error_text);
        CDFcloseCDF(id);
        return 1;
    }

    /* Add variable attribute: units */
    status = CDFcreateAttr(id, "units", VARIABLE_SCOPE, &attrNum);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to create units attribute: %s\n", error_text);
        CDFcloseCDF(id);
        return 1;
    }
    
    const char *units = "degrees_Celsius";
    status = CDFputAttrzEntry(id, attrNum, varNum, CDF_CHAR, strlen(units), (void *)units);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to write units attribute: %s\n", error_text);
        CDFcloseCDF(id);
        return 1;
    }

    /* Add variable attribute: long_name */
    status = CDFcreateAttr(id, "long_name", VARIABLE_SCOPE, &attrNum);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to create long_name attribute: %s\n", error_text);
        CDFcloseCDF(id);
        return 1;
    }
    
    const char *long_name = "Air Temperature";
    status = CDFputAttrzEntry(id, attrNum, varNum, CDF_CHAR, strlen(long_name), (void *)long_name);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to write long_name attribute: %s\n", error_text);
        CDFcloseCDF(id);
        return 1;
    }

    /* Write data to the variable */
    status = CDFhyperPutzVarData(id, varNum, recNum, 1L, 1L, indices, counts, intervals, data);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to write variable data: %s\n", error_text);
        CDFcloseCDF(id);
        return 1;
    }

    /* Close the CDF file */
    status = CDFcloseCDF(id);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to close CDF: %s\n", error_text);
        return 1;
    }

    printf("  ✓ Successfully created test file\n\n");
    return 0;
}

/**
 * @brief Main test function
 * 
 * Creates CDF test file, then reads it back to validate library integration.
 * 
 * @return 0 on success, non-zero on failure
 */
int main(void)
{
    CDFid id;
    CDFstatus status;
    char error_text[CDF_STATUSTEXT_LEN+1];
    long encoding, majority, numDims;
    long numAttrs, numzVars, numrVars;
    char varName[CDF_VAR_NAME_LEN256+1];
    long varNum, dataType;
    long attrNum, attrScope;
    char attrName[CDF_ATTR_NAME_LEN256+1];
    int errors = 0;

    printf("=== NEP CDF Library Integration Test ===\n\n");

    /* Create test file */
    if (create_test_file() != 0) {
        fprintf(stderr, "ERROR: Failed to create test file\n");
        return 1;
    }

    /* Open the CDF file */
    printf("Opening CDF file: %s\n", TEST_FILE);
    status = CDFopenCDF(TEST_FILE, &id);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to open CDF file: %s\n", error_text);
        return 1;
    }
    printf("  ✓ Successfully opened CDF file\n\n");

    /* Get basic file information using CDFlib */
    printf("Querying file metadata...\n");
    status = CDFlib(SELECT_, CDF_, id,
                    GET_, CDF_ENCODING_, &encoding,
                    GET_, CDF_MAJORITY_, &majority,
                    GET_, CDF_NUMrVARS_, &numrVars,
                    GET_, CDF_NUMzVARS_, &numzVars,
                    GET_, CDF_NUMATTRS_, &numAttrs,
                    NULL_);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to inquire CDF: %s\n", error_text);
        CDFcloseCDF(id);
        return 1;
    }

    printf("  File encoding: %ld\n", encoding);
    printf("  Majority: %s\n", (majority == ROW_MAJOR) ? "ROW_MAJOR" : "COLUMN_MAJOR");
    printf("  Number of attributes: %ld\n", numAttrs);
    printf("  ✓ Successfully queried file metadata\n\n");

    printf("Variables in file:\n");
    printf("  zVariables: %ld\n", numzVars);
    printf("  rVariables: %ld\n", numrVars);

    /* Validate we have expected number of variables */
    if (numzVars != 1) {
        fprintf(stderr, "ERROR: Expected 1 zVariable, found %ld\n", numzVars);
        errors++;
    } else {
        printf("  ✓ Found expected number of zVariables\n");
    }

    /* Get information about the first zVariable */
    if (numzVars > 0) {
        printf("\nQuerying zVariable information...\n");
        varNum = 0;
        status = CDFlib(SELECT_, CDF_, id,
                        SELECT_, zVAR_, varNum,
                        GET_, zVAR_NAME_, varName,
                        GET_, zVAR_DATATYPE_, &dataType,
                        GET_, zVAR_NUMDIMS_, &numDims,
                        NULL_);
        if (status != CDF_OK) {
            CDFgetStatusText(status, error_text);
            fprintf(stderr, "ERROR: Failed to inquire zVariable: %s\n", error_text);
            errors++;
        } else {
            printf("  Variable name: %s\n", varName);
            printf("  Data type: %ld\n", dataType);
            printf("  Number of dimensions: %ld\n", numDims);
            
            /* Validate variable name */
            if (strcmp(varName, EXPECTED_VAR_NAME) != 0) {
                fprintf(stderr, "ERROR: Expected variable name '%s', found '%s'\n", 
                        EXPECTED_VAR_NAME, varName);
                errors++;
            } else {
                printf("  ✓ Variable name matches expected value\n");
            }
        }
    }

    /* List global attributes */
    printf("\nGlobal attributes:\n");
    for (attrNum = 0; attrNum < numAttrs; attrNum++) {
        status = CDFlib(SELECT_, CDF_, id,
                        SELECT_, ATTR_, attrNum,
                        GET_, ATTR_NAME_, attrName,
                        GET_, ATTR_SCOPE_, &attrScope,
                        NULL_);
        if (status != CDF_OK) {
            continue;  /* Skip on error */
        }
        
        if (attrScope == GLOBAL_SCOPE) {
            printf("  %s (global attribute)\n", attrName);
        }
    }
    printf("  ✓ Successfully listed global attributes\n");

    /* Close the CDF file */
    printf("\nClosing CDF file...\n");
    status = CDFcloseCDF(id);
    if (status != CDF_OK) {
        CDFgetStatusText(status, error_text);
        fprintf(stderr, "ERROR: Failed to close CDF: %s\n", error_text);
        return 1;
    }
    printf("  ✓ Successfully closed CDF file\n\n");

    /* Clean up test file */
    /* Commented out for Sprint 4 - keep file for UDF testing */
    /* printf("\nCleaning up test file...\n");
    if (remove(TEST_FILE) != 0) {
        fprintf(stderr, "WARNING: Failed to remove test file %s\n", TEST_FILE);
    } else {
        printf("  ✓ Test file removed\n");
    } */
    printf("\nKeeping test file for UDF testing: %s\n", TEST_FILE);

    /* Print test summary */
    printf("\n=== Test Summary ===\n");
    if (errors == 0) {
        printf("✓ All tests PASSED\n");
        printf("CDF library integration validated successfully.\n");
        return 0;
    } else {
        printf("✗ %d test(s) FAILED\n", errors);
        return 1;
    }
}
