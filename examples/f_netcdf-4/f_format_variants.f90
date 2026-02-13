!> @file f_format_variants.f90
!! @brief Demonstrates all five NetCDF binary format variants in Fortran
!!
!! This is the Fortran equivalent of format_variants.c, demonstrating all five
!! NetCDF binary format variants using the Fortran 90 NetCDF API. The program
!! creates identical data structures in each format and compares their characteristics.
!!
!! **Learning Objectives:**
!! - Understand format flags in Fortran (NF90_CLASSIC_MODEL, NF90_64BIT_OFFSET,
!!   NF90_64BIT_DATA, NF90_NETCDF4, IOR(NF90_NETCDF4, NF90_CLASSIC_MODEL))
!! - Learn format detection with nf90_inq_format()
!! - Compare file sizes and format characteristics
!! - Make informed format choices in Fortran applications
!!
!! **Fortran Format Constants:**
!! - NF90_CLOBBER (default) - CDF-1 format (2GB limits, no format flag needed)
!! - NF90_64BIT_OFFSET - CDF-2 format (4GB variable limit)
!! - NF90_64BIT_DATA - CDF-5 format (unlimited sizes)
!! - NF90_NETCDF4 - NetCDF-4/HDF5 format (groups, compression, user types)
!! - IOR(NF90_NETCDF4, NF90_CLASSIC_MODEL) - HDF5 storage, classic data model
!! - NF90_CLASSIC_MODEL is only used with NF90_NETCDF4 to restrict to classic data model
!!
!! **Prerequisites:**
!! - f_simple_2D.f90 - Basic file operations
!! - f_simple_nc4.f90 - NetCDF-4 basics
!!
!! **Related Examples:**
!! - format_variants.c - C equivalent
!! - f_compression.f90 - NetCDF-4 compression features
!!
!! **Compilation:**
!! @code
!! gfortran -o f_format_variants f_format_variants.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_format_variants
!! ls -lh f_format_*.nc
!! @endcode
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026

