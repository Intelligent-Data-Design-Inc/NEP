program ftst_pdb_udf
  use netcdf
  use iso_c_binding
  implicit none

  interface
     function c_nc_pdb_initialize() result(dispatch_ptr) bind(c, name="NC_PDB_initialize")
       use iso_c_binding
       type(c_ptr) :: dispatch_ptr
     end function c_nc_pdb_initialize
  end interface

  ! Path to one of the real PDB test files.
  character (len = *), parameter :: FILE_NAME = "../test/data/PDB/1GAB.pdb"
  integer :: ncid
  integer :: retval
  type(c_ptr) :: init_status
  integer :: ndims, nvars, ngatts, unlimdimid

  ! Ensure the PDB UDF handler is registered.
  init_status = c_nc_pdb_initialize()
  if (.not. c_associated(init_status)) then
     print *, "Error initializing PDB UDF handler: NULL dispatch table"
     stop 1
  endif
  print *, "PASS: NC_PDB_initialize"

  ! Open the PDB file read-only.
  retval = nf90_open(FILE_NAME, NF90_NOWRITE, ncid)
  if (retval /= nf90_noerr) then
     print *, "Error opening PDB file: ", trim(nf90_strerror(retval))
     stop 1
  endif
  print *, "PASS: nf90_open"

  ! Sanity-check the metadata exposed by the reader.
  retval = nf90_inquire(ncid, ndims, nvars, ngatts, unlimdimid)
  if (retval /= nf90_noerr) then
     print *, "Error in nf90_inquire: ", trim(nf90_strerror(retval))
     stop 1
  endif
  if (ndims /= 2) then
     print *, "Expected 2 dims, got ", ndims
     stop 1
  endif
  if (nvars < 3) then
     print *, "Expected at least 3 vars, got ", nvars
     stop 1
  endif
  print *, "PASS: nf90_inquire ndims=", ndims, " nvars=", nvars

  ! Close the file.
  retval = nf90_close(ncid)
  if (retval /= nf90_noerr) then
     print *, "Error closing PDB file: ", trim(nf90_strerror(retval))
     stop 1
  endif
  print *, "PASS: nf90_close"

end program ftst_pdb_udf
