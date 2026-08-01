program ftst_mmcif_udf
  use netcdf
  use iso_c_binding
  implicit none

  interface
     function c_nc_mmcif_initialize() result(dispatch_ptr) bind(c, name="NC_MMCIF_initialize")
       use iso_c_binding
       type(c_ptr) :: dispatch_ptr
     end function c_nc_mmcif_initialize
  end interface

  ! Path to one of the real PDBx/mmCIF test files.
  character (len = *), parameter :: FILE_NAME = "../test/data/mmCIF/1J7W.cif"
  integer :: ncid
  integer :: retval
  type(c_ptr) :: init_status

  ! Ensure the PDBx/mmCIF UDF handler is registered.
  init_status = c_nc_mmcif_initialize()
  if (.not. c_associated(init_status)) then
     print *, "Error initializing mmCIF UDF handler: NULL dispatch table"
     stop 1
  endif
  print *, "PASS: NC_MMCIF_initialize"

  ! Open the mmCIF file read-only, forcing UDF slot 8 so the PDBx/mmCIF
  ! dispatcher is selected (NC_UDF8 = 0x1000000 = 16777216).
  retval = nf90_open(FILE_NAME, ior(NF90_NOWRITE, 16777216), ncid)
  if (retval /= nf90_noerr) then
     print *, "Error opening mmCIF file: ", trim(nf90_strerror(retval))
     stop 1
  endif
  print *, "PASS: nf90_open"

  ! Close the file.
  retval = nf90_close(ncid)
  if (retval /= nf90_noerr) then
     print *, "Error closing mmCIF file: ", trim(nf90_strerror(retval))
     stop 1
  endif
  print *, "PASS: nf90_close"

end program ftst_mmcif_udf