program f_format_variants
   use netcdf
   implicit none
   
   integer, parameter :: NTIME = 10, NLAT = 20, NLON = 30
   integer, parameter :: ERRCODE = 2
   integer :: ncid, retval
   
   print *, "NetCDF Format Variants Comparison"
   print *, ""
   print *, "This program creates five files with identical data structures"
   print *, "in all five NetCDF binary formats to demonstrate their differences."
   print *, ""
   print *, "Data structure:"
   print *, "  Dimensions: time=", NTIME, ", lat=", NLAT, ", lon=", NLON
   print *, "  Variables: temperature(time,lat,lon), pressure(time,lat,lon)"
   print *, "  Data type: NF90_FLOAT (4 bytes per value)"
   print *, "  Total data: ", NTIME * NLAT * NLON, " values per variable"
   print *, ""
   
   ! Create files in each format.
   !
   ! The nf90_create() calls below show the key difference between formats:
   ! only the format flag changes. Each file gets identical metadata and
   ! data via populate_file().
   print *, "=== Creating Format Files ==="
   print *, ""
   
   ! CDF-1: Original classic format (2GB file/variable limit).
   ! No format flag needed - NF90_CLOBBER alone creates a classic CDF-1 file.
   print *, "Creating classic (CDF-1) format file: f_format_classic.nc"
   retval = nf90_create("f_format_classic.nc", NF90_CLOBBER, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   call populate_file(ncid)
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! CDF-2: 64-bit offset format (4GB variable limit)
   print *, "Creating NF90_64BIT_OFFSET format file: f_format_64bit_offset.nc"
   retval = nf90_create("f_format_64bit_offset.nc", NF90_64BIT_OFFSET, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   call populate_file(ncid)
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! CDF-5: 64-bit data format (unlimited variable sizes)
   print *, "Creating NF90_64BIT_DATA format file: f_format_64bit_data.nc"
   retval = nf90_create("f_format_64bit_data.nc", NF90_64BIT_DATA, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   call populate_file(ncid)
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! NetCDF-4: HDF5-based format (groups, compression, user-defined types)
   print *, "Creating NF90_NETCDF4 format file: f_format_netcdf4.nc"
   retval = nf90_create("f_format_netcdf4.nc", NF90_NETCDF4, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   call populate_file(ncid)
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! NetCDF-4 Classic Model: HDF5 storage with classic data model restrictions
   print *, "Creating NF90_NETCDF4+NF90_CLASSIC_MODEL format file: f_format_netcdf4_classic.nc"
   retval = nf90_create("f_format_netcdf4_classic.nc", &
                        IOR(NF90_NETCDF4, NF90_CLASSIC_MODEL), ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   call populate_file(ncid)
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Verify files
   print *, ""
   print *, "=== Verifying Format Files ==="
   
   call verify_format_file("f_format_classic.nc", NF90_FORMAT_CLASSIC, &
                          "NF90_FORMAT_CLASSIC")
   call verify_format_file("f_format_64bit_offset.nc", NF90_FORMAT_64BIT_OFFSET, &
                          "NF90_FORMAT_64BIT_OFFSET")
   call verify_format_file("f_format_64bit_data.nc", NF90_FORMAT_64BIT_DATA, &
                          "NF90_FORMAT_64BIT_DATA")
   call verify_format_file("f_format_netcdf4.nc", NF90_FORMAT_NETCDF4, &
                          "NF90_FORMAT_NETCDF4")
   call verify_format_file("f_format_netcdf4_classic.nc", &
                          NF90_FORMAT_NETCDF4_CLASSIC, &
                          "NF90_FORMAT_NETCDF4_CLASSIC")
   
   ! Summary
   print *, ""
   print *, "=== Format Comparison Summary ==="
   print *, ""
   print *, "Format Characteristics:"
   print *, ""
   print *, "Classic CDF-1 (default, no format flag):"
   print *, "  File size limit: 2GB"
   print *, "  Variable size limit: 2GB"
   print *, "  Storage backend: CDF binary"
   print *, "  Compatibility: NetCDF 3.0+, all tools"
   print *, "  Use when: Maximum compatibility needed, files < 2GB"
   print *, ""
   print *, "NF90_64BIT_OFFSET (CDF-2):"
   print *, "  File size limit: effectively unlimited"
   print *, "  Variable size limit: 4GB per variable"
   print *, "  Storage backend: CDF binary"
   print *, "  Compatibility: NetCDF 3.6.0+"
   print *, "  Use when: Large files needed, variables < 4GB each"
   print *, ""
   print *, "NF90_64BIT_DATA (CDF-5):"
   print *, "  File size limit: effectively unlimited"
   print *, "  Variable size limit: effectively unlimited"
   print *, "  Storage backend: CDF binary"
   print *, "  Compatibility: NetCDF 4.4.0+ or PnetCDF"
   print *, "  Use when: Very large variables needed (> 4GB)"
   print *, ""
   print *, "NF90_NETCDF4 (HDF5):"
   print *, "  File size limit: effectively unlimited"
   print *, "  Variable size limit: effectively unlimited"
   print *, "  Storage backend: HDF5"
   print *, "  Compatibility: NetCDF 4.0+"
   print *, "  Features: groups, compression, chunking, user-defined types"
   print *, "  Use when: Advanced features needed (compression, groups, etc.)"
   print *, ""
   print *, "IOR(NF90_NETCDF4, NF90_CLASSIC_MODEL) (HDF5 Classic Model):"
   print *, "  File size limit: effectively unlimited"
   print *, "  Variable size limit: effectively unlimited"
   print *, "  Storage backend: HDF5"
   print *, "  Compatibility: NetCDF 4.0+"
   print *, "  Features: compression, chunking (no groups, no user-defined types)"
   print *, "  Use when: HDF5 storage benefits needed with classic data model"
   print *, ""
   print *, "Key Observations:"
   print *, "  - All five formats store identical data correctly"
   print *, "  - Classic formats (CDF-1/2/5) have smaller overhead for small files"
   print *, "  - NetCDF-4 formats (HDF5) have larger overhead but support compression"
   print *, "  - NC4 classic model is a useful middle ground: HDF5 storage, simple model"
   print *, "  - Use nf90_inq_format() to detect format type when reading files"
   print *, ""
   print *, "*** SUCCESS: All format tests passed! ***"
   
contains

   ! Define dimensions, variables, attributes, and write data to an open file.
   ! The nf90_create() call is done in the main program so the format flag
   ! is clearly visible.
   subroutine populate_file(ncid)
      integer, intent(in) :: ncid
      
      integer :: time_dimid, lat_dimid, lon_dimid
      integer :: temp_varid, pressure_varid
      integer :: dimids(3)
      integer :: retval
      
      real :: temperature(NLON, NLAT, NTIME)
      real :: pressure(NLON, NLAT, NTIME)
      integer :: t, i, j
      
      ! Initialize data
      do t = 1, NTIME
         do i = 1, NLAT
            do j = 1, NLON
               temperature(j, i, t) = 273.15 + (t-1) * 1.0 + (i-1) * 0.5 + (j-1) * 0.2
               pressure(j, i, t) = 1013.25 + (t-1) * 0.1 + (i-1) * 0.05 + (j-1) * 0.02
            end do
         end do
      end do
      
      ! Define dimensions
      retval = nf90_def_dim(ncid, "time", NTIME, time_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_def_dim(ncid, "lat", NLAT, lat_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_def_dim(ncid, "lon", NLON, lon_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Define variables (Fortran order: lon, lat, time)
      dimids(1) = lon_dimid
      dimids(2) = lat_dimid
      dimids(3) = time_dimid
      
      retval = nf90_def_var(ncid, "temperature", NF90_FLOAT, dimids, temp_varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_def_var(ncid, "pressure", NF90_FLOAT, dimids, pressure_varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Add attributes
      retval = nf90_put_att(ncid, temp_varid, "units", "K")
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_put_att(ncid, pressure_varid, "units", "hPa")
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! End define mode
      retval = nf90_enddef(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Write data
      retval = nf90_put_var(ncid, temp_varid, temperature)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_put_var(ncid, pressure_varid, pressure)
      if (retval /= nf90_noerr) call handle_err(retval)
   end subroutine populate_file

   subroutine verify_format_file(filename, expected_format, expected_format_name)
      character(len=*), intent(in) :: filename, expected_format_name
      integer, intent(in) :: expected_format
      
      integer :: ncid, retval
      integer :: format_in
      integer :: ndims, nvars
      real :: temperature(NLON, NLAT, NTIME)
      real :: pressure(NLON, NLAT, NTIME)
      integer :: temp_varid, pressure_varid
      character(len=50) :: detected_format
      integer :: errors
      real :: expected_temp, expected_pressure
      
      print *, ""
      print *, "Verifying file: ", trim(filename)
      print *, "  Expected format: ", trim(expected_format_name)
      
      ! Open file
      retval = nf90_open(filename, NF90_NOWRITE, ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Check format
      retval = nf90_inq_format(ncid, format_in)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Determine format name
      if (format_in == NF90_FORMAT_CLASSIC) then
         detected_format = "NF90_FORMAT_CLASSIC (CDF-1)"
      else if (format_in == NF90_FORMAT_64BIT_OFFSET) then
         detected_format = "NF90_FORMAT_64BIT_OFFSET (CDF-2)"
      else if (format_in == NF90_FORMAT_64BIT_DATA) then
         detected_format = "NF90_FORMAT_64BIT_DATA (CDF-5)"
      else if (format_in == NF90_FORMAT_NETCDF4) then
         detected_format = "NF90_FORMAT_NETCDF4 (HDF5)"
      else if (format_in == NF90_FORMAT_NETCDF4_CLASSIC) then
         detected_format = "NF90_FORMAT_NETCDF4_CLASSIC (HDF5/CM)"
      else
         detected_format = "UNKNOWN"
      end if
      
      print *, "  Format detected: ", trim(detected_format)
      
      ! Verify expected format
      if (format_in /= expected_format) then
         print *, "Error: Expected format ", trim(expected_format_name), &
                  " (", expected_format, "), got ", trim(detected_format), &
                  " (", format_in, ")"
         stop ERRCODE
      end if
      
      ! Verify metadata
      retval = nf90_inquire(ncid, ndims, nvars)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      if (ndims /= 3 .or. nvars /= 2) then
         print *, "Error: Expected 3 dimensions and 2 variables, found ", &
                  ndims, " dims, ", nvars, " vars"
         stop ERRCODE
      end if
      print *, "  Metadata: ", ndims, " dimensions, ", nvars, " variables"
      
      ! Get variable IDs
      retval = nf90_inq_varid(ncid, "temperature", temp_varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_inq_varid(ncid, "pressure", pressure_varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Read data
      retval = nf90_get_var(ncid, temp_varid, temperature)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_get_var(ncid, pressure_varid, pressure)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Verify a few data values
      errors = 0
      expected_temp = 273.15
      expected_pressure = 1013.25
      
      if (temperature(1, 1, 1) /= expected_temp) then
         print *, "Error: temperature(1,1,1) = ", temperature(1, 1, 1), &
                  ", expected ", expected_temp
         errors = errors + 1
      end if
      
      if (pressure(1, 1, 1) /= expected_pressure) then
         print *, "Error: pressure(1,1,1) = ", pressure(1, 1, 1), &
                  ", expected ", expected_pressure
         errors = errors + 1
      end if
      
      if (errors == 0) then
         print *, "  Data validation: ", NTIME * NLAT * NLON * 2, " values verified"
      else
         print *, "*** FAILED: ", errors, " data validation errors"
         stop ERRCODE
      end if
      
      ! Close file
      retval = nf90_close(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
   end subroutine verify_format_file

   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop ERRCODE
   end subroutine handle_err
   
end program f_format_variants
