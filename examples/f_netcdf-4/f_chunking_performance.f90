!> @file f_chunking_performance.f90
!! @brief Demonstrates chunking strategies and I/O performance impact (Fortran)
!!
!! Fortran equivalent of chunking_performance.c, exploring NetCDF-4 chunking using
!! the Fortran 90 NetCDF API. Creates datasets with different chunking strategies.
!!
!! **Learning Objectives:**
!! - Configure chunking with nf90_def_var_chunking() in Fortran
!! - Measure I/O performance for different chunk sizes
!! - Select optimal chunking for Fortran access patterns
!!
!! **Fortran Chunking Functions:**
!! - nf90_def_var_chunking(ncid, varid, storage, chunksizes)
!! - storage: NF90_CONTIGUOUS or NF90_CHUNKED
!! - chunksizes: Integer array with chunk dimensions
!!
!! **Prerequisites:**
!! - f_simple_nc4.f90 - NetCDF-4 format basics
!! - chunking_performance.c - C equivalent
!!
!! **Related Examples:**
!! - chunking_performance.c - C equivalent
!! - f_compression.f90 - Compression requires chunking
!!
!! **Compilation:**
!! @code
!! gfortran -o f_chunking_performance f_chunking_performance.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026

program f_chunking_performance
   use netcdf
   implicit none
   
   integer, parameter :: NTIME = 100
   integer, parameter :: NLAT = 180
   integer, parameter :: NLON = 360
   integer, parameter :: NDIMS = 3
   
   real, allocatable :: data(:,:,:)
   real(8) :: write_times(4), read_times(4)
   integer :: t, lat, lon
   
   ! Allocate data array
   allocate(data(NLON, NLAT, NTIME))
   
   print *, "Chunking Performance Demonstration"
   print *, "==================================="
   print *, "Dataset dimensions: [time=", NTIME, ", lat=", NLAT, ", lon=", NLON, "]"
   print *, "Total data points: ", NTIME * NLAT * NLON
   print *, "Total data size: ", (NTIME * NLAT * NLON * 4) / 1048576.0, " MB"
   
   ! Initialize data with simple pattern
   do t = 1, NTIME
      do lat = 1, NLAT
         do lon = 1, NLON
            data(lon, lat, t) = real((t-1) * 1000 + (lat-1) * 10 + (lon-1))
         end do
      end do
   end do
   
   ! Strategy 1: Contiguous storage (no chunking)
   call create_chunked_file("f_chunk_contiguous.nc", &
                           "Contiguous Storage", &
                           NF90_CONTIGUOUS, [0, 0, 0], data, write_times(1))
   
   ! Strategy 2: Time-optimized chunks
   call create_chunked_file("f_chunk_time_optimized.nc", &
                           "Time-Optimized Chunking", &
                           NF90_CHUNKED, [1, 1, 100], data, write_times(2))
   
   ! Strategy 3: Spatial-optimized chunks
   call create_chunked_file("f_chunk_spatial_optimized.nc", &
                           "Spatial-Optimized Chunking", &
                           NF90_CHUNKED, [360, 180, 1], data, write_times(3))
   
   ! Strategy 4: Balanced chunks
   call create_chunked_file("f_chunk_balanced.nc", &
                           "Balanced Chunking", &
                           NF90_CHUNKED, [90, 45, 10], data, write_times(4))
   
   ! Read and validate all files
   print *, ""
   print *, "=== Reading and Validating Files ==="
   call read_and_validate("f_chunk_contiguous.nc", "Contiguous", data, read_times(1))
   call read_and_validate("f_chunk_time_optimized.nc", "Time-optimized", data, read_times(2))
   call read_and_validate("f_chunk_spatial_optimized.nc", "Spatial-optimized", data, read_times(3))
   call read_and_validate("f_chunk_balanced.nc", "Balanced", data, read_times(4))
   
   ! Performance summary
   print *, ""
   print *, "=== Performance Summary ==="
   print '(A25, A12, A12)', "Strategy", "Write (s)", "Read (s)"
   print '(A25, F12.3, F12.3)', "Contiguous", write_times(1), read_times(1)
   print '(A25, F12.3, F12.3)', "Time-optimized", write_times(2), read_times(2)
   print '(A25, F12.3, F12.3)', "Spatial-optimized", write_times(3), read_times(3)
   print '(A25, F12.3, F12.3)', "Balanced", write_times(4), read_times(4)
   
   print *, ""
   print *, "=== Recommendations ==="
   print *, "- Contiguous storage: Best for small datasets or sequential access"
   print *, "- Time-optimized: Best for time-series analysis at specific locations"
   print *, "- Spatial-optimized: Best for spatial analysis at specific times"
   print *, "- Balanced: Good compromise for mixed access patterns"
   print *, "- Chunk size should align with typical access patterns"
   print *, "- Larger chunks reduce metadata overhead but may waste I/O"
   print *, "- Consider compression when using chunking (see f_compression.f90)"
   
   print *, ""
   print *, "*** SUCCESS: All chunking strategies tested!"
   
   deallocate(data)
   
