program ftst_dicom_udf
  use netcdf
  use iso_c_binding
  implicit none

  interface
     function c_nc_dicom_initialize() result(dispatch_ptr) bind(c, name="NC_DICOM_initialize")
       use iso_c_binding
       type(c_ptr) :: dispatch_ptr
     end function c_nc_dicom_initialize
  end interface

  character (len = *), parameter :: FILE_NAME = "../test/data/DICOM/tst_dicom_uncompressed.dcm"

  integer :: ncid
  integer :: retval
  type(c_ptr) :: init_status
  integer :: ndims, nvars, ngatts, unlimdimid
  integer :: i
  character(len=NF90_MAX_NAME) :: dimname
  integer :: dimlen
  integer :: varid, xtype, var_ndims, var_natts
  integer :: dimids(NF90_MAX_VAR_DIMS)
  integer :: pixels(4)
  integer :: start(3), counts(3)
  integer :: att_type, att_len

  init_status = c_nc_dicom_initialize()
  if (.not. c_associated(init_status)) then
     print *, "Error initializing DICOM UDF handler: NULL dispatch table"
     stop 1
  endif

  ! Open the DICOM file read-only, forcing UDF slot 6 because the DICOM
  ! magic offset is not yet honored by this NetCDF-C build.
  retval = nf90_open(FILE_NAME, ior(NF90_NOWRITE, 4194304), ncid)
  if (retval /= nf90_noerr) then
     print *, "Error opening DICOM file: ", trim(nf90_strerror(retval))
     stop 1
  endif
  print *, "PASS: nf90_open"

  retval = nf90_inquire(ncid, ndims, nvars, ngatts, unlimdimid)
  if (retval /= nf90_noerr) then
     print *, "Error in nf90_inquire: ", trim(nf90_strerror(retval))
     stop 1
  endif
  if (ndims /= 3 .or. nvars /= 1 .or. unlimdimid /= -1) then
     print *, "Unexpected file shape: ndims=", ndims, " nvars=", nvars, " unlimdim=", unlimdimid
     stop 1
  endif
  print *, "PASS: nf90_inquire ndims=", ndims, " nvars=", nvars

  do i = 1, ndims
     retval = nf90_inquire_dimension(ncid, i, dimname, dimlen)
     if (retval /= nf90_noerr) then
        print *, "Error inquiring dim ", i, ": ", trim(nf90_strerror(retval))
        stop 1
     endif
     if (i == 1) then
        if (trim(dimname) /= "frame" .or. dimlen /= 1) then
           print *, "frame: expected name='frame' len=1, got '", trim(dimname), "' len=", dimlen
           stop 1
        endif
     else if (i == 2) then
        if (trim(dimname) /= "row" .or. dimlen /= 4) then
           print *, "row: expected name='row' len=4, got '", trim(dimname), "' len=", dimlen
           stop 1
        endif
     else if (i == 3) then
        if (trim(dimname) /= "column" .or. dimlen /= 6) then
           print *, "column: expected name='column' len=6, got '", trim(dimname), "' len=", dimlen
           stop 1
        endif
     endif
     print *, "PASS: dim ", trim(dimname), " len=", dimlen
  end do

  ! Check the pixel_data variable.
  retval = nf90_inq_varid(ncid, "pixel_data", varid)
  if (retval /= nf90_noerr) then
     print *, "Error finding pixel_data: ", trim(nf90_strerror(retval))
     stop 1
  endif

  retval = nf90_inquire_variable(ncid, varid, dimname, xtype, var_ndims, dimids, var_natts)
  if (retval /= nf90_noerr) then
     print *, "Error inquiring pixel_data: ", trim(nf90_strerror(retval))
     stop 1
  endif
  if (xtype /= NF90_UBYTE) then
     print *, "Expected NF90_UBYTE for pixel_data, got ", xtype
     stop 1
  endif
  print *, "PASS: pixel_data type=NF90_UBYTE"

  ! Read the first four pixels of the first row.
  ! netCDF Fortran reverses start/count vs C; count=(4,1,1) maps to
  ! C count=(1,1,4) for the [frame][row][column] variable.
  start = (/ 1, 1, 1 /)
  counts = (/ 4, 1, 1 /)
  retval = nf90_get_var(ncid, varid, pixels, start=start, count=counts)
  if (retval /= nf90_noerr) then
     print *, "Error reading pixel_data: ", trim(nf90_strerror(retval))
     stop 1
  endif
  if (pixels(1) /= 0 .or. pixels(2) /= 1 .or. pixels(3) /= 2 .or. pixels(4) /= 3) then
     print *, "Unexpected pixel values: ", pixels
     stop 1
  endif
  print *, "PASS: pixel_data slice values=", pixels

  ! Verify global attributes exist and are character-valued.
  retval = nf90_inquire_attribute(ncid, NF90_GLOBAL, "PatientName", att_type, att_len)
  if (retval /= nf90_noerr .or. att_type /= NF90_CHAR) then
     print *, "Error or wrong type for PatientName attribute"
     stop 1
  endif
  print *, "PASS: global att PatientName type=NF90_CHAR len=", att_len

  retval = nf90_inquire_attribute(ncid, NF90_GLOBAL, "Modality", att_type, att_len)
  if (retval /= nf90_noerr .or. att_type /= NF90_CHAR) then
     print *, "Error or wrong type for Modality attribute"
     stop 1
  endif
  print *, "PASS: global att Modality type=NF90_CHAR len=", att_len

  retval = nf90_inquire_attribute(ncid, NF90_GLOBAL, "TransferSyntaxUID", att_type, att_len)
  if (retval /= nf90_noerr .or. att_type /= NF90_CHAR) then
     print *, "Error or wrong type for TransferSyntaxUID attribute"
     stop 1
  endif
  print *, "PASS: global att TransferSyntaxUID type=NF90_CHAR len=", att_len

  ! Read a variable attribute to confirm DICOM tag mapping.
  retval = nf90_inquire_attribute(ncid, varid, "BitsAllocated", att_type, att_len)
  if (retval /= nf90_noerr .or. att_type /= NF90_CHAR) then
     print *, "Error or wrong type for BitsAllocated attribute"
     stop 1
  endif
  print *, "PASS: var att BitsAllocated type=NF90_CHAR len=", att_len

  retval = nf90_close(ncid)
  if (retval /= nf90_noerr) then
     print *, "Error closing DICOM file: ", trim(nf90_strerror(retval))
     stop 1
  endif
  print *, "PASS: nf90_close"

  print *, "Success!"
end program ftst_dicom_udf
