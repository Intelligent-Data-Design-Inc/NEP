/**
 * @file
 * @internal This file handles the AB dimension functions.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date Nov 13, 2025
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

/* #include "nc4internal.h" */
/* #include "nc4dispatch.h" */

/* /\** */
/*  * @internal Dims cannot be defined for AB files. */
/*  * */
/*  * @param ncid Ignored. */
/*  * @param name Ignored. */
/*  * @param len Ignored. */
/*  * @param idp Ignored. */
/*  * */
/*  * @return NC_EPERM Can't define dims. */
/*  * @author Ed Hartnett */
/*  *\/ */
/* int */
/* AB_def_dim(int ncid, const char *name, size_t len, int *idp) */
/* { */
/*    return NC_EPERM; */
/* } */

/* /\** */
/*  * @internal Not allowed for AB. */
/*  * */
/*  * @param ncid Ignored. */
/*  * @param dimid Ignored. */
/*  * @param name Ignored. */
/*  * */
/*  * @return NC_NEPERM Can't write to AB file. */
/*  * @author Ed Hartnett */
/*  *\/ */
/* int */
/* AB_rename_dim(int ncid, int dimid, const char *name) */
/* { */
/*    return NC_EPERM; */
/* } */
