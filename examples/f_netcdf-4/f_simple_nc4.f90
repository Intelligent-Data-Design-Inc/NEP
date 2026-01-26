!> @file f_simple_nc4.f90
!! @brief Basic NetCDF-4/HDF5 format file creation and format detection (Fortran)
!!
!! Fortran equivalent of simple_nc4.c, demonstrating NetCDF-4 format using the
!! Fortran 90 NetCDF API. Creates a simple 2D array with NF90_NETCDF4 flag.
!!
!! **Learning Objectives:**
!! - Understand NF90_NETCDF4 flag in Fortran
!! - Learn format detection with nf90_inq_format()
!! - Prepare for NetCDF-4 features (compression, chunking, groups)
!!
!! **Fortran NetCDF-4 Constants:**
!! - NF90_NETCDF4 - Create NetCDF-4/HDF5 format file
!! - NF90_FORMAT_NETCDF4 - Format detection constant
!!
!! **Prerequisites:**
!! - f_simple_2D.f90 - Basic Fortran NetCDF operations
!! - simple_nc4.c - C equivalent
!!
!! **Related Examples:**
!! - simple_nc4.c - C equivalent
!! - f_compression.f90 - NetCDF-4 compression
!! - f_chunking_performance.f90 - NetCDF-4 chunking
!!
!! **Compilation:**
!! @code
!! gfortran -o f_simple_nc4 f_simple_nc4.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026

program f_simple_nc4
   use netcdf
   implicit none
   
   character(len=*), parameter :: FILE_NAME = "f_simple_nc4.nc"
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
   integer :: format
   
   ! ========== WRITE PHASE ==========
   print *, "Creating NetCDF-4 file: ", FILE_NAME
   
   ! Initialize data with sequential integers (0, 1, 2, 3, ...)
   do j = 1, NY
      do i = 1, NX
         data_out(i, j) = (j-1) * NX + (i-1)
      end do
   end do
   
   ! Create the NetCDF-4 file with NF90_NETCDF4 flag
   retval = nf90_create(FILE_NAME, NF90_CLOBBER + NF90_NETCDF4, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define dimensions
   retval = nf90_def_dim(ncid, "x", NX, x_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "y", NY, y_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define the variable (dimension order: x, y for Fortran column-major)
   dimids(1) = x_dimid
   dimids(2) = y_dimid
   retval = nf90_def_var(ncid, "data", NF90_INT, dimids, varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! End define mode
   retval = nf90_enddef(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Write the data to the file
   retval = nf90_put_var(ncid, varid, data_out)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Close the file
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   print *, "*** SUCCESS writing file!"
   
   ! ========== READ PHASE ==========
   print *, ""
   print *, "Reopening file for validation..."
   
   ! Open the file for reading
   retval = nf90_open(FILE_NAME, NF90_NOWRITE, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Verify format is NetCDF-4
   retval = nf90_inq_format(ncid, format)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   if (format /= NF90_FORMAT_NETCDF4) then
      print *, "Error: Expected NF90_FORMAT_NETCDF4 (", NF90_FORMAT_NETCDF4, &
               "), found ", format
      stop 2
   end if
   print *, "Verified: Format is NF90_FORMAT_NETCDF4"
   
   ! Verify metadata: check number of dimensions and variables
   retval = nf90_inquire(ncid, ndims_in, nvars_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   
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
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_inquire_dimension(ncid, y_dimid, len=len_y)
   if (retval /= nf90_noerr) call handle_err(retval)
   
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
   if (retval /= nf90_noerr) call handle_err(retval)
   
   if (var_type /= NF90_INT) then
      print *, "Error: Expected variable type NF90_INT, found ", var_type
      stop 2
   end if
   print *, "Verified: variable type is NF90_INT"
   
   ! Read the data back
   retval = nf90_get_var(ncid, varid, data_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   
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
   if (retval /= nf90_noerr) call handle_err(retval)
   
   print *, ""
   print *, "*** SUCCESS: All validation checks passed!"
   print *, "NetCDF-4 format uses HDF5 as storage backend."
   
contains
   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop 2
   end subroutine handle_err
   
end program f_simple_nc4
