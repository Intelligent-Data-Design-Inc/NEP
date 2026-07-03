! Copyright 2026, Intelligent Data Design Inc. All rights reserved.
! See the COPYRIGHT file for copyright and license information.

!>
!! @file f_opendap_simple.f90
!! @brief Simple OPeNDAP example in Fortran.
!!
!! This is the Fortran equivalent of opendap_simple.c, demonstrating that
!! OPeNDAP remote access works identically in Fortran through the
!! NetCDF-Fortran library.
!!
!! What this example does:
!! 1. Opens a remote sea surface temperature dataset from test.opendap.org
!! 2. Queries dataset metadata using nf90_inquire
!! 3. Reads information about the 'sst' variable dimensions
!! 4. Reads a small 5x5 spatial subset from the first time step
!! 5. Closes the remote connection
!!
!! CRITICAL: Indexing differences to understand:
!! - OPeNDAP constraint expressions in URLs use 0-based indexing [0:2]
!! - NetCDF-Fortran API uses 1-based indexing: start=(/1, 1, 1/)
!! - This is automatically handled by the Fortran interface
!!
!! The example uses the same public test dataset as the C version:
!! sst.mnmean.nc.gz - NOAA Extended Reconstructed Sea Surface Temperature
!!
!! **Learning Objectives:**
!! - Open a remote OPeNDAP dataset from Fortran using nf90_open() with an HTTP URL
!! - Query dataset metadata (dimensions, variables, attributes) via nf90_inquire()
!! - Read a spatial subset using start/count arrays with 1-based Fortran indexing
!! - Understand that OPeNDAP access is transparent — same API as local files
!!
!! **Key Concepts:**
!! - **Transparent Remote Access**: nf90_open() accepts HTTP URLs just like local paths
!! - **1-Based Indexing**: Fortran start arrays begin at 1, not 0 as in C
!! - **Constraint vs Client Subsetting**: This example uses no URL constraints;
!!   subsetting is done client-side via start/count in nf90_get_var()
!! - **OPeNDAP Indexing**: URL constraint expressions use 0-based indexing,
!!   but Fortran API calls use 1-based — the library handles the translation
!!
!! **Prerequisites:**
!! - f_simple_2D.f90 - Basic Fortran NetCDF operations
!! - opendap_simple.c - C equivalent for comparison
!! - A NetCDF library built with OPeNDAP support
!!
!! **Related Examples:**
!! - opendap_simple.c - C equivalent
!! - f_opendap_constraint.f90 - Server-side subsetting from Fortran
!! - f_opendap_subset.f90 - Client-side subsetting patterns from Fortran
!!
!! **Compilation:**
!! @code
!! gfortran -o f_opendap_simple f_opendap_simple.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_opendap_simple
!! @endcode
!!
!! **Expected Output:**
!! - Opens remote SST dataset and prints dimension/variable counts
!! - Reads and displays a 5x5 spatial subset from the first time step
!!
!! @author Edward Hartnett
!! @date 6/15/26
!

program f_opendap_simple
  use netcdf
  implicit none

  integer :: ncid, varid, status
  integer :: ndims, nvars, natts, unlimdimid
  integer :: var_ndims, var_natts, var_dimids(NF90_MAX_VAR_DIMS)
  integer :: xtype
  integer :: dimlen
  character(len=NF90_MAX_NAME) :: var_name, dim_name
  
  ! Public OPeNDAP test server dataset
  character(len=256) :: url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz"
  
  ! Data array for reading subset
  real :: data(5, 5)
  integer :: start(3), count(3)
  integer :: i, j

  print *, "Fortran OPeNDAP Simple: ", trim(url)
  print *

  ! Open and get metadata
  status = nf90_open(url, NF90_NOWRITE, ncid)
  if (status /= NF90_NOERR) then
     print *, "Error: ", trim(nf90_strerror(status))
     stop 1
  end if

  status = nf90_inquire(ncid, ndims, nvars, natts, unlimdimid)
  if (status /= NF90_NOERR) stop 1
  print *, "Dataset: ", ndims, " dims, ", nvars, " vars, ", natts, " atts, unlimdim=", unlimdimid

  ! Print dimensions compactly
  write(*, '(A)', advance='no') "Dimensions: "
  do i = 1, ndims
     status = nf90_inquire_dimension(ncid, i, dim_name, dimlen)
     if (status == NF90_NOERR) then
        write(*, '(A,I0,A,I0,A)', advance='no') trim(dim_name)//"=", dimlen, " "
     end if
  end do
  print *
  
  ! Get variable 'sst' info and read subset
  status = nf90_inq_varid(ncid, "sst", varid)
  if (status == NF90_NOERR) then
     status = nf90_inquire_variable(ncid, varid, var_name, xtype, var_ndims, var_dimids, var_natts)
     if (status == NF90_NOERR) then
        write(*, '(A,A,A,I0,A,I0,A,I0,A)', advance='no') "Variable '"//trim(var_name)//"': type=", xtype, ", ndims=", var_ndims, ", natts=", var_natts, ", shape=["
        do i = 1, var_ndims
           status = nf90_inquire_dimension(ncid, var_dimids(i), len=dimlen)
           write(*, '(I0,A)', advance='no') dimlen, merge(",","]", i<var_ndims)
        end do
        print *
     end if

     ! Read 5x5 subset (Fortran 1-based indexing)
     start = (/ 1, 1, 1 /)
     count = (/ 1, 5, 5 /)
     print *, "Reading subset [1:1][1:5][1:5]:"

     status = nf90_get_var(ncid, varid, data, start=start, count=count)
     if (status /= NF90_NOERR) then
        print *, "Error: ", trim(nf90_strerror(status))
     else
        do i = 1, 5
           write(*, '(2X)', advance='no')
           do j = 1, 5
              write(*, '(F7.2)', advance='no') data(i, j)
           end do
           print *
        end do
     end if
  else
     print *, "Variable 'sst' not found."
  end if
  
  ! Close
  status = nf90_close(ncid)
  if (status /= NF90_NOERR) stop 1
  print *, "Done."

end program f_opendap_simple
