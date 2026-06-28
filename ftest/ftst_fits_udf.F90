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
  print *, "PASS: nf90_open FITS file"

  ! Close the file.
  retval = nf90_close(ncid)
  if (retval /= nf90_noerr) then
     print *, "Error closing FITS file: ", trim(nf90_strerror(retval))
     stop 1
  endif
  print *, "PASS: nf90_close FITS file"

end program ftst_fits_udf
