! This is part of the book: Writing NetCDF Programs.
!
! Demonstrates multi-dimensional arrays with 2D, 3D, and 4D variables.
! Creates a file with 4 dimensions (time, level, lat, lon) and three
! variables of different dimensionalities to show dimension reuse.
!
! Author: Edward Hartnett, Intelligent Data Design, Inc.
! Copyright: 2026

program f_var4d
   use netcdf
   implicit none
   
   character(len=*), parameter :: FILE_NAME = "f_var4d.nc"
   integer, parameter :: NTIME = 3, NLEVEL = 2, NLAT = 4, NLON = 5
   
   integer :: ncid, varid_2d, varid_3d, varid_4d
   integer :: time_dimid, level_dimid, lat_dimid, lon_dimid
   integer :: dimids_2d(2), dimids_3d(3), dimids_4d(4)
   integer :: retval
   
   real :: temp_surface(NLON, NLAT)
   real :: temp_profile(NLON, NLAT, NTIME)
   real :: temp_3d(NLON, NLAT, NLEVEL, NTIME)
   
   real :: temp_surface_in(NLON, NLAT)
   real :: temp_profile_in(NLON, NLAT, NTIME)
   real :: temp_3d_in(NLON, NLAT, NLEVEL, NTIME)
   
   integer :: i, j, k, t
   integer :: ndims_in, nvars_in
   integer :: len_time, len_level, len_lat, len_lon
   integer :: var_type, var_ndims
   integer :: errors
   
   ! ========== WRITE PHASE ==========
   print *, "Creating NetCDF file: ", FILE_NAME
   
   ! Initialize 2D surface temperature (lon, lat) - Fortran column-major
   do i = 1, NLAT
      do j = 1, NLON
         temp_surface(j, i) = 273.15 + (i-1) * 5.0 + (j-1) * 2.0
      end do
   end do
   
   ! Initialize 3D temperature profile (lon, lat, time)
   do t = 1, NTIME
      do i = 1, NLAT
         do j = 1, NLON
            temp_profile(j, i, t) = 273.15 + (t-1) * 1.0 + (i-1) * 5.0 + (j-1) * 2.0
         end do
      end do
   end do
   
   ! Initialize 4D temperature field (lon, lat, level, time)
   do t = 1, NTIME
      do k = 1, NLEVEL
         do i = 1, NLAT
            do j = 1, NLON
               temp_3d(j, i, k, t) = 273.15 + (t-1) * 1.0 + (k-1) * 10.0 + &
                                     (i-1) * 5.0 + (j-1) * 2.0
            end do
         end do
      end do
   end do
   
   ! Create the NetCDF file
   retval = nf90_create(FILE_NAME, NF90_CLOBBER, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define dimensions
   retval = nf90_def_dim(ncid, "time", NTIME, time_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "level", NLEVEL, level_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "lat", NLAT, lat_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "lon", NLON, lon_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define 2D variable: temp_surface(lon, lat) - Fortran order
   dimids_2d(1) = lon_dimid
   dimids_2d(2) = lat_dimid
   retval = nf90_def_var(ncid, "temp_surface", NF90_FLOAT, dimids_2d, varid_2d)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define 3D variable: temp_profile(lon, lat, time)
   dimids_3d(1) = lon_dimid
   dimids_3d(2) = lat_dimid
   dimids_3d(3) = time_dimid
   retval = nf90_def_var(ncid, "temp_profile", NF90_FLOAT, dimids_3d, varid_3d)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define 4D variable: temp_3d(lon, lat, level, time)
   dimids_4d(1) = lon_dimid
   dimids_4d(2) = lat_dimid
   dimids_4d(3) = level_dimid
   dimids_4d(4) = time_dimid
   retval = nf90_def_var(ncid, "temp_3d", NF90_FLOAT, dimids_4d, varid_4d)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! End define mode
   retval = nf90_enddef(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Write the data
   retval = nf90_put_var(ncid, varid_2d, temp_surface)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_var(ncid, varid_3d, temp_profile)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_var(ncid, varid_4d, temp_3d)
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
   
   ! Verify metadata: check number of dimensions and variables
   retval = nf90_inquire(ncid, ndims_in, nvars_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   if (ndims_in /= 4) then
      print *, "Error: Expected 4 dimensions, found ", ndims_in
      stop 2
   end if
   print *, "Verified: ", ndims_in, " dimensions"
   
   if (nvars_in /= 3) then
      print *, "Error: Expected 3 variables, found ", nvars_in
      stop 2
   end if
   print *, "Verified: ", nvars_in, " variables"
   
   ! Verify dimension sizes
   retval = nf90_inquire_dimension(ncid, time_dimid, len=len_time)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_inquire_dimension(ncid, level_dimid, len=len_level)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_inquire_dimension(ncid, lat_dimid, len=len_lat)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_inquire_dimension(ncid, lon_dimid, len=len_lon)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   if (len_time /= NTIME .or. len_level /= NLEVEL .or. &
       len_lat /= NLAT .or. len_lon /= NLON) then
      print *, "Error: Dimension sizes incorrect"
      stop 2
   end if
   print *, "Verified: time=", len_time, ", level=", len_level, &
            ", lat=", len_lat, ", lon=", len_lon
   
   ! Verify variable types and dimensions
   
   ! Check 2D variable
   retval = nf90_inquire_variable(ncid, varid_2d, xtype=var_type, ndims=var_ndims)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (var_type /= NF90_FLOAT .or. var_ndims /= 2) then
      print *, "Error: temp_surface has wrong type or dimensions"
      stop 2
   end if
   print *, "Verified: temp_surface is 2D NF90_FLOAT"
   
   ! Check 3D variable
   retval = nf90_inquire_variable(ncid, varid_3d, xtype=var_type, ndims=var_ndims)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (var_type /= NF90_FLOAT .or. var_ndims /= 3) then
      print *, "Error: temp_profile has wrong type or dimensions"
      stop 2
   end if
   print *, "Verified: temp_profile is 3D NF90_FLOAT"
   
   ! Check 4D variable
   retval = nf90_inquire_variable(ncid, varid_4d, xtype=var_type, ndims=var_ndims)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (var_type /= NF90_FLOAT .or. var_ndims /= 4) then
      print *, "Error: temp_3d has wrong type or dimensions"
      stop 2
   end if
   print *, "Verified: temp_3d is 4D NF90_FLOAT"
   
   ! Read the data back
   retval = nf90_get_var(ncid, varid_2d, temp_surface_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_var(ncid, varid_3d, temp_profile_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_var(ncid, varid_4d, temp_3d_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Verify data correctness
   errors = 0
   
   ! Verify 2D data
   do i = 1, NLAT
      do j = 1, NLON
         if (temp_surface_in(j, i) /= temp_surface(j, i)) then
            print *, "Error: temp_surface(", j, ",", i, ") mismatch"
            errors = errors + 1
         end if
      end do
   end do
   
   ! Verify 3D data
   do t = 1, NTIME
      do i = 1, NLAT
         do j = 1, NLON
            if (temp_profile_in(j, i, t) /= temp_profile(j, i, t)) then
               print *, "Error: temp_profile(", j, ",", i, ",", t, ") mismatch"
               errors = errors + 1
            end if
         end do
      end do
   end do
   
   ! Verify 4D data
   do t = 1, NTIME
      do k = 1, NLEVEL
         do i = 1, NLAT
            do j = 1, NLON
               if (temp_3d_in(j, i, k, t) /= temp_3d(j, i, k, t)) then
                  print *, "Error: temp_3d(", j, ",", i, ",", k, ",", t, ") mismatch"
                  errors = errors + 1
               end if
            end do
         end do
      end do
   end do
   
   if (errors > 0) then
      print *, "*** FAILED: ", errors, " data validation errors"
      stop 2
   end if
   
   print *, "Verified: all data values correct"
   print *, "  2D array: ", NLAT, " x ", NLON, " = ", NLAT*NLON, " values"
   print *, "  3D array: ", NTIME, " x ", NLAT, " x ", NLON, " = ", &
            NTIME*NLAT*NLON, " values"
   print *, "  4D array: ", NTIME, " x ", NLEVEL, " x ", NLAT, " x ", NLON, &
            " = ", NTIME*NLEVEL*NLAT*NLON, " values"
   
   ! Close the file
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   print *, ""
   print *, "*** SUCCESS: All validation checks passed!"
   
contains
   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop 2
   end subroutine handle_err
   
end program f_var4d
