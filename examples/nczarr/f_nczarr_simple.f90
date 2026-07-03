!> @file f_nczarr_simple.f90
!! @brief Creating, writing, and reading a
!! local NcZarr dataset (Fortran).
!!
!! Fortran equivalent of nczarr_simple.c. Creates a 4x5 temperature array in a
!! local NcZarr Zarr store, attaches units,
!! long_name, and _FillValue attributes, closes
!! the dataset, reopens it read-only, and
!! verifies metadata and data values.
!!
!! **Learning Objectives:**
!! - Use `file://...#mode=nczarr` URL from Fortran via nf90_create / nf90_open
!! - Apply the same nf90_def_dim, nf90_def_var, nf90_put_att, nf90_put_var APIs
!!   used for NetCDF-4/HDF5 files
!! - Understand Fortran column-major dimension ordering vs C row-major
!! - Validate a round-trip write/read through the Fortran NetCDF API
!!
!! **Key Concepts:**
!! - **NcZarr**: NetCDF-4 data model backed by Zarr V2 storage
!! - **URL fragment**: `#mode=nczarr` selects the NcZarr dispatcher
!! - **Fortran column-major**: dimids(1)=x,
!!   dimids(2)=y produces temperature(y,x)
!! - **NF90_NETCDF4**: Required mode flag for NcZarr
!!
!! **Fortran vs C Differences:**
!! - Array declaration: Fortran `real :: data(NX, NY)` vs C `float data[NY][NX]`
!! - Dimension order: Fortran dimids(1)=x,
!!   dimids(2)=y vs C dimids[0]=y, dimids[1]=x
!! - API prefix: Fortran nf90_* vs C nc_*
!!
!! **Related Examples:**
!! - nczarr_simple.c - C equivalent
!! - f_simple_nc4.f90 - NetCDF-4/HDF5 Fortran example
!!
!! **Compilation:**
!! @code
!! gfortran -o f_nczarr_simple f_nczarr_simple.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_nczarr_simple
!! ncdump 'file://f_simple_nczarr.zarr#mode=nczarr'
!! @endcode
!!
!! **Expected Output:**
!! Creates the directory f_simple_nczarr.zarr containing:
!! - 2 dimensions: y(4), x(5)
!! - 1 variable: temperature(y, x) of type NF90_FLOAT
!! - Attributes: units="K", long_name="Temperature", _FillValue=-999.0
!! - A 4x5 temperature data grid (same values as nczarr_simple.c)
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026-06-26

program f_nczarr_simple
   use netcdf
   implicit none

   character(len=*), parameter :: FILE_URL = &
        "file://f_simple_nczarr.zarr" // &
        "#mode=nczarr"
   integer, parameter :: NX = 5, NY = 4
   integer, parameter :: NDIMS = 2

   integer :: ncid, varid, retval
   integer :: y_dimid, x_dimid
   integer :: dimids(NDIMS)
   integer :: ndims_in, nvars_in
   integer :: len_y, len_x
   integer :: i, j, errors
   real :: data_out(NX, NY), data_in(NX, NY)
   real :: expected_val

   ! Generate a simple 2D temperature field (same formula as nczarr_simple.c).
   ! Fortran is column-major: data_out(i, j) where i=x-index, j=y-index.
   do j = 1, NY
      do i = 1, NX
         data_out(i, j) = 280.0 + real(j - 1) * 2.0 + real(i - 1) * 0.5
      end do
   end do

   ! Create a local NcZarr dataset.
   retval = nf90_create(FILE_URL, NF90_CLOBBER + NF90_NETCDF4, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Define dimensions. Fortran reverses C order: x first, then y.
   retval = nf90_def_dim(ncid, "x", NX, x_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "y", NY, y_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! dimids(1)=x, dimids(2)=y produces temperature(y, x) in the file.
   dimids(1) = x_dimid
   dimids(2) = y_dimid
   retval = nf90_def_var(ncid, "temperature", NF90_FLOAT, dimids, varid)
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

   print *, "Created ", FILE_URL

   ! Reopen the dataset and validate metadata.
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

   retval = nf90_get_var(ncid, varid, data_in)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Verify the data read back.
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

end program f_nczarr_simple
