#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Import the actual function from the production code */
extern int H5Z_filter_lz4(unsigned int flags, size_t cd_nelmts,
                          const unsigned int cd_values[], size_t nbytes,
                          size_t *buf_size, void **buf);

START_TEST(test_buffer_reads_never_exceed_declared_length)
{
    /* Security invariant: Buffer reads never exceed the declared length */
    
    /* Payloads: exploit case, boundary case, valid case */
    struct {
        size_t input_size;
        size_t declared_size;
        const char *description;
    } test_cases[] = {
        /* Exploit case: origSize = UINT64_MAX, blockSize = 0 */
        { 12, 0, "exploit_overflow" },
        /* Boundary case: origSize = 1GB, blockSize = 1GB */
        { 12, 1024*1024*1024, "boundary_1gb" },
        /* Valid case: small reasonable sizes */
        { 12, 1024, "valid_small" }
    };
    
    for (int i = 0; i < 3; i++) {
        /* Create malicious input buffer */
        size_t test_size = test_cases[i].input_size;
        void *input_buf = malloc(test_size);
        void *output_buf = NULL;
        size_t output_size = test_cases[i].declared_size;
        
        if (input_buf == NULL) {
            ck_abort_msg("Failed to allocate test buffer");
        }
        
        /* Craft malicious header with oversized origSize */
        uint64_t *i64Buf = (uint64_t *)input_buf;
        uint32_t *i32Buf = (uint32_t *)((char *)input_buf + 8);
        
        /* Set origSize to test value (big-endian) */
        *i64Buf = 0xFFFFFFFFFFFFFFFF; /* Maximum value for exploit case */
        if (i == 1) *i64Buf = 1024*1024*1024; /* 1GB for boundary case */
        if (i == 2) *i64Buf = 1024; /* 1KB for valid case */
        
        /* Set blockSize */
        *i32Buf = 0; /* Zero for exploit, will be adjusted in code */
        if (i == 1) *i32Buf = 1024*1024*1024; /* 1GB block */
        if (i == 2) *i32Buf = 512; /* 512 byte block */
        
        /* Call the actual filter function */
        int result = H5Z_filter_lz4(
            0, /* flags */
            0, /* cd_nelmts */
            NULL, /* cd_values */
            test_size, /* input size */
            &output_size, /* declared output buffer size */
            &output_buf /* output buffer */
        );
        
        /* Security check: Either the function should fail or output size should be reasonable */
        if (result >= 0) {
            /* If function succeeded, verify output size is reasonable */
            ck_assert_msg(output_size <= test_cases[i].declared_size,
                         "Output size %zu exceeds declared size %zu for case %s",
                         output_size, test_cases[i].declared_size, test_cases[i].description);
        }
        
        free(input_buf);
        if (output_buf != NULL) {
            free(output_buf);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_buffer_reads_never_exceed_declared_length);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}