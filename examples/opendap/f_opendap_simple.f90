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
