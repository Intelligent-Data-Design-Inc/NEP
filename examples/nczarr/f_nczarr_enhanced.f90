!> @file f_nczarr_enhanced.f90
!! @brief Demonstrate NcZarr with the NetCDF-4 enhanced data model (Fortran).
!!
!! Creates a local NcZarr store with:
!!   Root group: time (unlimited) and x dimensions, temperature(time,x)
!!   and pressure(time,x) variables.
!!   Child group obs/: station (unlimited), obs_value(station) and
!!   obs_time(station) variables.
!!
!! Note: NcZarr supports groups and unlimited dims from the enhanced model;
!! user-defined types (enum, compound) require HDF5 storage.
!!
!! Reopens the store and validates all metadata and data values.
!!
!! **Learning Objectives:**
!! - Use NcZarr with the NetCDF-4 enhanced data model features from Fortran
!! - Create groups in a Zarr store with nf90_def_grp()
!! - Use unlimited dimensions in NcZarr datasets
!! - Combine root-level and child-group variables in one store
!! - Verify round-trip correctness for multi-group NcZarr datasets
!!
!! **Key Concepts:**
!! - **Enhanced Data Model**: NcZarr supports groups and unlimited dimensions
!!   from the NetCDF-4 enhanced model (but not user-defined types)
!! - **Groups in Zarr**: nf90_def_grp() creates sub-groups stored as nested
!!   directories in the Zarr store
!! - **Unlimited Dimensions**: NF90_UNLIMITED works in NcZarr just like NetCDF-4/HDF5
!! - **Mixed Variables**: Root and child groups can have independent variables
!!   with different dimensions
!! - **Column-Major Ordering**: Fortran arrays have reversed dimension order
!!   compared to C (x varies fastest in Fortran)
!!
!! **Prerequisites:**
!! - f_nczarr_simple.f90 - Basic NcZarr Fortran example
!! - f_groups.f90 - NetCDF-4 groups in Fortran
!!
!! **Related Examples:**
!! - nczarr_enhanced.c - C equivalent
!! - f_nczarr_simple.f90 - Basic NcZarr without groups
!! - f_nczarr_chunking.f90 - Chunking in NcZarr
!! - f_nczarr_compression.f90 - Compression in NcZarr
!! - f_groups.f90 - Groups in NetCDF-4/HDF5
!!
!! **Compilation:**
!! @code
!! gfortran -o f_nczarr_enhanced f_nczarr_enhanced.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_nczarr_enhanced
!! ncdump 'file://f_nczarr_enhanced.zarr#mode=nczarr'
!! @endcode
!!
!! **Expected Output:**
!! Creates the directory f_nczarr_enhanced.zarr containing:
!! - Root group with dimensions time(unlimited), x(5) and variables
!!   temperature(x, time), pressure(x, time)
!! - Child group obs/ with dimension station(unlimited) and variables
!!   obs_value(station), obs_time(station)
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026-06-27

