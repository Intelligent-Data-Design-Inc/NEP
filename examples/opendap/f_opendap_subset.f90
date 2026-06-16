! Copyright 2026, Intelligent Data Design Inc. All rights reserved.
! See the COPYRIGHT file for copyright and license information.

!>
!! @file f_opendap_subset.f90
!! @brief OPeNDAP client-side subsetting example in Fortran with timing.
!!
!! This is the Fortran equivalent of opendap_subset.c, demonstrating
!! client-side subsetting and performance characteristics.
!!
!! What this example does:
!! 1. Opens the full remote dataset without constraints
!! 2. Uses system_clock to time the open operation
!! 3. Demonstrates four different subset access patterns:
!!    - Single time slice: All lat/lon for time step 1 (using 1-based)
!!    - Time series: All time points at lat=46, lon=91
!!    - Regional subset: 3x10x10 cube using fixed-size array
!!    - Multiple scattered reads: Loop showing request overhead
!! 4. Times each read operation to show performance differences
!! 5. Closes the dataset with timing
!!
!! Key differences from constraint expressions:
!! - Full dataset metadata is available (original dimension sizes)
!! - Can make multiple different subset requests without reopening
!! - More flexible for data exploration and iterative analysis
!! - Network transfer includes only requested subset data
!!
!! Fortran-specific notes:
!! - nf90_get_var uses 1-based start indices: (/1, 1, 1/)
!! - system_clock provides portable timing for performance analysis
!! - allocatable arrays handle dynamic sizes based on dimensions
!!
!! Performance lessons:
!! - Opening the dataset has connection overhead (time it)
!! - One large request is faster than many small requests
!! - Reusing the open connection amortizes setup costs
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
  character(len=256) :: url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz"
  
  ! Data arrays for reading subsets
  real :: data_3d(3, 10, 10)
  real, allocatable :: data_slice(:,:)
  real, allocatable :: data_time(:)
  integer :: start(3), count(3)
  integer :: i, j, k
  real :: elapsed_time
  integer :: count_rate, count_start, count_end

  print *, "Fortran OPeNDAP Client-Side Subsetting Example"
  print *, "==============================================="
  print *
  print *, "Opening full dataset:"
  print *, "  ", trim(url)
  print *
  
  ! Open the dataset once and measure time
  call system_clock(count_start, count_rate)
  status = nf90_open(url, NF90_NOWRITE, ncid)
  call system_clock(count_end)
  elapsed_time = real(count_end - count_start) / real(count_rate)
  
  if (status /= NF90_NOERR) then
     print *, "Error opening dataset: ", trim(nf90_strerror(status))
     stop 1
  end if
  print *, "Open dataset: ", elapsed_time, " seconds"
  
  ! Get variable ID
  status = nf90_inq_varid(ncid, "sst", varid)
  if (status /= NF90_NOERR) then
     print *, "Error finding variable 'sst': ", trim(nf90_strerror(status))
     stop 1
  end if
  
  ! Get dimension sizes
  status = nf90_inquire_variable(ncid, varid, ndims=var_ndims, dimids=var_dimids)
  status = nf90_inquire_dimension(ncid, var_dimids(1), len=time_len)
  status = nf90_inquire_dimension(ncid, var_dimids(2), len=lat_len)
  status = nf90_inquire_dimension(ncid, var_dimids(3), len=lon_len)
  
  print *
  print *, "Full dataset dimensions:"
  print *, "  Time: ", time_len
  print *, "  Latitude: ", lat_len
  print *, "  Longitude: ", lon_len
  
  ! Read 1: Single time slice (all lat/lon for one time step)
  print *
  print *, "--- Request 1: Single time slice ---"
  
  ! Note: Fortran uses 1-based indexing in nf90_get_var
  start = (/ 1, 1, 1 /)  ! First time step
  count = (/ 1, lat_len, lon_len /)
  
  allocate(data_slice(lat_len, lon_len), stat=status)
  if (status /= 0) then
     print *, "Memory allocation failed"
     stop 1
  end if
  
  call system_clock(count_start)
  status = nf90_get_var(ncid, varid, data_slice, start=start, count=count)
  call system_clock(count_end)
  elapsed_time = real(count_end - count_start) / real(count_rate)
  
  if (status /= NF90_NOERR) then
     print *, "Error reading data: ", trim(nf90_strerror(status))
     deallocate(data_slice)
     stop 1
  end if
  
  print *, "Read full spatial slice (1 time step): ", elapsed_time, " seconds"
  print *, "Data shape: ", lat_len, " x ", lon_len
  print *, "Sample value [45,90]: ", data_slice(45, 90)
  
  deallocate(data_slice)
  
  ! Read 2: Time series at a specific location
  print *
  print *, "--- Request 2: Time series at single location ---"
  
  ! Fortran 1-based indexing: start at time 1, lat 46, lon 91
  start = (/ 1, 46, 91 /)
  count = (/ time_len, 1, 1 /)
  
  allocate(data_time(time_len), stat=status)
  if (status /= 0) then
     print *, "Memory allocation failed"
     stop 1
  end if
  
  call system_clock(count_start)
  status = nf90_get_var(ncid, varid, data_time, start=start, count=count)
  call system_clock(count_end)
  elapsed_time = real(count_end - count_start) / real(count_rate)
  
  if (status /= NF90_NOERR) then
     print *, "Error reading data: ", trim(nf90_strerror(status))
     deallocate(data_time)
     stop 1
  end if
  
  print *, "Read time series (all time steps, 1 location): ", elapsed_time, " seconds"
  print *, "Data shape: ", time_len, " time points"
  print *, "First 5 values: ", data_time(1:min(5, time_len))
  
  deallocate(data_time)
  
  ! Read 3: Small regional subset
  print *
  print *, "--- Request 3: Regional subset ---"
  
  start = (/ 1, 41, 86 /)  ! Fortran 1-based indexing
  count = (/ 3, 10, 10 /)   ! 3 time steps, 10x10 spatial
  
  call system_clock(count_start)
  status = nf90_get_var(ncid, varid, data_3d, start=start, count=count)
  call system_clock(count_end)
  elapsed_time = real(count_end - count_start) / real(count_rate)
  
  if (status /= NF90_NOERR) then
     print *, "Error reading data: ", trim(nf90_strerror(status))
     stop 1
  end if
  
  print *, "Read regional subset (3x10x10): ", elapsed_time, " seconds"
  print *, "Data shape: 3x10x10 (only showing last 2 dims)"
  print *, "Value at [1,6,6]: ", data_3d(1, 6, 6)
  
  ! Read 4: Multiple scattered requests
  print *
  print *, "--- Request 4: Multiple scattered reads ---"
  
  call system_clock(count_start)
  
  ! Make 3 separate requests for different regions
  do k = 1, 3
     start = (/ k, 41, 86 /)
     count = (/ 1, 10, 10 /)
     status = nf90_get_var(ncid, varid, data_3d, start=start, count=count)
     if (status /= NF90_NOERR) then
        print *, "Error on read ", k, ": ", trim(nf90_strerror(status))
     end if
  end do
  
  call system_clock(count_end)
  elapsed_time = real(count_end - count_start) / real(count_rate)
  
  print *, "Read 3 separate 10x10 slices: ", elapsed_time, " seconds"
  print *, "(Note: Multiple requests have overhead)"
  
  ! Close the dataset
  print *
  call system_clock(count_start)
  status = nf90_close(ncid)
  call system_clock(count_end)
  elapsed_time = real(count_end - count_start) / real(count_rate)
  print *, "Close dataset: ", elapsed_time, " seconds"
  
  if (status /= NF90_NOERR) then
     print *, "Error closing dataset: ", trim(nf90_strerror(status))
     stop 1
  end if
  
  print *
  print *, "Key takeaways:"
  print *, "- Open once, read multiple times for efficiency"
  print *, "- Use start/count to request only needed data"
  print *, "- Fewer large requests are more efficient than many small ones"
  print *, "- Remember Fortran uses 1-based indexing, OPeNDAP constraints use 0-based"

end program f_opendap_subset
