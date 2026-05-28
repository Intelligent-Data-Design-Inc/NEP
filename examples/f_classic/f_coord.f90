!> @file f_coord.f90
!! @brief Demonstrates 3D surface temperature with time, lat, lon coordinate variables (Fortran)
!!
!! This is the Fortran equivalent of coord.c, demonstrating 3D surface temperature
!! data with time, latitude, and longitude coordinate variables following CF conventions.
!! The program creates a surface temperature dataset on a 4x5 lat/lon grid with 3
!! time steps using the Fortran 90 NetCDF API.
!!
!! **Learning Objectives:**
!! - Work with 3D data in Fortran (time, lat, lon)
!! - Define time coordinate variables with CF conventions
!! - Use the CF `calendar` attribute for time coordinates
!! - Add the `coordinates` attribute to data variables (CF best practice)
!! - Handle Fortran column-major array ordering for multi-dimensional data
!!
!! **Key Fortran Concepts:**
!! - **Array Ordering**: sfc_temp(NLON, NLAT, NTIME) vs C sfc_temp[NTIME][NLAT][NLON]
!! - **Dimension Order**: dimids = (/lon_dimid, lat_dimid, time_dimid/) reversed from C
!! - **1-Based Indexing**: Loop indices start at 1, adjust formula accordingly
!! - **Character Attributes**: nf90_put_att handles string length automatically
!!
!! **Prerequisites:**
!! - f_coord_vars.f90 - 2D coordinate variables in Fortran
!! - coord.c - C equivalent for comparison
!!
!! **Related Examples:**
!! - coord.c - C equivalent
!! - f_coord_vars.f90 - 2D version without time dimension
!! - f_unlimited_dim.f90 - Unlimited dimensions in Fortran
!! - f_var4d.f90 - 4D data in Fortran
!!
!! **Compilation:**
!! @code
!! gfortran -o f_coord f_coord.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_coord
!! ncdump f_coord.nc
!! @endcode
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026