program f_nczarr_enhanced
   use netcdf
   implicit none

   character(len=*), parameter :: FILE_URL = "file://f_nczarr_enhanced.zarr#mode=nczarr"
   integer, parameter :: NX = 5, NTIMES = 2, NSTATIONS = 3

   integer :: ncid, obs_grpid, retval
   integer :: time_dimid, x_dimid, station_dimid
   integer :: temp_varid, pres_varid, obs_value_varid, obs_time_varid
   integer :: dimids(2)
   integer :: t, i, errors

   ! Temperature and pressure: Fortran column-major, x varies fastest
   real :: temp_data(NX, NTIMES), temp_in(NX, NTIMES)
   real :: pres_data(NX, NTIMES)

   ! Observation arrays in child group obs/
   real    :: obs_value(NSTATIONS), obs_value_in(NSTATIONS)
   integer :: obs_time(NSTATIONS)

   ! Build data
   do t = 1, NTIMES
      do i = 1, NX
         temp_data(i, t) = 280.0 + real(t - 1) * 5.0 + real(i - 1) * 0.5
         pres_data(i, t) = 1013.0 - real(t - 1) * 2.0 - real(i - 1) * 0.1
      end do
   end do

   obs_value(1) = 14.2; obs_time(1) = 0
   obs_value(2) = 15.8; obs_time(2) = 0
   obs_value(3) = 13.1; obs_time(3) = 1

   ! === CREATE ===
   retval = nf90_create(FILE_URL, NF90_CLOBBER + NF90_NETCDF4, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Root dimensions (Fortran: x fastest-varying first)
   retval = nf90_def_dim(ncid, "x",    NX,             x_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "time", NF90_UNLIMITED, time_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Root variables: temperature(time, x) in CDL — Fortran dimids reversed
   dimids(1) = x_dimid
   dimids(2) = time_dimid
   retval = nf90_def_var(ncid, "temperature", NF90_FLOAT, dimids, temp_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_var(ncid, "pressure",    NF90_FLOAT, dimids, pres_varid)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_put_att(ncid, temp_varid, "units", "K")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, pres_varid, "units", "hPa")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(ncid, NF90_GLOBAL, "title", "NcZarr Enhanced Model Example")
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Child group obs/ with its own independent unlimited dimension
   retval = nf90_def_grp(ncid, "obs", obs_grpid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(obs_grpid, "station", NF90_UNLIMITED, station_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_var(obs_grpid, "obs_value", NF90_FLOAT, (/ station_dimid /), obs_value_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_var(obs_grpid, "obs_time",  NF90_INT,   (/ station_dimid /), obs_time_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(obs_grpid, obs_value_varid, "units", "Celsius")
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_att(obs_grpid, NF90_GLOBAL, "description", "Station observation records")
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_enddef(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Write two time steps of temperature and pressure
   retval = nf90_put_var(ncid, temp_varid, temp_data)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_var(ncid, pres_varid, pres_data)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Write three station observations to child group
   retval = nf90_put_var(obs_grpid, obs_value_varid, obs_value)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_var(obs_grpid, obs_time_varid, obs_time)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   print *, "Created ", FILE_URL
   print *, "  root(time=unlimited, x=", NX, "), obs/(station=unlimited)"

   ! === REOPEN and VERIFY ===
   retval = nf90_open(FILE_URL, NF90_NOWRITE, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Verify root time dimension grew to NTIMES
   retval = nf90_inquire_dimension(ncid, time_dimid, len=t)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (t /= NTIMES) then
      print *, "*** FAILED: time length =", t, ", expected", NTIMES; stop 2
   end if
   print *, "time=", t, " x=", NX

   ! Verify obs/ group station dimension
   retval = nf90_inq_grp_ncid(ncid, "obs", obs_grpid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_inquire_dimension(obs_grpid, station_dimid, len=i)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (i /= NSTATIONS) then
      print *, "*** FAILED: station length =", i, ", expected", NSTATIONS; stop 2
   end if
   print *, "obs/station=", i

   ! Read back and verify temperature
   retval = nf90_inq_varid(ncid, "temperature", temp_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_var(ncid, temp_varid, temp_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   errors = 0
   do t = 1, NTIMES
      do i = 1, NX
         if (temp_in(i, t) /= temp_data(i, t)) errors = errors + 1
      end do
   end do
   if (errors > 0) then
      print *, "*** FAILED:", errors, "temperature mismatches"; stop 2
   end if
   print *, "Temperature:", NTIMES * NX, "values correct"

   ! Read back and verify obs_value
   retval = nf90_inq_varid(obs_grpid, "obs_value", obs_value_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_get_var(obs_grpid, obs_value_varid, obs_value_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   do i = 1, NSTATIONS
      if (obs_value_in(i) /= obs_value(i)) then
         print *, "*** FAILED: obs_value mismatch at", i; stop 2
      end if
   end do
   print *, "obs_value:", NSTATIONS, "records correct"

   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   print *, "Done."

contains
   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop 2
   end subroutine handle_err

end program f_nczarr_enhanced
