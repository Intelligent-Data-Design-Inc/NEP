!> @file f_coord_vars.f90
!! @brief Demonstrates coordinate variables and CF convention metadata (Fortran)
!!
!! This is the Fortran equivalent of coord_vars.c, demonstrating coordinate variables
!! and CF (Climate and Forecast) convention metadata using the Fortran 90 NetCDF API.
!! The program creates a 2D temperature field with latitude and longitude coordinate
!! variables following CF conventions.
!!
!! **Learning Objectives:**
!! - Understand coordinate variables in Fortran NetCDF API
!! - Learn CF convention attributes (nf90_put_att)
!! - Master attribute definition and retrieval in Fortran
!! - Work with geospatial data in Fortran
!! - Verify equivalence with C version (coord_vars.c)
!!
!! **Key Fortran Concepts:**
!! - **Character Attributes**: Use nf90_put_att with character strings
!! - **Attribute Length**: Fortran handles string length automatically
!! - **Array Ordering**: Temperature(NLON, NLAT) vs C temperature[NLAT][NLON]
!!
!! **Prerequisites:**
!! - f_simple_2D.f90 - Basic Fortran NetCDF operations
!! - coord_vars.c - C equivalent for comparison
!!
!! **Related Examples:**
!! - coord_vars.c - C equivalent
!! - f_unlimited_dim.f90 - Time-series with coordinates
!! - f_var4d.f90 - 4D data with multiple coordinates
!!
!! **Compilation:**
!! @code
!! gfortran -o f_coord_vars f_coord_vars.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_coord_vars
!! ncdump f_coord_vars.nc
!! @endcode
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026

