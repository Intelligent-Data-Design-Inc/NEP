!> @file
!!
!! This is a test program for the NEP FITS User Defined Format (UDF)
!! handler. Sprints 4a/4b: metadata checks. Sprint 5: data reads.
!!
!! @author Edward Hartnett
!! @date 2026-06-28
!! @copyright Intelligent Data Design, Inc. All rights reserved.

program ftst_fits_udf
  use netcdf
  use iso_c_binding
  implicit none

  interface
     function c_nc_fits_initialize() result(dispatch_ptr) bind(c, name="NC_FITS_initialize")
       use iso_c_binding
       type(c_ptr) :: dispatch_ptr
     end function c_nc_fits_initialize
  end interface

  ! This is the name of the FITS data file we will open.
  character (len = *), parameter :: FILE_NAME = "../test/data/WFPC2u5780205r_c0fx.fits"
  integer :: ncid
  integer :: retval
  type(c_ptr) :: init_status
  integer :: ndims, nvars, ngatts, unlimdimid
  integer :: grpid, grpid2
  character(len=NF90_MAX_NAME) :: dimname
  integer :: dimlen
  integer :: image_varid, crval1_varid, fillcnt_varid
  real    :: pixels(4)
  double precision :: crval1(4)
  integer :: fillcnt(4)
  integer :: s3(3), c3(3), s1(1), c1(1)

  ! Ensure the FITS UDF handler is registered (also covers builds where
  ! .ncrc autoload is not active).
  init_status = c_nc_fits_initialize()
  if (.not. c_associated(init_status)) then
     print *, "Error initializing FITS UDF handler: NULL dispatch table"
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

  ! --- Sprint 4b: extension HDU as child group ---
  ! HDU 2 (ASCII table) -> group "u5780205r_cvt_c0h_tab"
  retval = nf90_inq_grp_ncid(ncid, "u5780205r_cvt_c0h_tab", grpid)
  if (retval /= nf90_noerr) then
     print *, "Error finding child group: ", trim(nf90_strerror(retval))
     stop 1
  endif
  print *, "PASS: nf90_inq_grp_ncid 'u5780205r_cvt_c0h_tab'"

  retval = nf90_inquire(grpid, ndims, nvars, ngatts, unlimdimid)
  if (retval /= nf90_noerr) then
     print *, "Error in group nf90_inquire: ", trim(nf90_strerror(retval))
     stop 1
  endif
  if (nvars /= 49) then
     print *, "Expected 49 vars in group, got ", nvars
     stop 1
  endif
  print *, "PASS: group nf90_inquire ndims=", ndims, " nvars=", nvars

  ! Row dimension must exist with 4 rows
  retval = nf90_inq_dimid(grpid, "row", grpid2)
  if (retval /= nf90_noerr) then
     print *, "Error finding row dim: ", trim(nf90_strerror(retval))
     stop 1
  endif
  retval = nf90_inquire_dimension(grpid, grpid2, dimname, dimlen)
  if (retval /= nf90_noerr) then
     print *, "Error inquiring row dim: ", trim(nf90_strerror(retval))
     stop 1
  endif
  if (dimlen /= 4) then
     print *, "Expected row dim len=4, got ", dimlen
     stop 1
  endif
  print *, "PASS: group row dim len=", dimlen

  ! --- Sprint 5: read image data ---
  ! Read image[0,0,0:4] (4 pixels along fastest dim_2 / NAXIS1).
  ! Fortran nf90_get_var uses 1-based indices; dim order same as C here
  ! because netCDF Fortran reverses vs C, but we match with reversed start/count.
  retval = nf90_inq_varid(ncid, "image", image_varid)
  if (retval /= nf90_noerr) then
     print *, "Error finding image var: ", trim(nf90_strerror(retval))
     stop 1
  endif
  ! netCDF Fortran dim order is reversed vs C: dim 1 = C dim_2 (size 200),
  ! dim 2 = C dim_1 (size 200), dim 3 = C dim_0 (size 4).
  ! start=(1,1,1) count=(4,1,1) reads 4 pixels along NAXIS1.
  s3 = (/ 1, 1, 1 /)
  c3 = (/ 4, 1, 1 /)
  retval = nf90_get_var(ncid, image_varid, pixels, start=s3, count=c3)
  if (retval /= nf90_noerr) then
     print *, "Error reading image pixels: ", trim(nf90_strerror(retval))
     stop 1
  endif
  ! Expected: pixels(1)~-1.5443, pixels(2)~0.9169
  if (abs(pixels(1) - (-1.5442986)) > 1e-5) then
     print *, "image pixel(1): expected ~-1.5443, got ", pixels(1)
     stop 1
  endif
  if (abs(pixels(2) - 0.9169310) > 1e-5) then
     print *, "image pixel(2): expected ~0.9169, got ", pixels(2)
     stop 1
  endif
  print *, "PASS: image pixels(1:4) =", pixels

  ! --- Sprint 5: read table column CRVAL1 (NC_DOUBLE, 4 rows) ---
  retval = nf90_inq_varid(grpid, "CRVAL1", crval1_varid)
  if (retval /= nf90_noerr) then
     print *, "Error finding CRVAL1 var: ", trim(nf90_strerror(retval))
     stop 1
  endif
  s1 = (/ 1 /)
  c1 = (/ 4 /)
  retval = nf90_get_var(grpid, crval1_varid, crval1, start=s1, count=c1)
  if (retval /= nf90_noerr) then
     print *, "Error reading CRVAL1: ", trim(nf90_strerror(retval))
     stop 1
  endif
  ! Expected row 0: ~182.631, row 1: ~182.626
  if (abs(crval1(1) - 182.63118863080002d0) > 1d-6) then
     print *, "CRVAL1(1): expected ~182.631, got ", crval1(1)
     stop 1
  endif
  print *, "PASS: CRVAL1 =", crval1

  ! --- Sprint 5: read table column FILLCNT (NC_INT, 4 rows) ---
  retval = nf90_inq_varid(grpid, "FILLCNT", fillcnt_varid)
  if (retval /= nf90_noerr) then
     print *, "Error finding FILLCNT var: ", trim(nf90_strerror(retval))
     stop 1
  endif
  retval = nf90_get_var(grpid, fillcnt_varid, fillcnt, start=s1, count=c1)
  if (retval /= nf90_noerr) then
     print *, "Error reading FILLCNT: ", trim(nf90_strerror(retval))
     stop 1
  endif
  if (fillcnt(1) /= 0 .or. fillcnt(2) /= 0 .or. fillcnt(3) /= 0 .or. fillcnt(4) /= 0) then
     print *, "FILLCNT: expected all zeros, got", fillcnt
     stop 1
  endif
  print *, "PASS: FILLCNT =", fillcnt

  ! --- Sprint 6: second image plane image[1,0,0] ---
  ! Fortran dim order reversed: dim 3 = C dim_0 (size 4, plane index).
  ! start=(1,1,2) count=(1,1,1) => C start={1,0,0} count={1,1,1}
  s3 = (/ 1, 1, 2 /)
  c3 = (/ 1, 1, 1 /)
  retval = nf90_get_var(ncid, image_varid, pixels(1:1), start=s3, count=c3)
  if (retval /= nf90_noerr) then
     print *, "Error reading image[1,0,0]: ", trim(nf90_strerror(retval))
     stop 1
  endif
  ! Expected: ~0.5045
  if (abs(pixels(1) - 0.5044580698) > 1e-5) then
     print *, "image[1,0,0]: expected ~0.5045, got ", pixels(1)
     stop 1
  endif
  print *, "PASS: image[1,0,0] =", pixels(1)

  ! Close the file.
  retval = nf90_close(ncid)
  if (retval /= nf90_noerr) then
     print *, "Error closing FITS file: ", trim(nf90_strerror(retval))
     stop 1
  endif
  print *, "PASS: nf90_close"

end program ftst_fits_udf
