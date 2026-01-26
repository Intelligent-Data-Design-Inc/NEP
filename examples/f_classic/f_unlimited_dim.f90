!> @file f_unlimited_dim.f90
!! @brief Demonstrates unlimited dimensions for appendable time-series data (Fortran)
!!
!! This is the Fortran equivalent of unlimited_dim.c, demonstrating unlimited
!! dimensions using the Fortran 90 NetCDF API. The program creates a file with
!! an unlimited time dimension and demonstrates appending data.
!!
!! **Learning Objectives:**
!! - Understand NF90_UNLIMITED in Fortran
!! - Learn to append data to existing files in Fortran
!! - Work with time-series data patterns in Fortran
!! - Master file reopening with NF90_WRITE mode
!!
!! **Fortran Unlimited Dimension:**
!! - Define with nf90_def_dim(ncid, "time", NF90_UNLIMITED, dimid)
!! - Must be first dimension in Fortran (last in C)
!! - Use nf90_put_var with start and count for appending
!!
!! **Prerequisites:**
!! - f_simple_2D.f90 - Basic file operations
!! - unlimited_dim.c - C equivalent for comparison
!!
!! **Related Examples:**
!! - unlimited_dim.c - C equivalent
!! - f_multi_unlimited.f90 - Multiple unlimited dimensions (NetCDF-4)
!! - f_var4d.f90 - 4D data with time dimension
!!
!! **Compilation:**
!! @code
!! gfortran -o f_unlimited_dim f_unlimited_dim.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_unlimited_dim
!! ncdump f_unlimited_dim.nc
!! @endcode
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026

