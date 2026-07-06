! Copyright 2026, Intelligent Data Design Inc. All rights reserved.
! See the COPYRIGHT file for copyright and license information.

!>
!! @file f_opendap_subset.f90
!! @brief OPeNDAP client-side subsetting example in Fortran.
!!
!! This is the Fortran equivalent of opendap_subset.c, demonstrating
!! client-side subsetting using start/count arrays.
!!
!! What this example does:
!! 1. Opens the full remote dataset without constraints
!! 2. Demonstrates four different subset access patterns:
!!    - Single time slice: All lat/lon for time step 1 (using 1-based)
!!    - Time series: All time points at lat=46, lon=91
!!    - Regional subset: 3x10x10 cube using fixed-size array
!!    - Multiple scattered reads: Loop showing multiple requests
!! 3. Closes the dataset
!!
!! Key differences from constraint expressions:
!! - Full dataset metadata is available (original dimension sizes)
!! - Can make multiple different subset requests without reopening
!! - More flexible for data exploration and iterative analysis
!! - Network transfer includes only requested subset data
!!
!! Fortran-specific notes:
!! - nf90_get_var uses 1-based start indices: (/1, 1, 1/)
!! - allocatable arrays handle dynamic sizes based on dimensions
!!
!! **Learning Objectives:**
!! - Open a full remote dataset without constraint expressions from Fortran
!! - Use start/count arrays (1-based) for multiple targeted data requests
!! - Implement four different access patterns
!!   (time slice, time series, regional, scattered)
!! - Understand the trade-offs between client-side and server-side subsetting
!! - Recognize the overhead of many small requests vs fewer large requests
!!
!! **Key Concepts:**
!! - **Client-Side Subsetting**: Open the full
!!   dataset once, then use start/count
!!   in nf90_get_var() calls to select different subregions on each read
!! - **Time Slice Access**: Read all spatial points for one time step
!!   (start=(/1,1,1/), count=(/lon_len,lat_len,1/) in Fortran column-major)
!! - **Time Series Access**: Read all time points at one spatial location
!!   (start=(/lon,lat,1/), count=(/1,1,time_len/))
!! - **Regional Subset**: Read a small 3D cube from the full dataset
!! - **Multiple Requests**: One nf90_open()
!!   supports unlimited nf90_get_var() calls
!!   with different start/count values — no need to reopen for each subset
!! - **1-Based Indexing**: Fortran start arrays
!!   begin at 1, unlike C's 0-based indexing
!!
!! **Prerequisites:**
!! - f_opendap_simple.f90 - Basic OPeNDAP access from Fortran
!! - opendap_subset.c - C equivalent for comparison
!! - f_simple_2D.f90 - Understanding start/count subsetting for local files
!!
!! **Related Examples:**
!! - opendap_subset.c - C equivalent
!! - f_opendap_simple.f90 - Simplest OPeNDAP access (single subset)
!! - f_opendap_constraint.f90 - Server-side subsetting via URL constraints
!!
!! **Compilation:**
!! @code
!! gfortran -o f_opendap_subset f_opendap_subset.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_opendap_subset
!! @endcode
!!
!! **Expected Output:**
!! - Opens remote SST dataset and prints full dimension sizes
!! - Demonstrates four access patterns with data summaries:
!!   single time slice, time series at a point, regional 3D cube,
!!   and multiple scattered requests
!!
!! @note Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to Writing
!! High-Performance Programs for Scientific Data Management, Second Edition"
!! (https://www.amazon.com/dp/B0H7Q1Z75L)
!!
!! @author Edward Hartnett
!! @date 6/15/26
!

program f_opendap_subset
  use netcdf
  implicit none

  integer :: ncid, varid, status
  integer :: var_ndims
  integer :: var_dimids(NF90_MAX_VAR_DIMS)
  integer :: time_len, lat_len, lon_len
  
  ! URL without constraint - we'll subset client-side
  character(len=256) :: url = &
       "http://test.opendap.org" // &
       "/dap/data/nc/sst.mnmean.nc.gz"
  
  ! Data arrays for reading subsets
  real :: data_3d(3, 10, 10)
  real, allocatable :: data_slice(:,:)
  real, allocatable :: data_time(:)
  integer :: start(3), count(3)
  integer :: i, j, k

  print *, "Fortran OPeNDAP Client-Side: ", trim(url)
  print *

  ! Open and get dimensions
  status = nf90_open(url, NF90_NOWRITE, ncid)
  if (status /= NF90_NOERR) then
     print *, "Error: ", trim(nf90_strerror(status))
     stop 1
  end if

  status = nf90_inq_varid(ncid, "sst", varid)
  if (status /= NF90_NOERR) stop 1

  status = nf90_inquire_variable(ncid, varid, &
       ndims=var_ndims, dimids=var_dimids)
  status = nf90_inquire_dimension(ncid, var_dimids(1), len=time_len)
  status = nf90_inquire_dimension(ncid, var_dimids(2), len=lat_len)
  status = nf90_inquire_dimension(ncid, var_dimids(3), len=lon_len)
  print *, "Dimensions: time=", time_len, ", lat=", lat_len, ", lon=", lon_len
  
  ! Request 1: Single time slice
  print *
  print *, "1. Single time slice (all lat/lon for t=1):"
  start = (/ 1, 1, 1 /)
  count = (/ 1, lat_len, lon_len /)

  allocate(data_slice(lat_len, lon_len), stat=status)
  if (status /= 0) stop 1

  status = nf90_get_var(ncid, varid, data_slice, start=start, count=count)
  if (status /= NF90_NOERR) stop 1

  print *, "Shape ", lat_len, "x", lon_len, &
       ", sample[45,90]=", data_slice(45, 90)
  deallocate(data_slice)
  
  ! Request 2: Time series at one location
  print *
  print *, "2. Time series at (lat=46, lon=91):"
  start = (/ 1, 46, 91 /)
  count = (/ time_len, 1, 1 /)

  allocate(data_time(time_len), stat=status)
  if (status /= 0) stop 1

  status = nf90_get_var(ncid, varid, data_time, start=start, count=count)
  if (status /= NF90_NOERR) stop 1

  print *, time_len, " points, first 5: ", data_time(1:min(5, time_len))
  deallocate(data_time)
  
  ! Request 3: Regional subset
  print *
  print *, "3. Regional subset (3x10x10 at t=1, lat=41, lon=86):"
  start = (/ 1, 41, 86 /)
  count = (/ 3, 10, 10 /)

  status = nf90_get_var(ncid, varid, data_3d, start=start, count=count)
  if (status /= NF90_NOERR) stop 1

  print *, "3x10x10, value[1,6,6]=", data_3d(1, 6, 6)
  
  ! Request 4: Multiple scattered reads
  print *
  print *, "4. Multiple scattered reads (3 separate 10x10 slices):"

  do k = 1, 3
     start = (/ k, 41, 86 /)
     count = (/ 1, 10, 10 /)
     status = nf90_get_var(ncid, varid, data_3d, start=start, count=count)
  end do
  print *, "Complete"
  
  ! Close
  print *
  status = nf90_close(ncid)
  if (status /= NF90_NOERR) stop 1
  print *, "Done."

end program f_opendap_subset
