!> @file f_nczarr_compression.f90
!! @brief Demonstrate deflate+shuffle compression in a local NcZarr dataset (Fortran).
!!
!! Fortran equivalent of nczarr_compression.c. Creates a 4x5 temperature array
!! with explicit chunking [2, 5] (y, x) and deflate level 4 + shuffle in a
!! local NcZarr store, verifies compression metadata after reopening, and
!! validates data values.
!!
!! **Learning Objectives:**
!! - Apply deflate compression and shuffle filter from Fortran
!! - Follow the recommended workflow: set chunks first, then compression
!! - Query compression metadata after reopen
!! - Verify that decompression is transparent
!!
!! **Key Concepts:**
!! - Set chunk shape before compression (recommended workflow)
!! - nf90_def_var_deflate(ncid, varid, shuffle, deflate, deflate_level)
!!   enables shuffle and deflate filters
!! - Compression is transparent: nf90_get_var returns original values
!! - Same API works for NetCDF-4/HDF5 and NcZarr
!!
!! **Related Examples:**
!! - nczarr_compression.c - C equivalent
!! - f_nczarr_chunking.f90 - Chunking without compression
!! - f_nczarr_simple.f90 - Basic NcZarr Fortran example
!!
!! **Compilation:**
!! @code
!! gfortran -o f_nczarr_compression f_nczarr_compression.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_nczarr_compression
!! ncdump 'file://f_nczarr_compression.zarr#mode=nczarr'
!! @endcode
!!
!! **Expected Output:**
!! Creates the directory f_nczarr_compression.zarr containing:
!! - 2 dimensions: y(4), x(5)
!! - 1 variable: temperature(y, x) of type NF90_FLOAT, chunked [2, 5],
!!   with shuffle + deflate level 4
!! - Attributes: units="K", long_name="Temperature", _FillValue=-999.0
!! - A 4x5 temperature data grid stored compressed in 2 chunks
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026-06-26

program f_nczarr_compression
   use netcdf
   implicit none

   character(len=*), parameter :: FILE_URL = "file://f_nczarr_compression.zarr#mode=nczarr"
   integer, parameter :: NX = 5, NY = 4
   integer, parameter :: NDIMS = 2
   integer, parameter :: CHUNK_Y = 2, CHUNK_X = 5
   integer, parameter :: DEFLATE_LEVEL = 4

   integer :: ncid, varid, retval
   integer :: y_dimid, x_dimid
   integer :: dimids(NDIMS)
   integer :: chunksizes(NDIMS)
   integer :: ndims_in, nvars_in
   integer :: shuffle_in, deflate_in, deflate_level_in
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

   ! Step 1: Set chunk shape (required before compression).
   chunksizes(1) = CHUNK_X
   chunksizes(2) = CHUNK_Y
   retval = nf90_def_var_chunking(ncid, varid, NF90_CHUNKED, chunksizes)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Step 2: Enable shuffle + deflate level 4.
   retval = nf90_def_var_deflate(ncid, varid, 1, 1, DEFLATE_LEVEL)
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

   print *, "Created ", FILE_URL, " with deflate=", DEFLATE_LEVEL, ", shuffle=on"

   ! Reopen and verify compression metadata.
   retval = nf90_open(FILE_URL, NF90_NOWRITE, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_inquire(ncid, ndims_in, nvars_in)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_inquire_dimension(ncid, y_dimid, len=len_y)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_inquire_dimension(ncid, x_dimid, len=len_x)
   if (retval /= nf90_noerr) call handle_err(retval)

   print *, "Dataset:", ndims_in, "dims,", nvars_in, "vars, y=", len_y, ", x=", len_x

   retval = nf90_inq_varid(ncid, "temperature", varid)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_inq_var_deflate(ncid, varid, shuffle_in, deflate_in, deflate_level_in)
   if (retval /= nf90_noerr) call handle_err(retval)

   print *, "Compression: shuffle=", shuffle_in, ", deflate=", deflate_in, ", level=", deflate_level_in

   if (shuffle_in /= 1 .or. deflate_in /= 1 .or. deflate_level_in /= DEFLATE_LEVEL) then
      print *, "*** FAILED: compression metadata mismatch"
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
            print *, "Mismatch at", i, j, ": got", data_in(i, j), ", expected", expected_val
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

end program f_nczarr_compression
