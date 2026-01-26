!> @file f_compression.f90
!! @brief Demonstrates NetCDF-4 compression filters with performance analysis (Fortran)
!!
!! Fortran equivalent of compression.c, exploring NetCDF-4 compression using the
!! Fortran 90 NetCDF API. Tests various compression configurations and measures performance.
!!
!! **Learning Objectives:**
!! - Configure compression with nf90_def_var_deflate() in Fortran
!! - Measure compression performance in Fortran applications
!! - Select appropriate compression levels for different data
!!
!! **Fortran Compression Functions:**
!! - nf90_def_var_deflate(ncid, varid, shuffle, deflate, deflate_level)
!! - shuffle: 0=off, 1=on
!! - deflate: 0=off, 1=on
!! - deflate_level: 1-9 (higher=better compression, slower)
!!
!! **Prerequisites:**
!! - f_simple_nc4.f90 - NetCDF-4 format basics
!! - compression.c - C equivalent
!!
!! **Related Examples:**
!! - compression.c - C equivalent
!! - f_chunking_performance.f90 - Chunking impacts compression
!!
!! **Compilation:**
!! @code
!! gfortran -o f_compression f_compression.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026

program f_compression
   use netcdf
   implicit none
   
   integer, parameter :: NTIME = 50
   integer, parameter :: NLAT = 90
   integer, parameter :: NLON = 180
   integer, parameter :: NDIMS = 3
   integer, parameter :: NUM_TESTS = 6
   
   real, allocatable :: data(:,:,:)
   real(8) :: write_times(NUM_TESTS), read_times(NUM_TESTS)
   integer :: file_sizes(NUM_TESTS)
   real(8) :: compression_ratios(NUM_TESTS)
   character(len=64) :: test_names(NUM_TESTS)
   character(len=128) :: filenames(NUM_TESTS)
   integer :: shuffle_flags(NUM_TESTS), deflate_flags(NUM_TESTS), deflate_levels(NUM_TESTS)
   integer :: i, t, lat, lon
   real :: base_temp, seasonal, spatial
   real, parameter :: PI = 3.14159265359
   
   print *, "Compression Filter Demonstration"
   print *, "================================="
   print *, "Dataset dimensions: [time=", NTIME, ", lat=", NLAT, ", lon=", NLON, "]"
   print *, "Total data points: ", NTIME * NLAT * NLON
   print *, "Total data size: ", (NTIME * NLAT * NLON * 4) / 1048576.0, " MB"
   
   ! Allocate and generate realistic temperature data
   allocate(data(NLON, NLAT, NTIME))
   
   do t = 1, NTIME
      do lat = 1, NLAT
         do lon = 1, NLON
            base_temp = 15.0 - (lat - NLAT/2) * 0.5
            seasonal = 10.0 * sin(2.0 * PI * t / NTIME)
            spatial = 5.0 * sin(2.0 * PI * lon / NLON) * cos(2.0 * PI * lat / NLAT)
            data(lon, lat, t) = base_temp + seasonal + spatial
         end do
      end do
   end do
   
   ! Define compression tests
   test_names(1) = "Uncompressed (baseline)"
   filenames(1) = "f_compress_none.nc"
   shuffle_flags(1) = 0
   deflate_flags(1) = 0
   deflate_levels(1) = 0
   
   test_names(2) = "Shuffle only"
   filenames(2) = "f_compress_shuffle.nc"
   shuffle_flags(2) = 1
   deflate_flags(2) = 0
   deflate_levels(2) = 0
   
   test_names(3) = "Deflate level 1"
   filenames(3) = "f_compress_deflate1.nc"
   shuffle_flags(3) = 0
   deflate_flags(3) = 1
   deflate_levels(3) = 1
   
   test_names(4) = "Deflate level 5"
   filenames(4) = "f_compress_deflate5.nc"
   shuffle_flags(4) = 0
   deflate_flags(4) = 1
   deflate_levels(4) = 5
   
   test_names(5) = "Deflate level 9"
   filenames(5) = "f_compress_deflate9.nc"
   shuffle_flags(5) = 0
   deflate_flags(5) = 1
   deflate_levels(5) = 9
   
   test_names(6) = "Shuffle + Deflate 5 (recommended)"
   filenames(6) = "f_compress_shuffle_deflate5.nc"
   shuffle_flags(6) = 1
   deflate_flags(6) = 1
   deflate_levels(6) = 5
   
   ! Run all tests
   do i = 1, NUM_TESTS
      call create_compressed_file(trim(test_names(i)), trim(filenames(i)), &
                                  shuffle_flags(i), deflate_flags(i), deflate_levels(i), &
                                  data, write_times(i), file_sizes(i))
      call read_compressed_file(trim(filenames(i)), shuffle_flags(i), deflate_flags(i), &
                               deflate_levels(i), data, read_times(i))
   end do
   
   ! Calculate compression ratios
   do i = 1, NUM_TESTS
      compression_ratios(i) = real(file_sizes(1), 8) / real(file_sizes(i), 8)
   end do
   
   ! Print summary table
   print *, ""
   print *, "=== Performance Summary ==="
   print '(A35, A12, A12, A12, A10)', "Strategy", "Write (s)", "Read (s)", "Size (MB)", "Ratio"
   print '(A35, A12, A12, A12, A10)', "--------", "---------", "--------", "---------", "-----"
   
   do i = 1, NUM_TESTS
      print '(A35, F12.3, F12.3, F12.2, F9.2, A1)', &
         trim(test_names(i)), write_times(i), read_times(i), &
         file_sizes(i) / 1048576.0, compression_ratios(i), "x"
   end do
   
   ! Print recommendations
   print *, ""
   print *, "=== Recommendations ==="
   print *, "- Uncompressed: Fastest I/O but largest files"
   print *, "- Shuffle only: Reorganizes bytes for better compression (use with deflate)"
   print *, "- Deflate level 1: Fast compression, moderate space savings"
   print *, "- Deflate level 5: Good balance of speed and compression"
   print *, "- Deflate level 9: Maximum compression, slower writes"
   print *, "- Shuffle + Deflate: RECOMMENDED for scientific data"
   print *, "- Higher deflate levels increase write time but improve compression"
   print *, "- Read performance generally similar across compression levels"
   print *, "- Compression effectiveness depends on data patterns"
   
   deallocate(data)
   print *, ""
   print *, "*** SUCCESS: All compression strategies tested!"
   
