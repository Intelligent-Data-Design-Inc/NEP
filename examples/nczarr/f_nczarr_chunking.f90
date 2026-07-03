!> @file f_nczarr_chunking.f90
!! @brief Demonstrate chunked storage in a local NcZarr dataset (Fortran).
!!
!! Fortran equivalent of nczarr_chunking.c. Creates a 4x5 temperature array
!! with explicit chunk shape [2, 5] (y, x) in a local NcZarr store, verifies
!! chunking metadata after reopening, and validates data values.
!!
!! **Learning Objectives:**
!! - Set explicit chunk shape for a NcZarr variable from Fortran
!! - Query chunk metadata after reopen using nf90_inquire_variable
!! - Understand Fortran column-major chunksizes ordering
!!
!! **Key Concepts:**
!! - nf90_def_var_chunking() sets chunk shape for NcZarr storage
!! - Fortran column-major: chunksizes(1)=CHUNK_X, chunksizes(2)=CHUNK_Y
!! - Chunking controls how data is split into Zarr chunk files
!! - Same nf90_def_var_chunking() API works for NetCDF-4/HDF5 and NcZarr
!!
!! **Fortran vs C Differences:**
!! - Chunk dimensions reversed: Fortran chunksizes(1)=x, (2)=y
!!   vs C chunksizes[0]=y, [1]=x
!! - Uses nf90_inquire_variable(chunksizes=) instead of separate function
!!
!! **Related Examples:**
!! - nczarr_chunking.c - C equivalent
!! - f_nczarr_simple.f90 - Basic NcZarr Fortran example
!! - f_nczarr_compression.f90 - Adding compression on top of chunking
!!
!! **Compilation:**
!! @code
!! gfortran -o f_nczarr_chunking f_nczarr_chunking.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_nczarr_chunking
!! ncdump 'file://f_nczarr_chunking.zarr#mode=nczarr'
!! @endcode
!!
!! **Expected Output:**
!! Creates the directory f_nczarr_chunking.zarr containing:
!! - 2 dimensions: y(4), x(5)
!! - 1 variable: temperature(y, x) of type NF90_FLOAT with chunks [2, 5]
!! - Attributes: units="K", long_name="Temperature", _FillValue=-999.0
!! - A 4x5 temperature data grid stored in 2 chunks of 2x5 each
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026-06-26

program f_nczarr_chunking
   use netcdf
   implicit none

   character(len=*), parameter :: FILE_URL = &
        "file://f_nczarr_chunking.zarr" // &
        "#mode=nczarr"
   integer, parameter :: NX = 5, NY = 4
   integer, parameter :: NDIMS = 2
   integer, parameter :: CHUNK_Y = 2, CHUNK_X = 5

   integer :: ncid, varid, retval
   integer :: y_dimid, x_dimid
   integer :: dimids(NDIMS)
   integer :: chunksizes(NDIMS), chunks_in(NDIMS)
   integer :: ndims_in, nvars_in
   integer :: len_y, len_x
   integer :: i, j, errors
   real :: data_out(NX, NY), data_in(NX, NY)
   real :: expected_val

   ! Generate a simple 2D temperature field.
   do j = 1, NY
      do i = 1, NX
         data_out(i, j) = 280.0 + real(j - 1) * 2.0 + real(i - 1) * 0.5
      end do
   end do

   ! Create a local NcZarr dataset.
   retval = nf90_create(FILE_URL, NF90_CLOBBER + NF90_NETCDF4, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_def_dim(ncid, "x", NX, x_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "y", NY, y_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)

   dimids(1) = x_dimid
   dimids(2) = y_dimid
   retval = nf90_def_var(ncid, "temperature", NF90_FLOAT, dimids, varid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Set chunk sizes (Fortran order: x first, y second).
   chunksizes(1) = CHUNK_X
   chunksizes(2) = CHUNK_Y
   retval = nf90_def_var_chunking(ncid, varid, NF90_CHUNKED, chunksizes)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_put_att(ncid, varid, "units", "K")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, varid, "long_name", "Temperature")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, varid, "_FillValue", -999.0)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_enddef(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_put_var(ncid, varid, data_out)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   print *, "Created ", FILE_URL, " with chunks [", CHUNK_Y, ",", CHUNK_X, "]"

   ! Reopen and verify chunking metadata.
   retval = nf90_open(FILE_URL, NF90_NOWRITE, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_inquire(ncid, ndims_in, nvars_in)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_inquire_dimension(ncid, y_dimid, len=len_y)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_inquire_dimension(ncid, x_dimid, len=len_x)
   if (retval /= nf90_noerr) call handle_err(retval)

   print *, "Dataset:", ndims_in, "dims,", &
        nvars_in, "vars, y=", len_y, &
        ", x=", len_x

   retval = nf90_inq_varid(ncid, "temperature", varid)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_inquire_variable(ncid, varid, chunksizes=chunks_in)
   if (retval /= nf90_noerr) call handle_err(retval)

   print *, "Chunking: chunks=[", chunks_in(2), ",", chunks_in(1), "]"

   if (chunks_in(1) /= CHUNK_X .or. chunks_in(2) /= CHUNK_Y) then
      print *, "*** FAILED: chunk metadata mismatch"
      stop 2
   end if

   retval = nf90_get_var(ncid, varid, data_in)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Verify data.
   errors = 0
   do j = 1, NY
      do i = 1, NX
         expected_val = 280.0 + real(j - 1) * 2.0 + real(i - 1) * 0.5
         if (data_in(i, j) /= expected_val) then
            print *, "Mismatch at", i, j, &
                 ": got", data_in(i, j), &
                 ", expected", expected_val
            errors = errors + 1
         end if
      end do
   end do

   if (errors > 0) then
      print *, "*** FAILED:", errors, "data validation errors"
      stop 2
   end if

   print *, "Read", NX * NY, "values, all correct. Done."

contains
   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop 2
   end subroutine handle_err

end program f_nczarr_chunking
