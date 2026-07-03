! Copyright 2026, Intelligent Data Design Inc. All rights reserved.
! See the COPYRIGHT file for copyright and license information.

!>
!! @file f_opendap_constraint.f90
!! @brief OPeNDAP constraint expression example in Fortran.
!!
!! This is the Fortran equivalent of opendap_constraint.c, demonstrating
!! server-side subsetting using OPeNDAP constraint expressions.
!!
!! What this example does:
!! 1. Builds a constrained URL: base_url + "?sst[0:2][0:88][0:179]"
!! 2. Opens the constrained dataset - only 3 time steps are visible
!! 3. Shows that nf90_inquire_dimension returns CONSTRAINED sizes
!! 4. Dynamically allocates array based on constrained dimensions
!! 5. Reads all constrained data with nf90_get_var
!! 6. Reopens with stride constraint [0:2:12] to sample every 2nd step
!!
!! IMPORTANT indexing note:
!! - Constraint expressions in the URL use 0-based indexing: [0:2]
!! - Fortran nf90_get_var uses 1-based indexing for start/count
!! - These are independent - constraints happen at the server,
!!   Fortran indexing happens in the client API
!!
!! Server-side subsetting reduces network transfer by having the OPeNDAP
!! server extract only requested data. The constrained dataset appears
!! to have only the subset dimensions, not the full original dimensions.
!!
!! **Learning Objectives:**
!! - Build constrained OPeNDAP URLs with server-side subsetting expressions
!! - Understand that constraint expressions use 0-based indexing (independent of Fortran)
!! - See how nf90_inquire_dimension() returns constrained sizes, not full sizes
!! - Use allocatable arrays to handle dynamically-sized constrained data
!! - Apply stride constraints for temporal sampling
!!
!! **Key Concepts:**
!! - **Server-Side Subsetting**: Constraint expression appended to URL (e.g., ?sst[0:2][0:88][0:179])
!!   tells the server to send only the requested subset — reduces network transfer
!! - **Constrained Dimensions**: After opening a constrained URL, dimension queries
!!   return the constrained sizes (e.g., time=3 instead of full time=1857)
!! - **0-Based Constraints**: URL constraints always use 0-based C-style indexing
!!   regardless of the client language; Fortran nf90_get_var still uses 1-based
!! - **Stride Expressions**: [start:stride:stop] syntax (e.g., [0:2:12]) samples
!!   every Nth element on the server side
!! - **Dynamic Allocation**: Fortran allocatable arrays adapt to constrained sizes
!!
!! **Prerequisites:**
!! - f_opendap_simple.f90 - Basic OPeNDAP access from Fortran
!! - opendap_constraint.c - C equivalent for comparison
!!
!! **Related Examples:**
!! - opendap_constraint.c - C equivalent
!! - f_opendap_simple.f90 - Simplest OPeNDAP access (no constraints)
!! - f_opendap_subset.f90 - Client-side subsetting (no URL constraints)
!!
!! **Compilation:**
!! @code
!! gfortran -o f_opendap_constraint f_opendap_constraint.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_opendap_constraint
!! @endcode
!!
!! **Expected Output:**
!! - Opens constrained SST dataset (3 time steps, full spatial) and prints
!!   constrained dimension sizes
!! - Reads and displays a sample of the constrained data
!! - Reopens with stride constraint and shows sampled time steps
!!
!! @author Edward Hartnett
!! @date 6/15/26
!

program f_opendap_constraint
  use netcdf
  implicit none

  integer :: ncid, varid, status
  integer :: var_ndims
  integer :: var_dimids(NF90_MAX_VAR_DIMS)
  integer :: dimlen
  character(len=NF90_MAX_NAME) :: dim_name
  
  ! Base URL and constrained URL
  character(len=512) :: base_url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz"
  character(len=1024) :: url
  
  ! Data array for reading constrained data
  real, allocatable :: data(:,:,:)
  integer :: i, j, k
  integer :: time_size, lat_size, lon_size
  integer :: total_elements

  ! Build constrained URL
  url = trim(base_url) // "?sst[0:2][0:88][0:179]"
  print *, "Fortran OPeNDAP Constraint: ", trim(url)
  print *

  ! Open constrained dataset
  status = nf90_open(url, NF90_NOWRITE, ncid)
  if (status /= NF90_NOERR) then
     print *, "Error: ", trim(nf90_strerror(status))
     stop 1
  end if

  ! Get variable and constrained dimension info
  status = nf90_inq_varid(ncid, "sst", varid)
  if (status /= NF90_NOERR) stop 1
  status = nf90_inquire_variable(ncid, varid, ndims=var_ndims, dimids=var_dimids)
  if (status /= NF90_NOERR) stop 1

  ! Print constrained dimensions (subset sizes)
  write(*, '(A)', advance='no') "Constrained dimensions: "
  do i = 1, var_ndims
     status = nf90_inquire_dimension(ncid, var_dimids(i), dim_name, dimlen)
     if (status == NF90_NOERR) then
        write(*, '(A,I0,A)', advance='no') trim(dim_name)//"=", dimlen, " "
        if (i == 1) time_size = dimlen
        if (i == 2) lat_size = dimlen
        if (i == 3) lon_size = dimlen
     end if
  end do
  print *
  
  ! Read and display sample of constrained data
  if (time_size > 0 .and. lat_size > 0 .and. lon_size > 0) then
     total_elements = time_size * lat_size * lon_size
     print *, "Reading ", total_elements, " elements..."

     allocate(data(time_size, lat_size, lon_size), stat=status)
     if (status /= 0) stop 1

     status = nf90_get_var(ncid, varid, data)
     if (status /= NF90_NOERR) stop 1

     print *, "Sample [1:5,1:5] of first time step:"
     do j = 1, min(5, lat_size)
        write(*, '(2X)', advance='no')
        do k = 1, min(5, lon_size)
           write(*, '(F7.2)', advance='no') data(1, j, k)
        end do
        print *
     end do
     deallocate(data)
  end if
  
  ! Demonstrate stride constraint
  print *
  url = trim(base_url) // "?sst[0:2:12][0:88][0:179]"
  print *, "Stride constraint: ", trim(url)
  status = nf90_close(ncid)
  status = nf90_open(url, NF90_NOWRITE, ncid)
  if (status /= NF90_NOERR) stop 1

  status = nf90_inq_varid(ncid, "sst", varid)
  status = nf90_inquire_variable(ncid, varid, dimids=var_dimids)
  status = nf90_inquire_dimension(ncid, var_dimids(1), len=dimlen)
  print *, "Time dimension with stride: ", dimlen, " (was 3 without stride)"

  status = nf90_close(ncid)
  if (status /= NF90_NOERR) stop 1
  print *, "Done."

end program f_opendap_constraint
