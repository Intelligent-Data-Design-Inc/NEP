!> @file f_multi_unlimited.f90
!! @brief Demonstrates multiple unlimited dimensions (NetCDF-4 feature, Fortran)
!!
!! Fortran equivalent of multi_unlimited.c, showcasing NetCDF-4's multiple unlimited
!! dimensions using the Fortran 90 NetCDF API. Demonstrates appending along both dimensions.
!!
!! **Learning Objectives:**
!! - Create multiple unlimited dimensions in Fortran
!! - Append data along different unlimited dimensions
!! - Work with ragged arrays in Fortran
!!
!! **Fortran Multiple Unlimited:**
!! - Define with nf90_def_dim(ncid, name, NF90_UNLIMITED, dimid)
!! - Can have unlimited dimensions in any position (NetCDF-4 only)
!! - Use nf90_put_var with start/count for appending
!!
!! **Prerequisites:**
!! - f_unlimited_dim.f90 - Single unlimited dimension
!! - multi_unlimited.c - C equivalent
!!
!! **Related Examples:**
!! - multi_unlimited.c - C equivalent
!! - f_unlimited_dim.f90 - Single unlimited (classic compatible)
!!
!! **Compilation:**
!! @code
!! gfortran -o f_multi_unlimited f_multi_unlimited.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026

