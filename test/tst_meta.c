#include <nep_meta.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int major, minor, patch;
    int ret;

    (void)argc;
    (void)argv;

    /* Verify the version string matches the three numeric macros. */
    ret = sscanf(NEP_VERSION, "%d.%d.%d", &major, &minor, &patch);
    if (ret != 3) {
        fprintf(stderr, "Error: NEP_VERSION \"%s\" is not a valid major.minor.patch string.\n",
                NEP_VERSION);
        return 1;
    }
    if (major != NEP_VERSION_MAJOR || minor != NEP_VERSION_MINOR || patch != NEP_VERSION_PATCH) {
        fprintf(stderr, "Error: NEP_VERSION \"%s\" does not match macros %d.%d.%d.\n",
                NEP_VERSION, NEP_VERSION_MAJOR, NEP_VERSION_MINOR, NEP_VERSION_PATCH);
        return 1;
    }
    if (major <= 0) {
        fprintf(stderr, "Error: NEP_VERSION_MAJOR must be positive.\n");
        return 1;
    }

    /* Verify the build date looks like an ISO date. */
    if (strlen(NEP_BUILD_DATE) != 10 ||
        NEP_BUILD_DATE[4] != '-' || NEP_BUILD_DATE[7] != '-' ||
        strncmp(NEP_BUILD_DATE, "20", 2) != 0) {
        fprintf(stderr, "Error: NEP_BUILD_DATE \"%s\" is not in YYYY-MM-DD form.\n",
                NEP_BUILD_DATE);
        return 1;
    }

    /* Verify compiler information is non-empty. */
    if (strlen(NEP_COMPILER_NAME) == 0) {
        fprintf(stderr, "Error: NEP_COMPILER_NAME is empty.\n");
        return 1;
    }
    if (strlen(NEP_COMPILER_VERSION) == 0) {
        fprintf(stderr, "Error: NEP_COMPILER_VERSION is empty.\n");
        return 1;
    }

    /* Verify at least one feature flag is defined and is 0 or 1. */
    if (NEP_HAS_PDB != 0 && NEP_HAS_PDB != 1) {
        fprintf(stderr, "Error: NEP_HAS_PDB is %d, expected 0 or 1.\n", NEP_HAS_PDB);
        return 1;
    }
    if (NEP_HAS_LZ4 != 0 && NEP_HAS_LZ4 != 1) {
        fprintf(stderr, "Error: NEP_HAS_LZ4 is %d, expected 0 or 1.\n", NEP_HAS_LZ4);
        return 1;
    }
    if (NEP_HAS_HDF5 != 1) {
        fprintf(stderr, "Error: NEP_HAS_HDF5 is %d, expected 1.\n", NEP_HAS_HDF5);
        return 1;
    }
    if (NEP_HAS_NEXTCDF4 != 0 && NEP_HAS_NEXTCDF4 != 1) {
        fprintf(stderr, "Error: NEP_HAS_NEXTCDF4 is %d, expected 0 or 1.\n", NEP_HAS_NEXTCDF4);
        return 1;
    }

    printf("OK: nep_meta.h version %s date %s compiler %s %s\n",
           NEP_VERSION, NEP_BUILD_DATE, NEP_COMPILER_NAME, NEP_COMPILER_VERSION);
    return 0;
}
