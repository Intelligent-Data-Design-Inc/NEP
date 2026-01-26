! This is part of the book: Writing NetCDF Programs.
!
! This program demonstrates basic NetCDF-4 file creation, writing, and reading
! with a 2D integer array using Fortran 90. This program creates a 
! file with 2 dimensions and 1 2D variable, writes sequential integer 
! data, then reopens the file to verify both metadata and data correctness.
!
! Author: Edward Hartnett, Intelligent Data Design, Inc.
! Copyright: 2026


program f_simple_2D
   use netcdf
   implicit none
   
   character(len=*), parameter :: FILE_NAME = "f_simple_2D.nc"
   integer, parameter :: NDIMS = 2
   integer, parameter :: NX = 6, NY = 12
   
   integer :: ncid, varid
   integer :: x_dimid, y_dimid
   integer :: dimids(NDIMS)
   integer :: retval
   
   integer :: data_out(NX, NY)
   integer :: data_in(NX, NY)
   
   integer :: i, j
   integer :: ndims_in, nvars_in
   integer :: len_x, len_y
   integer :: var_type
   integer :: errors
   integer :: expected
   
   ! ========== WRITE PHASE ==========
   print *, "Creating NetCDF file: ", FILE_NAME
   
   ! Initialize data with sequential integers (0, 1, 2, 3, ...)
   ! Note: Fortran is column-major, so we fill by column to match C row-major layout
   do j = 1, NY
      do i = 1, NX
         data_out(i, j) = (j-1) * NX + (i-1)
      end do
   end do
   
   ! Create the NetCDF file (NF90_CLOBBER overwrites existing file)
   retval = nf90_create(FILE_NAME, IOR(NF90_CLOBBER, NF90_NETCDF4), ncid)
   if (retval /= nf90_noerr) then
      print *, "Error: ", trim(nf90_strerror(retval))
      stop 2
   end if
   
   ! Define dimensions
   retval = nf90_def_dim(ncid, "x", NX, x_dimid)
   if (retval /= nf90_noerr) then
      print *, "Error: ", trim(nf90_strerror(retval))
      stop 2
   end if
   retval = nf90_def_dim(ncid, "y", NY, y_dimid)
   if (retval /= nf90_noerr) then
      print *, "Error: ", trim(nf90_strerror(retval))
      stop 2
   end if
   
   ! Define the variable (dimension order: x, y for Fortran column-major)
   dimids(1) = x_dimid
   dimids(2) = y_dimid
   retval = nf90_def_var(ncid, "data", NF90_INT, dimids, varid)
   if (retval /= nf90_noerr) then
      print *, "Error: ", trim(nf90_strerror(retval))
      stop 2
   end if
   
   ! End define mode
   retval = nf90_enddef(ncid)
   if (retval /= nf90_noerr) then
      print *, "Error: ", trim(nf90_strerror(retval))
      stop 2
   end if
   
   ! Write the data to the file
   retval = nf90_put_var(ncid, varid, data_out)
   if (retval /= nf90_noerr) then
      print *, "Error: ", trim(nf90_strerror(retval))
      stop 2
   end if
   
   ! Close the file
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) then
      print *, "Error: ", trim(nf90_strerror(retval))
      stop 2
   end if
   
   print *, "*** SUCCESS writing file!"
   
   ! ========== READ PHASE ==========
   print *, ""
   print *, "Reopening file for validation..."
   
   ! Open the file for reading
   retval = nf90_open(FILE_NAME, NF90_NOWRITE, ncid)
   if (retval /= nf90_noerr) then
      print *, "Error: ", trim(nf90_strerror(retval))
      stop 2
   end if
   
   ! Verify metadata: check number of dimensions and variables
   retval = nf90_inquire(ncid, ndims_in, nvars_in)
   if (retval /= nf90_noerr) then
      print *, "Error: ", trim(nf90_strerror(retval))
      stop 2
   end if
   
   if (ndims_in /= NDIMS) then
      print *, "Error: Expected ", NDIMS, " dimensions, found ", ndims_in
      stop 2
   end if
   print *, "Verified: ", ndims_in, " dimensions"
   
   if (nvars_in /= 1) then
      print *, "Error: Expected 1 variable, found ", nvars_in
      stop 2
   end if
   print *, "Verified: ", nvars_in, " variable"
   
   ! Verify dimension sizes
   retval = nf90_inquire_dimension(ncid, x_dimid, len=len_x)
   if (retval /= nf90_noerr) then
      print *, "Error: ", trim(nf90_strerror(retval))
      stop 2
   end if
   retval = nf90_inquire_dimension(ncid, y_dimid, len=len_y)
   if (retval /= nf90_noerr) then
      print *, "Error: ", trim(nf90_strerror(retval))
      stop 2
   end if
   
   if (len_x /= NX) then
      print *, "Error: Expected x dimension = ", NX, ", found ", len_x
      stop 2
   end if
   if (len_y /= NY) then
      print *, "Error: Expected y dimension = ", NY, ", found ", len_y
      stop 2
   end if
   print *, "Verified: x dimension = ", len_x, ", y dimension = ", len_y
   
   ! Verify variable type
   retval = nf90_inquire_variable(ncid, varid, xtype=var_type)
   if (retval /= nf90_noerr) then
      print *, "Error: ", trim(nf90_strerror(retval))
      stop 2
   end if
   
   if (var_type /= NF90_INT) then
      print *, "Error: Expected variable type NF90_INT, found ", var_type
      stop 2
   end if
   print *, "Verified: variable type is NF90_INT"
   
   ! Read the data back
   retval = nf90_get_var(ncid, varid, data_in)
   if (retval /= nf90_noerr) then
      print *, "Error: ", trim(nf90_strerror(retval))
      stop 2
   end if
   
   ! Verify data correctness
   errors = 0
   do j = 1, NY
      do i = 1, NX
         expected = (j-1) * NX + (i-1)
         if (data_in(i, j) /= expected) then
            print *, "Error: data(", i, ",", j, ") = ", data_in(i, j), &
                     ", expected ", expected
            errors = errors + 1
         end if
      end do
   end do
   
   if (errors > 0) then
      print *, "*** FAILED: ", errors, " data validation errors"
      stop 2
   end if
   
   print *, "Verified: all ", NX * NY, " data values correct (0, 1, 2, ..., ", &
            NX * NY - 1, ")"
   
   ! Close the file
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) then
      print *, "Error: ", trim(nf90_strerror(retval))
      stop 2
   end if
   
   print *, ""
   print *, "*** SUCCESS: All validation checks passed!"
   
end program f_simple_2D