contains

   subroutine create_chunked_file(filename, strategy_name, storage, chunksizes, data, write_time)
      character(len=*), intent(in) :: filename, strategy_name
      integer, intent(in) :: storage
      integer, intent(in) :: chunksizes(3)
      real, intent(in) :: data(:,:,:)
      real(8), intent(out) :: write_time
      
      integer :: ncid, varid
      integer :: time_dimid, lat_dimid, lon_dimid
      integer :: dimids(NDIMS)
      integer :: retval
      real(8) :: start_time, end_time
      integer :: file_size
      logical :: file_exists
      
      print *, ""
      print *, "=== ", trim(strategy_name), " ==="
      
      ! Start timing
      call cpu_time(start_time)
      
      ! Create file
      retval = nf90_create(filename, NF90_CLOBBER + NF90_NETCDF4, ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Define dimensions
      retval = nf90_def_dim(ncid, "time", NTIME, time_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_def_dim(ncid, "lat", NLAT, lat_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_def_dim(ncid, "lon", NLON, lon_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Define variable (Fortran order: lon, lat, time)
      dimids(1) = lon_dimid
      dimids(2) = lat_dimid
      dimids(3) = time_dimid
      retval = nf90_def_var(ncid, "temperature", NF90_FLOAT, dimids, varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Set chunking
      retval = nf90_def_var_chunking(ncid, varid, storage, chunksizes)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! End define mode
      retval = nf90_enddef(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Write data
      retval = nf90_put_var(ncid, varid, data)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Close file
      retval = nf90_close(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! End timing
      call cpu_time(end_time)
      write_time = end_time - start_time
      
      ! Get file size
      inquire(file=filename, exist=file_exists, size=file_size)
      if (file_exists) then
         print *, "File size: ", file_size, " bytes (", file_size / 1048576.0, " MB)"
      end if
      
      print *, "Write time: ", write_time, " seconds"
      
      if (storage == NF90_CHUNKED) then
         print *, "Chunk sizes: [", chunksizes(1), ",", chunksizes(2), ",", chunksizes(3), "]"
      else
         print *, "Storage: Contiguous (no chunking)"
      end if
      
   end subroutine create_chunked_file
   
   subroutine read_and_validate(filename, test_name, expected_data, read_time)
      character(len=*), intent(in) :: filename, test_name
      real, intent(in) :: expected_data(:,:,:)
      real(8), intent(out) :: read_time
      
      integer :: ncid, varid
      integer :: retval
      real(8) :: start_time, end_time
      real, allocatable :: data(:,:,:)
      integer :: storage
      integer :: chunksizes(NDIMS)
      integer :: errors
      integer :: t, lat, lon
      real :: expected, actual
      
      allocate(data(NLON, NLAT, NTIME))
      
      ! Start timing
      call cpu_time(start_time)
      
      ! Open file
      retval = nf90_open(filename, NF90_NOWRITE, ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Get variable ID
      retval = nf90_inq_varid(ncid, "temperature", varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Verify chunking settings
      retval = nf90_inq_var_chunking(ncid, varid, storage, chunksizes)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Read all data
      retval = nf90_get_var(ncid, varid, data)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Close file
      retval = nf90_close(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! End timing
      call cpu_time(end_time)
      read_time = end_time - start_time
      
      ! Validate a few data points (using expected_data for reference)
      errors = 0
      do t = 1, 5
         do lat = 1, 5
            do lon = 1, 5
               expected = expected_data(lon, lat, t)
               actual = data(lon, lat, t)
               if (actual /= expected) then
                  print *, "Error: data(", lon, ",", lat, ",", t, ") = ", actual, &
                           ", expected ", expected
                  errors = errors + 1
               end if
            end do
         end do
      end do
      
      if (errors > 0) then
         print *, "*** FAILED: ", errors, " validation errors"
         stop 2
      end if
      
      print *, trim(test_name), " read time: ", read_time, " seconds"
      
      deallocate(data)
      
   end subroutine read_and_validate
   
   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop 2
   end subroutine handle_err
   
end program f_chunking_performance
