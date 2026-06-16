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

  print *, "Fortran OPeNDAP Constraint Expression Example"
  print *, "=============================================="
  print *
  print *, "Base URL: ", trim(base_url)
  
  ! Build URL with constraint expression (first 3 time steps)
  url = trim(base_url) // "?sst[0:2][0:88][0:179]"
  print *, "Constraint: sst[0:2][0:88][0:179]"
  print *, "Full URL: ", trim(url)
  print *
  
  print *, "Opening dataset with constraint expression..."
  status = nf90_open(url, NF90_NOWRITE, ncid)
  if (status /= NF90_NOERR) then
     print *, "Error opening dataset: ", trim(nf90_strerror(status))
     stop 1
  end if
  print *, "Dataset opened successfully."
  print *
  
  ! Get variable information
  status = nf90_inq_varid(ncid, "sst", varid)
  if (status /= NF90_NOERR) then
     print *, "Error finding variable 'sst': ", trim(nf90_strerror(status))
     stop 1
  end if
  
  status = nf90_inquire_variable(ncid, varid, ndims=var_ndims, dimids=var_dimids)
  if (status /= NF90_NOERR) then
     print *, "Error inquiring variable: ", trim(nf90_strerror(status))
     stop 1
  end if
  
  print *, "Variable 'sst' has ", var_ndims, " dimensions:"
  
  ! Query the constrained dimensions
  time_size = 0
  lat_size = 0
  lon_size = 0
  
  do i = 1, var_ndims
     status = nf90_inquire_dimension(ncid, var_dimids(i), dim_name, dimlen)
     if (status == NF90_NOERR) then
        print *, "  Dimension ", i, ": ", trim(dim_name), " = ", dimlen
        if (i == 1) time_size = dimlen
        if (i == 2) lat_size = dimlen
        if (i == 3) lon_size = dimlen
     end if
  end do
  print *
  
  print *, "Note: The dimension sizes reflect the CONSTRAINED subset,"
  print *, "not the original full dataset."
  print *
  
  ! Read all the constrained data
  if (time_size > 0 .and. lat_size > 0 .and. lon_size > 0) then
     total_elements = time_size * lat_size * lon_size
     print *, "Reading ", total_elements, " total elements..."
     
     allocate(data(time_size, lat_size, lon_size), stat=status)
     if (status /= 0) then
        print *, "Memory allocation failed"
        stop 1
     end if
     
     status = nf90_get_var(ncid, varid, data)
     if (status /= NF90_NOERR) then
        print *, "Error reading variable: ", trim(nf90_strerror(status))
        deallocate(data)
        stop 1
     end if
     
     ! Print sample of first time step
     print *, "Sample data from first time step [1:5,1:5]:"
     do j = 1, min(5, lat_size)
        write(*, '(A,I2,A)', advance='no') "  Lat ", j, ": "
        do k = 1, min(5, lon_size)
           write(*, '(F7.2)', advance='no') data(1, j, k)
        end do
        print *
     end do
     
     deallocate(data)
  end if
  
  ! Demonstrate another constraint with stride
  print *
  print *, "----------------------------------------"
  print *
  
  ! Build URL with stride constraint (every 2nd time step)
  url = trim(base_url) // "?sst[0:2:12][0:88][0:179]"
  print *, "New constraint with stride (every 2nd time step):"
  print *, "URL: ", trim(url)
  print *
  
  status = nf90_close(ncid)
  if (status /= NF90_NOERR) then
     print *, "Error closing dataset: ", trim(nf90_strerror(status))
  end if
  
  status = nf90_open(url, NF90_NOWRITE, ncid)
  if (status /= NF90_NOERR) then
     print *, "Error opening dataset: ", trim(nf90_strerror(status))
     stop 1
  end if
  
  status = nf90_inq_varid(ncid, "sst", varid)
  status = nf90_inquire_variable(ncid, varid, dimids=var_dimids)
  status = nf90_inquire_dimension(ncid, var_dimids(1), len=dimlen)
  
  print *, "Time dimension after stride constraint: ", dimlen
  print *, "(Should be smaller than 3 from previous constraint)"
  
  status = nf90_close(ncid)
  if (status /= NF90_NOERR) then
     print *, "Error closing dataset: ", trim(nf90_strerror(status))
     stop 1
  end if
  
  print *
  print *, "Example completed successfully."
  print *
  print *, "Note: OPeNDAP constraint expressions use 0-based indexing,"
  print *, "but the NetCDF-Fortran API uses 1-based indexing for data access."

end program f_opendap_constraint