contains

   subroutine create_compressed_file(test_name, filename, shuffle, deflate, deflate_level, &
                                     data, write_time, file_size)
      character(len=*), intent(in) :: test_name, filename
      integer, intent(in) :: shuffle, deflate, deflate_level
      real, intent(in) :: data(:,:,:)
      real(8), intent(out) :: write_time
      integer, intent(out) :: file_size
      
      integer :: ncid, varid
      integer :: time_dimid, lat_dimid, lon_dimid
      integer :: dimids(NDIMS)
      integer :: retval
      real(8) :: start_time, end_time
      logical :: file_exists
      
      print *, ""
      print *, "=== ", trim(test_name), " ==="
      
      call cpu_time(start_time)
      
      retval = nf90_create(filename, NF90_CLOBBER + NF90_NETCDF4, ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      retval = nf90_def_dim(ncid, "time", NTIME, time_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_def_dim(ncid, "lat", NLAT, lat_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_def_dim(ncid, "lon", NLON, lon_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      dimids(1) = lon_dimid
      dimids(2) = lat_dimid
      dimids(3) = time_dimid
      retval = nf90_def_var(ncid, "temperature", NF90_FLOAT, dimids, varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      if (deflate == 1 .or. shuffle == 1) then
         retval = nf90_def_var_deflate(ncid, varid, shuffle, deflate, deflate_level)
         if (retval /= nf90_noerr) call handle_err(retval)
      end if
      
      retval = nf90_enddef(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      retval = nf90_put_var(ncid, varid, data)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      retval = nf90_close(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      call cpu_time(end_time)
      write_time = end_time - start_time
      
      inquire(file=filename, exist=file_exists, size=file_size)
      if (file_exists) then
         print *, "File size: ", file_size, " bytes (", file_size / 1048576.0, " MB)"
      end if
      
      print *, "Write time: ", write_time, " seconds"
      
      if (shuffle == 1) print *, "Shuffle: enabled"
      if (deflate == 1) print *, "Deflate: level ", deflate_level
      
   end subroutine create_compressed_file
   
   subroutine read_compressed_file(filename, expected_shuffle, expected_deflate, &
                                   expected_level, original_data, read_time)
      character(len=*), intent(in) :: filename
      integer, intent(in) :: expected_shuffle, expected_deflate, expected_level
      real, intent(in) :: original_data(:,:,:)
      real(8), intent(out) :: read_time
      
      integer :: ncid, varid
      integer :: retval
      real(8) :: start_time, end_time
      real, allocatable :: data(:,:,:)
      integer :: shuffle, deflate, deflate_level
      integer :: errors, i
      
      allocate(data(NLON, NLAT, NTIME))
      
      call cpu_time(start_time)
      
      retval = nf90_open(filename, NF90_NOWRITE, ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      retval = nf90_inq_varid(ncid, "temperature", varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      retval = nf90_inq_var_deflate(ncid, varid, shuffle, deflate, deflate_level)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      if (shuffle /= expected_shuffle .or. deflate /= expected_deflate .or. &
          (deflate == 1 .and. deflate_level /= expected_level)) then
         print *, "Error: Compression settings mismatch"
         stop 2
      end if
      
      retval = nf90_get_var(ncid, varid, data)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      retval = nf90_close(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      call cpu_time(end_time)
      read_time = end_time - start_time
      
      errors = 0
      do i = 1, min(100, NLON * NLAT * NTIME)
         if (abs(data(i,1,1) - original_data(i,1,1)) > 0.001) then
            errors = errors + 1
         end if
      end do
      
      if (errors > 0) then
         print *, "*** FAILED: ", errors, " validation errors"
         stop 2
      end if
      
      print *, "Read time: ", read_time, " seconds"
      print *, "Data validated successfully"
      
      deallocate(data)
      
   end subroutine read_compressed_file
   
   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop 2
   end subroutine handle_err
   
end program f_compression