program f_unlimited_dim
   use netcdf
   implicit none
   
   character(len=*), parameter :: FILE_NAME = "f_unlimited_dim.nc"
   integer, parameter :: NLAT = 4, NLON = 5
   integer, parameter :: INITIAL_TIMESTEPS = 3
   integer, parameter :: APPEND_TIMESTEPS = 2
   integer, parameter :: TOTAL_TIMESTEPS = INITIAL_TIMESTEPS + APPEND_TIMESTEPS
   
   integer :: ncid, time_varid, temp_varid
   integer :: time_dimid, lat_dimid, lon_dimid
   integer :: dimids(3)
   integer :: retval
   
   real :: time_data(TOTAL_TIMESTEPS) = (/0.0, 1.0, 2.0, 3.0, 4.0/)
   real :: temp_data(NLON, NLAT, TOTAL_TIMESTEPS)
   real :: temp_in(NLON, NLAT, TOTAL_TIMESTEPS)
   
   integer :: i, j, t
   integer :: current_size, final_size
   integer :: unlimdimid
   integer :: errors
   real :: time_in(TOTAL_TIMESTEPS)
   integer :: start(3), count(3)
   
   ! ========== WRITE PHASE (Initial) ==========
   print *, "Creating NetCDF file: ", FILE_NAME
   
   ! Initialize temperature data for all timesteps
   do t = 1, TOTAL_TIMESTEPS
      do i = 1, NLAT
         do j = 1, NLON
            temp_data(j, i, t) = 273.15 + (t-1) * 1.0 + (i-1) * 5.0 + (j-1) * 2.0
         end do
      end do
   end do
   
   ! Create the NetCDF file
   retval = nf90_create(FILE_NAME, NF90_CLOBBER, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define dimensions - time is unlimited
   retval = nf90_def_dim(ncid, "time", NF90_UNLIMITED, time_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "lat", NLAT, lat_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "lon", NLON, lon_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define time coordinate variable
   retval = nf90_def_var(ncid, "time", NF90_FLOAT, time_dimid, time_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define temperature variable (lon, lat, time) - Fortran order
   dimids(1) = lon_dimid
   dimids(2) = lat_dimid
   dimids(3) = time_dimid
   retval = nf90_def_var(ncid, "temperature", NF90_FLOAT, dimids, temp_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! End define mode
   retval = nf90_enddef(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Write initial timesteps (0, 1, 2)
   start = (/1, 1, 1/)
   count = (/INITIAL_TIMESTEPS, 1, 1/)
   
   retval = nf90_put_var(ncid, time_varid, time_data(1:INITIAL_TIMESTEPS), &
                         start=start(1:1), count=count(1:1))
   if (retval /= nf90_noerr) call handle_err(retval)
   
   count = (/NLON, NLAT, INITIAL_TIMESTEPS/)
   retval = nf90_put_var(ncid, temp_varid, temp_data(:,:,1:INITIAL_TIMESTEPS), &
                         start=start, count=count)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Close the file
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   print *, "*** SUCCESS writing initial ", INITIAL_TIMESTEPS, " timesteps!"
   
   ! ========== APPEND PHASE ==========
   print *, ""
   print *, "Reopening file to append data..."
   
   ! Reopen file in write mode
   retval = nf90_open(FILE_NAME, NF90_WRITE, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Get variable IDs
   retval = nf90_inq_varid(ncid, "time", time_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_inq_varid(ncid, "temperature", temp_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Query current time dimension size
   retval = nf90_inquire_dimension(ncid, time_dimid, len=current_size)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   if (current_size /= INITIAL_TIMESTEPS) then
      print *, "Error: Expected ", INITIAL_TIMESTEPS, " timesteps, found ", current_size
      stop 2
   end if
   print *, "Current time dimension size: ", current_size
   
   ! Append additional timesteps (3, 4)
   start(1) = INITIAL_TIMESTEPS + 1
   count(1) = APPEND_TIMESTEPS
   
   retval = nf90_put_var(ncid, time_varid, &
                         time_data(INITIAL_TIMESTEPS+1:TOTAL_TIMESTEPS), &
                         start=start(1:1), count=count(1:1))
   if (retval /= nf90_noerr) call handle_err(retval)
   
   start = (/1, 1, INITIAL_TIMESTEPS + 1/)
   count = (/NLON, NLAT, APPEND_TIMESTEPS/)
   retval = nf90_put_var(ncid, temp_varid, &
                         temp_data(:,:,INITIAL_TIMESTEPS+1:TOTAL_TIMESTEPS), &
                         start=start, count=count)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Close the file
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   print *, "*** SUCCESS appending ", APPEND_TIMESTEPS, " timesteps!"
   
   ! ========== READ PHASE ==========
   print *, ""
   print *, "Reopening file for validation..."
   
   ! Open the file for reading
   retval = nf90_open(FILE_NAME, NF90_NOWRITE, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Verify unlimited dimension
   retval = nf90_inquire(ncid, unlimitedDimId=unlimdimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   if (unlimdimid /= time_dimid) then
      print *, "Error: time dimension is not unlimited"
      stop 2
   end if
   print *, "Verified: time dimension is unlimited"
   
   ! Verify total timesteps
   retval = nf90_inquire_dimension(ncid, time_dimid, len=final_size)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   if (final_size /= TOTAL_TIMESTEPS) then
      print *, "Error: Expected ", TOTAL_TIMESTEPS, " total timesteps, found ", final_size
      stop 2
   end if
   print *, "Verified: ", final_size, " total timesteps after append"
   
   ! Read all time values
   retval = nf90_get_var(ncid, time_varid, time_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Verify time values
   errors = 0
   do t = 1, TOTAL_TIMESTEPS
      if (time_in(t) /= time_data(t)) then
         print *, "Error: time(", t, ") = ", time_in(t), ", expected ", time_data(t)
         errors = errors + 1
      end if
   end do
   
   if (errors == 0) then
      print *, "Verified: time coordinate values correct [", &
               time_in(1), ", ", time_in(2), ", ", time_in(3), ", ", &
               time_in(4), ", ", time_in(5), "]"
   end if
   
   ! Read all temperature data
   retval = nf90_get_var(ncid, temp_varid, temp_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Verify temperature data continuity
   do t = 1, TOTAL_TIMESTEPS
      do i = 1, NLAT
         do j = 1, NLON
            if (temp_in(j, i, t) /= temp_data(j, i, t)) then
               print *, "Error: temperature(", j, ",", i, ",", t, ") = ", &
                        temp_in(j, i, t), ", expected ", temp_data(j, i, t)
               errors = errors + 1
            end if
         end do
      end do
   end do
   
   if (errors > 0) then
      print *, "*** FAILED: ", errors, " data validation errors"
      stop 2
   end if
   
   print *, "Verified: all temperature data correct (", TOTAL_TIMESTEPS, " timesteps x ", &
            NLAT, " x ", NLON, " = ", TOTAL_TIMESTEPS * NLAT * NLON, " values)"
   print *, "  Initial write: timesteps 0-", INITIAL_TIMESTEPS - 1
   print *, "  Appended: timesteps ", INITIAL_TIMESTEPS, "-", TOTAL_TIMESTEPS - 1
   
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
   
end program f_unlimited_dim