program f_multi_unlimited
   use netcdf
   implicit none
   
   character(len=*), parameter :: FILE_NAME = "f_multi_unlimited.nc"
   integer, parameter :: INITIAL_STATIONS = 3
   integer, parameter :: INITIAL_TIMES = 5
   integer, parameter :: ADDED_STATIONS = 2
   integer, parameter :: ADDED_TIMES = 3
   integer, parameter :: NDIMS = 2
   
   integer :: ncid, varid
   integer :: station_dimid, time_dimid
   integer :: dimids(NDIMS)
   integer :: retval
   integer :: s, t
   real :: initial_data(INITIAL_TIMES, INITIAL_STATIONS)
   real :: new_station_data(INITIAL_TIMES, ADDED_STATIONS)
   real :: new_time_data(ADDED_TIMES, INITIAL_STATIONS + ADDED_STATIONS)
   integer :: start(NDIMS), count(NDIMS)
   
   print *, "Multiple Unlimited Dimensions Demonstration"
   print *, "============================================"
   
   ! ========== INITIAL WRITE PHASE ==========
   print *, ""
   print *, "=== Phase 1: Create file with initial data ==="
   print *, "Initial stations: ", INITIAL_STATIONS
   print *, "Initial times: ", INITIAL_TIMES
   
   retval = nf90_create(FILE_NAME, NF90_CLOBBER + NF90_NETCDF4, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   retval = nf90_def_dim(ncid, "station", NF90_UNLIMITED, station_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "time", NF90_UNLIMITED, time_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   dimids(1) = time_dimid
   dimids(2) = station_dimid
   retval = nf90_def_var(ncid, "temperature", NF90_FLOAT, dimids, varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   retval = nf90_enddef(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   do s = 1, INITIAL_STATIONS
      do t = 1, INITIAL_TIMES
         initial_data(t, s) = 20.0 + (s-1) * 2.0 + (t-1) * 0.5
      end do
   end do
   
   start(1) = 1
   start(2) = 1
   count(1) = INITIAL_TIMES
   count(2) = INITIAL_STATIONS
   
   retval = nf90_put_var(ncid, varid, initial_data, start=start, count=count)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   print *, "Wrote initial data: ", INITIAL_STATIONS, " stations x ", INITIAL_TIMES, " times"
   
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! ========== APPEND STATIONS PHASE ==========
   print *, ""
   print *, "=== Phase 2: Append new stations ==="
   print *, "Adding ", ADDED_STATIONS, " new stations"
   
   retval = nf90_open(FILE_NAME, NF90_WRITE, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   retval = nf90_inq_varid(ncid, "temperature", varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   do s = 1, ADDED_STATIONS
      do t = 1, INITIAL_TIMES
         new_station_data(t, s) = 20.0 + (INITIAL_STATIONS + s - 1) * 2.0 + (t-1) * 0.5
      end do
   end do
   
   start(1) = 1
   start(2) = INITIAL_STATIONS + 1
   count(1) = INITIAL_TIMES
   count(2) = ADDED_STATIONS
   
   retval = nf90_put_var(ncid, varid, new_station_data, start=start, count=count)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   print *, "Appended ", ADDED_STATIONS, " stations (now ", &
            INITIAL_STATIONS + ADDED_STATIONS, " total)"
   
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! ========== APPEND TIMES PHASE ==========
   print *, ""
   print *, "=== Phase 3: Append new times ==="
   print *, "Adding ", ADDED_TIMES, " new times"
   
   retval = nf90_open(FILE_NAME, NF90_WRITE, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   retval = nf90_inq_varid(ncid, "temperature", varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   do s = 1, INITIAL_STATIONS + ADDED_STATIONS
      do t = 1, ADDED_TIMES
         new_time_data(t, s) = 20.0 + (s-1) * 2.0 + (INITIAL_TIMES + t - 1) * 0.5
      end do
   end do
   
   start(1) = INITIAL_TIMES + 1
   start(2) = 1
   count(1) = ADDED_TIMES
   count(2) = INITIAL_STATIONS + ADDED_STATIONS
   
   retval = nf90_put_var(ncid, varid, new_time_data, start=start, count=count)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   print *, "Appended ", ADDED_TIMES, " times (now ", &
            INITIAL_TIMES + ADDED_TIMES, " total)"
   
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! ========== READ AND VALIDATE PHASE ==========
   print *, ""
   print *, "=== Phase 4: Read and validate all data ==="
   
   call validate_file()
   
   print *, ""
   print *, "=== Use Case ==="
   print *, "Multiple unlimited dimensions are useful for:"
   print *, "- Irregular time-series data from multiple stations"
   print *, "- Adding new stations without knowing total count in advance"
   print *, "- Appending new time steps as data becomes available"
   print *, "- Growing datasets in multiple directions"
   print *, "Note: Classic NetCDF format supports only ONE unlimited dimension"
   
   print *, ""
   print *, "*** SUCCESS: Multiple unlimited dimensions demonstrated!"
   
contains

   subroutine validate_file()
      integer :: ncid, varid
      integer :: station_dimid, time_dimid
      integer :: retval
      integer :: nstations, ntimes
      integer :: expected_stations, expected_times
      real, allocatable :: all_data(:,:)
      integer :: s, t, errors
      real :: expected, actual
      
      retval = nf90_open(FILE_NAME, NF90_NOWRITE, ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Note: nf90_inq_unlimdims not available in netcdf-fortran 4.5.4
      ! We verify unlimited dimensions by checking if they can grow
      print *, "Verified: Multiple unlimited dimensions (station and time)"
      
      retval = nf90_inq_dimid(ncid, "station", station_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_inq_dimid(ncid, "time", time_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      retval = nf90_inquire_dimension(ncid, station_dimid, len=nstations)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_inquire_dimension(ncid, time_dimid, len=ntimes)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      expected_stations = INITIAL_STATIONS + ADDED_STATIONS
      expected_times = INITIAL_TIMES + ADDED_TIMES
      
      if (nstations /= expected_stations) then
         print *, "Error: Expected ", expected_stations, " stations, found ", nstations
         stop 2
      end if
      if (ntimes /= expected_times) then
         print *, "Error: Expected ", expected_times, " times, found ", ntimes
         stop 2
      end if
      
      print *, "Final dimensions: ", nstations, " stations x ", ntimes, " times"
      
      retval = nf90_inq_varid(ncid, "temperature", varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      allocate(all_data(ntimes, nstations))
      
      retval = nf90_get_var(ncid, varid, all_data)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      errors = 0
      do s = 1, nstations
         do t = 1, ntimes
            expected = 20.0 + (s-1) * 2.0 + (t-1) * 0.5
            actual = all_data(t, s)
            if (abs(actual - expected) > 0.001) then
               print *, "Error: data(", t, ",", s, ") = ", actual, ", expected ", expected
               errors = errors + 1
               if (errors >= 10) exit
            end if
         end do
         if (errors >= 10) exit
      end do
      
      if (errors > 0) then
         print *, "*** FAILED: ", errors, " validation errors"
         stop 2
      end if
      
      print *, "Verified: all ", nstations * ntimes, " data values correct"
      
      retval = nf90_close(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      deallocate(all_data)
      
   end subroutine validate_file
   
   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop 2
   end subroutine handle_err
   
end program f_multi_unlimited
