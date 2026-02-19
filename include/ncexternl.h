/*********************************************************************
 *   Copyright 2018, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Header$
 *********************************************************************/
/* Copied from netcdf-c v4.9.2 (commit 2360497 in NEP, 2025-09-29).
 * Source: https://github.com/Unidata/netcdf-c/blob/main/include/ncexternl.h
 * Do not edit. See docs/plan/v1.5.5-header-cleanup-map.md for elimination plan.
 * Category: TRANSITIVE - pulled in by nclist.h and ncuri.h.
 *   Never directly included by NEP .c files.
 * Elimination path: Eliminate when nc4internal.h is no longer needed. */

#ifndef NCEXTERNL_H
#define NCEXTERNL_H

#if defined(DLL_NETCDF) /* define when library is a DLL */
#  if defined(DLL_EXPORT) /* define when building the library */
#   define MSC_EXTRA __declspec(dllexport)
#  else
#   define MSC_EXTRA __declspec(dllimport)
#  endif
#else
#  define MSC_EXTRA
#endif	/* defined(DLL_NETCDF) */
#ifndef EXTERNL
# define EXTERNL MSC_EXTRA extern
#endif

#endif /*NCEXTERNL_H*/
