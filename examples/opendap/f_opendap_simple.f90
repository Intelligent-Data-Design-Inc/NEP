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

  print *, "Fortran OPeNDAP Simple Example"
  print *, "=============================="
  print *
  print *, "Opening remote dataset:"
  print *, "  " // trim(url)
  print *
  
  ! Open the remote dataset
  status = nf90_open(url, NF90_NOWRITE, ncid)
  if (status /= NF90_NOERR) then
     print *, "Error opening dataset: ", trim(nf90_strerror(status))
     stop 1
  end if
  print *, "Dataset opened successfully."
  print *
  
  ! Query dataset metadata
  status = nf90_inquire(ncid, ndims, nvars, natts, unlimdimid)
  if (status /= NF90_NOERR) then
     print *, "Error inquiring dataset: ", trim(nf90_strerror(status))
     stop 1
  end if
  
  print *, "Dataset contains:"
  print *, "  Dimensions: ", ndims
  print *, "  Variables: ", nvars
  print *, "  Global attributes: ", natts
  print *, "  Unlimited dimension ID: ", unlimdimid
  print *
  
  ! Query dimension information
  print *, "Dimensions:"
  do i = 1, ndims
     status = nf90_inquire_dimension(ncid, i, dim_name, dimlen)
     if (status == NF90_NOERR) then
        print *, "  [", i, "] ", trim(dim_name), " = ", dimlen
     end if
  end do
  print *
  
  ! Get variable ID for 'sst'
  status = nf90_inq_varid(ncid, "sst", varid)
  if (status == NF90_NOERR) then
     print *, "Found variable 'sst' (ID: ", varid, ")"
     
     ! Query variable information
     status = nf90_inquire_variable(ncid, varid, var_name, xtype, var_ndims, var_dimids, var_natts)
     if (status == NF90_NOERR) then
        print *, "  Type: ", xtype
        print *, "  Number of dimensions: ", var_ndims
        print *, "  Number of attributes: ", var_natts
        print *, "  Dimension IDs: ", var_dimids(1:var_ndims)
     end if
     
     ! Read a small subset using Fortran 1-based indexing
     ! Note: NetCDF-Fortran uses 1-based start indices automatically
     start = (/ 1, 1, 1 /)  ! First time step, first lat, first lon
     count = (/ 1, 5, 5 /)  ! 1 time step, 5x5 spatial
     
     print *
     print *, "Reading subset [1:1][1:5][1:5] (Fortran 1-based):"
     
     status = nf90_get_var(ncid, varid, data, start=start, count=count)
     if (status /= NF90_NOERR) then
        print *, "Error reading variable: ", trim(nf90_strerror(status))
     else
        print *, "  Sample values:"
        do i = 1, 5
           write(*, '(A,I2,A)', advance='no') "    Row ", i, ": "
           do j = 1, 5
              write(*, '(F7.2)', advance='no') data(i, j)
           end do
           print *
        end do
     end if
  else
     print *, "Variable 'sst' not found in dataset."
  end if
  
  ! Close the dataset
  status = nf90_close(ncid)
  if (status /= NF90_NOERR) then
     print *, "Error closing dataset: ", trim(nf90_strerror(status))
     stop 1
  end if
  
  print *
  print *, "Dataset closed successfully."
  print *
  print *, "Note: Fortran uses 1-based indexing for the API,"
  print *, "      but OPeNDAP constraint expressions use 0-based indexing."

end program f_opendap_simple