program f_coord_vars
   use netcdf
   implicit none
   
   character(len=*), parameter :: FILE_NAME = "f_coord_vars.nc"
   integer, parameter :: NLAT = 4, NLON = 5
   
   integer :: ncid, lat_varid, lon_varid, temp_varid
   integer :: lat_dimid, lon_dimid
   integer :: dimids(2)
   integer :: retval
   
   real :: lat(NLAT) = (/-45.0, -15.0, 15.0, 45.0/)
   real :: lon(NLON) = (/-120.0, -60.0, 0.0, 60.0, 120.0/)
   real :: temperature(NLON, NLAT)
   
   real :: lat_in(NLAT)
   real :: lon_in(NLON)
   real :: temperature_in(NLON, NLAT)
   
   integer :: i, j
   integer :: ndims_in, nvars_in
   integer :: errors
   character(len=256) :: att_text
   integer :: att_len
   real :: fill_value, fill_value_in
   
   ! ========== WRITE PHASE ==========
   print *, "Creating NetCDF file: ", FILE_NAME
   
   ! Initialize temperature data (synthetic: varies with lat and lon)
   do i = 1, NLAT
      do j = 1, NLON
         temperature(j, i) = 273.15 + (i-1) * 5.0 + (j-1) * 2.0
      end do
   end do
   
   ! Create the NetCDF file
   retval = nf90_create(FILE_NAME, NF90_CLOBBER, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define dimensions
   retval = nf90_def_dim(ncid, "lat", NLAT, lat_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "lon", NLON, lon_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define coordinate variables (same name as dimension)
   retval = nf90_def_var(ncid, "lat", NF90_FLOAT, lat_dimid, lat_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_var(ncid, "lon", NF90_FLOAT, lon_dimid, lon_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Add CF convention attributes to latitude
   retval = nf90_put_att(ncid, lat_varid, "units", "degrees_north")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, lat_varid, "standard_name", "latitude")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, lat_varid, "long_name", "Latitude")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, lat_varid, "axis", "Y")
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Add CF convention attributes to longitude
   retval = nf90_put_att(ncid, lon_varid, "units", "degrees_east")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, lon_varid, "standard_name", "longitude")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, lon_varid, "long_name", "Longitude")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, lon_varid, "axis", "X")
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define temperature variable (Fortran order: lon, lat)
   dimids(1) = lon_dimid
   dimids(2) = lat_dimid
   retval = nf90_def_var(ncid, "temperature", NF90_FLOAT, dimids, temp_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Add CF convention attributes to temperature
   retval = nf90_put_att(ncid, temp_varid, "units", "K")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, temp_varid, "standard_name", "air_temperature")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, temp_varid, "long_name", "Air Temperature")
   if (retval /= nf90_noerr) call handle_err(retval)
   
   fill_value = -999.0
   retval = nf90_put_att(ncid, temp_varid, "_FillValue", fill_value)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! End define mode
   retval = nf90_enddef(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Write coordinate variables
   retval = nf90_put_var(ncid, lat_varid, lat)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_var(ncid, lon_varid, lon)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Write temperature data
   retval = nf90_put_var(ncid, temp_varid, temperature)
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
   
   if (ndims_in /= 2) then
      print *, "Error: Expected 2 dimensions, found ", ndims_in
      stop 2
   end if
   print *, "Verified: ", ndims_in, " dimensions"
   
   if (nvars_in /= 3) then
      print *, "Error: Expected 3 variables, found ", nvars_in
      stop 2
   end if
   print *, "Verified: ", nvars_in, " variables (lat, lon, temperature)"
   
   ! Verify latitude attributes
   retval = nf90_inquire_attribute(ncid, lat_varid, "units", len=att_len)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_att(ncid, lat_varid, "units", att_text)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (trim(att_text) /= "degrees_north") then
      print *, "Error: lat units = '", trim(att_text), "', expected 'degrees_north'"
      stop 2
   end if
   print *, "Verified: lat units = '", trim(att_text), "'"
   
   retval = nf90_get_att(ncid, lat_varid, "standard_name", att_text)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (trim(att_text) /= "latitude") then
      print *, "Error: lat standard_name = '", trim(att_text), "', expected 'latitude'"
      stop 2
   end if
   print *, "Verified: lat standard_name = '", trim(att_text), "'"
   
   retval = nf90_get_att(ncid, lat_varid, "axis", att_text)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (trim(att_text) /= "Y") then
      print *, "Error: lat axis = '", trim(att_text), "', expected 'Y'"
      stop 2
   end if
   print *, "Verified: lat axis = '", trim(att_text), "'"
   
   ! Verify longitude attributes
   retval = nf90_get_att(ncid, lon_varid, "units", att_text)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (trim(att_text) /= "degrees_east") then
      print *, "Error: lon units = '", trim(att_text), "', expected 'degrees_east'"
      stop 2
   end if
   print *, "Verified: lon units = '", trim(att_text), "'"
   
   retval = nf90_get_att(ncid, lon_varid, "standard_name", att_text)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (trim(att_text) /= "longitude") then
      print *, "Error: lon standard_name = '", trim(att_text), "', expected 'longitude'"
      stop 2
   end if
   print *, "Verified: lon standard_name = '", trim(att_text), "'"
   
   ! Verify temperature attributes
   retval = nf90_get_att(ncid, temp_varid, "units", att_text)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (trim(att_text) /= "K") then
      print *, "Error: temperature units = '", trim(att_text), "', expected 'K'"
      stop 2
   end if
   print *, "Verified: temperature units = '", trim(att_text), "'"
   
   retval = nf90_get_att(ncid, temp_varid, "_FillValue", fill_value_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (fill_value_in /= fill_value) then
      print *, "Error: temperature _FillValue = ", fill_value_in, ", expected ", fill_value
      stop 2
   end if
   print *, "Verified: temperature _FillValue = ", fill_value_in
   
   ! Read coordinate variables
   retval = nf90_get_var(ncid, lat_varid, lat_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_var(ncid, lon_varid, lon_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Verify coordinate data
   errors = 0
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
      print *, "  lat: [", lat(1), ", ", lat(2), ", ", lat(3), ", ", lat(4), "]"
      print *, "  lon: [", lon(1), ", ", lon(2), ", ", lon(3), ", ", lon(4), ", ", lon(5), "]"
   end if
   
   ! Read temperature data
   retval = nf90_get_var(ncid, temp_varid, temperature_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Verify temperature data
   do i = 1, NLAT
      do j = 1, NLON
         if (temperature_in(j, i) /= temperature(j, i)) then
            print *, "Error: temperature(", j, ",", i, ") = ", temperature_in(j, i), &
                     ", expected ", temperature(j, i)
            errors = errors + 1
         end if
      end do
   end do
   
   if (errors > 0) then
      print *, "*** FAILED: ", errors, " data validation errors"
      stop 2
   end if
   
   print *, "Verified: all temperature data correct (", NLAT * NLON, " values)"
   
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
   
end program f_coord_vars