program f_coord
   use netcdf
   implicit none

   character(len=*), parameter :: FILE_NAME = "f_coord.nc"
   integer, parameter :: NTIME = 3, NLAT = 4, NLON = 5

   integer :: ncid, time_varid, lat_varid, lon_varid, temp_varid
   integer :: time_dimid, lat_dimid, lon_dimid
   integer :: dimids(3)
   integer :: retval

   real :: time_data(NTIME) = (/0.0, 6.0, 12.0/)
   real :: lat(NLAT) = (/-45.0, -15.0, 15.0, 45.0/)
   real :: lon(NLON) = (/-120.0, -60.0, 0.0, 60.0, 120.0/)
   real :: sfc_temp(NLON, NLAT, NTIME)

   real :: time_in(NTIME)
   real :: lat_in(NLAT)
   real :: lon_in(NLON)
   real :: sfc_temp_in(NLON, NLAT, NTIME)

   integer :: t, i, j
   integer :: ndims_in, nvars_in
   integer :: errors
   character(len=256) :: time_units, time_stdname, time_axis, time_cal
   character(len=256) :: lat_units, lat_stdname, lat_axis
   character(len=256) :: lon_units, lon_stdname
   character(len=256) :: temp_units, temp_coords
   real :: fill_value, fill_value_in
   integer :: time_len, lat_len, lon_len

   ! ========== WRITE PHASE ==========
   print *, "Creating NetCDF file: ", FILE_NAME

   ! Initialize surface temperature data
   do t = 1, NTIME
      do i = 1, NLAT
         do j = 1, NLON
            sfc_temp(j, i, t) = 280.0 + (i-1) * 2.0 + (j-1) * 0.5 + (t-1) * 1.0
         end do
      end do
   end do

   ! Create the NetCDF file (classic format)
   retval = nf90_create(FILE_NAME, NF90_CLOBBER, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Define dimensions
   retval = nf90_def_dim(ncid, "time", NTIME, time_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "lat", NLAT, lat_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "lon", NLON, lon_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Define time coordinate variable
   retval = nf90_def_var(ncid, "time", NF90_FLOAT, time_dimid, time_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, time_varid, "units", "hours since 2026-01-01")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, time_varid, "standard_name", "time")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, time_varid, "long_name", "Time")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, time_varid, "axis", "T")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, time_varid, "calendar", "standard")
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Define latitude coordinate variable
   retval = nf90_def_var(ncid, "lat", NF90_FLOAT, lat_dimid, lat_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, lat_varid, "units", "degrees_north")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, lat_varid, "standard_name", "latitude")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, lat_varid, "long_name", "Latitude")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, lat_varid, "axis", "Y")
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Define longitude coordinate variable
   retval = nf90_def_var(ncid, "lon", NF90_FLOAT, lon_dimid, lon_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, lon_varid, "units", "degrees_east")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, lon_varid, "standard_name", "longitude")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, lon_varid, "long_name", "Longitude")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, lon_varid, "axis", "X")
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Define surface temperature variable (Fortran order: lon, lat, time)
   dimids(1) = lon_dimid
   dimids(2) = lat_dimid
   dimids(3) = time_dimid
   retval = nf90_def_var(ncid, "sfc_temp", NF90_FLOAT, dimids, temp_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, temp_varid, "units", "K")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, temp_varid, "standard_name", "surface_temperature")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, temp_varid, "long_name", "Surface Temperature")
   if (retval /= nf90_noerr) call handle_err(retval)

   fill_value = -999.0
   retval = nf90_put_att(ncid, temp_varid, "_FillValue", fill_value)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, temp_varid, "coordinates", "time lat lon")
   if (retval /= nf90_noerr) call handle_err(retval)

   ! End define mode
   retval = nf90_enddef(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Write coordinate variables
   retval = nf90_put_var(ncid, time_varid, time_data)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_var(ncid, lat_varid, lat)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_var(ncid, lon_varid, lon)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Write surface temperature data
   retval = nf90_put_var(ncid, temp_varid, sfc_temp)
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

   ! Verify metadata
   retval = nf90_inquire(ncid, ndims_in, nvars_in)
   if (retval /= nf90_noerr) call handle_err(retval)

   if (ndims_in /= 3) then
      print *, "Error: Expected 3 dimensions, found ", ndims_in
      stop 2
   end if
   print *, "Verified: ", ndims_in, " dimensions"

   if (nvars_in /= 4) then
      print *, "Error: Expected 4 variables, found ", nvars_in
      stop 2
   end if
   print *, "Verified: ", nvars_in, " variables (time, lat, lon, sfc_temp)"

   ! Verify dimension sizes
   retval = nf90_inquire_dimension(ncid, time_dimid, len=time_len)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_inquire_dimension(ncid, lat_dimid, len=lat_len)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_inquire_dimension(ncid, lon_dimid, len=lon_len)
   if (retval /= nf90_noerr) call handle_err(retval)

   if (time_len /= NTIME .or. lat_len /= NLAT .or. lon_len /= NLON) then
      print *, "Error: dimension sizes incorrect"
      stop 2
   end if
   print *, "Verified: dimensions correct (time=", time_len, ", lat=", lat_len, ", lon=", lon_len, ")"

   ! Verify time coordinate attributes
   retval = nf90_get_att(ncid, time_varid, "units", time_units)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_att(ncid, time_varid, "standard_name", time_stdname)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_att(ncid, time_varid, "axis", time_axis)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_att(ncid, time_varid, "calendar", time_cal)
   if (retval /= nf90_noerr) call handle_err(retval)

   if (trim(time_units) /= "hours since 2026-01-01" .or. &
       trim(time_stdname) /= "time" .or. &
       trim(time_axis) /= "T" .or. &
       trim(time_cal) /= "standard") then
      print *, "Error: time coordinate attributes incorrect"
      stop 2
   end if
   print *, "Verified: all time coordinate attributes correct"

   ! Verify latitude coordinate attributes
   retval = nf90_get_att(ncid, lat_varid, "units", lat_units)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_att(ncid, lat_varid, "standard_name", lat_stdname)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_att(ncid, lat_varid, "axis", lat_axis)
   if (retval /= nf90_noerr) call handle_err(retval)

   if (trim(lat_units) /= "degrees_north" .or. &
       trim(lat_stdname) /= "latitude" .or. &
       trim(lat_axis) /= "Y") then
      print *, "Error: latitude coordinate attributes incorrect"
      stop 2
   end if
   print *, "Verified: all latitude coordinate attributes correct"

   ! Verify longitude coordinate attributes
   retval = nf90_get_att(ncid, lon_varid, "units", lon_units)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_att(ncid, lon_varid, "standard_name", lon_stdname)
   if (retval /= nf90_noerr) call handle_err(retval)

   if (trim(lon_units) /= "degrees_east" .or. &
       trim(lon_stdname) /= "longitude") then
      print *, "Error: longitude coordinate attributes incorrect"
      stop 2
   end if
   print *, "Verified: all longitude coordinate attributes correct"

   ! Verify sfc_temp variable attributes
   retval = nf90_get_att(ncid, temp_varid, "units", temp_units)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_att(ncid, temp_varid, "_FillValue", fill_value_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_att(ncid, temp_varid, "coordinates", temp_coords)
   if (retval /= nf90_noerr) call handle_err(retval)

   if (trim(temp_units) /= "K" .or. &
       fill_value_in /= fill_value .or. &
       trim(temp_coords) /= "time lat lon") then
      print *, "Error: sfc_temp variable attributes incorrect"
      stop 2
   end if
   print *, "Verified: all sfc_temp variable attributes correct"

   ! Read coordinate variables
   retval = nf90_get_var(ncid, time_varid, time_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_var(ncid, lat_varid, lat_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_var(ncid, lon_varid, lon_in)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Verify coordinate data
   errors = 0
   do t = 1, NTIME
      if (time_in(t) /= time_data(t)) then
         print *, "Error: time(", t, ") = ", time_in(t), ", expected ", time_data(t)
         errors = errors + 1
      end if
   end do

   do i = 1, NLAT
      if (lat_in(i) /= lat(i)) then
         print *, "Error: lat(", i, ") = ", lat_in(i), ", expected ", lat(i)
         errors = errors + 1
      end if
   end do

   do j = 1, NLON
      if (lon_in(j) /= lon(j)) then
         print *, "Error: lon(", j, ") = ", lon_in(j), ", expected ", lon(j)
         errors = errors + 1
      end if
   end do

   if (errors == 0) then
      print *, "Verified: coordinate arrays correct"
      print *, "  time: [", time_data(1), ", ", time_data(2), ", ", time_data(3), "]"
      print *, "  lat: [", lat(1), ", ", lat(2), ", ", lat(3), ", ", lat(4), "]"
      print *, "  lon: [", lon(1), ", ", lon(2), ", ", lon(3), ", ", lon(4), ", ", lon(5), "]"
   end if

   ! Read surface temperature data
   retval = nf90_get_var(ncid, temp_varid, sfc_temp_in)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Verify surface temperature data
   do t = 1, NTIME
      do i = 1, NLAT
         do j = 1, NLON
            if (sfc_temp_in(j, i, t) /= sfc_temp(j, i, t)) then
               print *, "Error: sfc_temp(", j, ",", i, ",", t, ") = ", sfc_temp_in(j, i, t), &
                        ", expected ", sfc_temp(j, i, t)
               errors = errors + 1
            end if
         end do
      end do
   end do

   if (errors > 0) then
      print *, "*** FAILED: ", errors, " data validation errors"
      stop 2
   end if

   print *, "Verified: all surface temperature data correct (", NTIME * NLAT * NLON, " values)"

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

end program f_coord
