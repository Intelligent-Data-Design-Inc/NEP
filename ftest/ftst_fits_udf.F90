!> @file
!!
!! This is a test program for the NEP FITS User Defined Format (UDF)
!! handler. It opens and closes a real FITS file through the netCDF
!! Fortran API, relying on the generated .ncrc for UDF autoload.
!!
!! @author Edward Hartnett
!! @date 2026-06-28
!! @copyright Intelligent Data Design, Inc. All rights reserved.

program ftst_fits_udf
  use netcdf
  use iso_c_binding
  implicit none

  interface
     function c_nc_fits_initialize() result(status) bind(c, name="NC_FITS_initialize")
       use iso_c_binding
       integer(c_int) :: status
     end function c_nc_fits_initialize
  end interface

  ! This is the name of the FITS data file we will open.
  character (len = *), parameter :: FILE_NAME = "../test/data/WFPC2u5780205r_c0fx.fits"
  integer :: ncid
  integer :: retval
  integer(c_int) :: init_status
  integer :: ndims, nvars, ngatts, unlimdimid
  character(len=NF90_MAX_NAME) :: dimname
  integer :: dimlen

  ! Ensure the FITS UDF handler is registered (also covers builds where
  ! .ncrc autoload is not active).
  init_status = c_nc_fits_initialize()
  if (init_status /= 0 .and. init_status /= -36) then
     print *, "Error initializing FITS UDF handler: ", init_status
     stop 1
  endif

  ! Open the FITS file read-only.
  retval = nf90_open(FILE_NAME, NF90_NOWRITE, ncid)
  if (retval /= nf90_noerr) then
     print *, "Error opening FITS file: ", trim(nf90_strerror(retval))
     stop 1
  endif
  print *, "PASS: nf90_open"

  ! Check primary HDU metadata: 3 dims, 1 var, no unlimited dim.
  retval = nf90_inquire(ncid, ndims, nvars, ngatts, unlimdimid)
  if (retval /= nf90_noerr) then
     print *, "Error in nf90_inquire: ", trim(nf90_strerror(retval))
     stop 1
  endif
  if (ndims /= 3) then
     print *, "Expected 3 dims, got ", ndims
     stop 1
  endif
  if (nvars /= 1) then
     print *, "Expected 1 var, got ", nvars
     stop 1
  endif
  if (unlimdimid /= -1) then
     print *, "Expected no unlimited dim, got ", unlimdimid
     stop 1
  endif
  print *, "PASS: nf90_inquire ndims=", ndims, " nvars=", nvars

  ! Check dim_0 = 4 (reversed NAXIS3).
  retval = nf90_inquire_dimension(ncid, 1, dimname, dimlen)
  if (retval /= nf90_noerr) then
     print *, "Error inquiring dim 1: ", trim(nf90_strerror(retval))
     stop 1
  endif
  if (trim(dimname) /= "dim_0" .or. dimlen /= 4) then
     print *, "dim_0: expected name='dim_0' len=4, got '", trim(dimname), "' len=", dimlen
     stop 1
  endif
  print *, "PASS: dim_0 len=", dimlen

  ! Check dim_1 = 200.
  retval = nf90_inquire_dimension(ncid, 2, dimname, dimlen)
  if (retval /= nf90_noerr) then
     print *, "Error inquiring dim 2: ", trim(nf90_strerror(retval))
     stop 1
  endif
  if (dimlen /= 200) then
     print *, "dim_1: expected len=200, got ", dimlen
     stop 1
  endif
  print *, "PASS: dim_1 len=", dimlen

  ! Close the file.
  retval = nf90_close(ncid)
  if (retval /= nf90_noerr) then
     print *, "Error closing FITS file: ", trim(nf90_strerror(retval))
     stop 1
  endif
  print *, "PASS: nf90_close"

end program ftst_fits_udf
