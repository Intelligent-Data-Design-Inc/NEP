!> @file f_compression.f90
!! @brief NetCDF-4 compression filters with
!! performance analysis (Fortran)
!!
!! Fortran equivalent of compression.c, exploring NetCDF-4 compression using the
!! Fortran 90 NetCDF API. Tests various
!! compression configurations and measures
!! performance.
!!
!! **Learning Objectives:**
!! - Configure compression with nf90_def_var_deflate() and nf90_def_var_zstandard() in Fortran
!! - Measure compression performance in Fortran applications
!! - Select appropriate compression levels for different data
!! - Set fill values with nf90_def_var_fill() for chunked/compressed variables
!! - Verify that unwritten chunks return the fill value on read
!!
!! **Fortran Compression Functions:**
!! - nf90_def_var_deflate(ncid, varid, shuffle, deflate, deflate_level)
!!   - shuffle: 0=off, 1=on; deflate: 0=off, 1=on; deflate_level: 1-9
!! - nf90_def_var_zstandard(ncid, varid, level)
!!   - level: 1-22 (higher=better compression, slower)
!!   - Zstandard is generally faster than zlib and provides better compression ratios.
!!     Prefer Zstandard when it is available; level 3 is the best tradeoff.
!! - zlib is the fallback when Zstandard is unavailable. For almost all real-world
!!   data, zlib level 1 is the preferred deflate value. Higher levels yield
!!   diminishing returns at much greater CPU cost.
!!
!! **Fortran Fill Value Functions:**
!! - nf90_def_var_fill(ncid, varid, no_fill,
!!   fill_value): set fill value (no_fill=0)
!! - nf90_inq_var_fill(ncid, varid, no_fill, fill_value): query fill value
!! - In chunked/compressed variables, unwritten chunks return the fill value
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
   use iso_fortran_env, only: int64
   implicit none
   
   integer, parameter :: NTIME = 50
   integer, parameter :: NLAT = 90
   integer, parameter :: NLON = 180
   integer, parameter :: NDIMS = 3
   integer, parameter :: NUM_TESTS = 11
   real, parameter :: FILL_VALUE = -9999.0
   
   real, allocatable :: data(:,:,:)
   real(8) :: write_times(NUM_TESTS), read_times(NUM_TESTS)
   integer(int64) :: file_sizes(NUM_TESTS)
   real(8) :: compression_ratios(NUM_TESTS)
   character(len=64) :: test_names(NUM_TESTS)
   character(len=128) :: filenames(NUM_TESTS)
   integer :: shuffle_flags(NUM_TESTS)
   integer :: deflate_flags(NUM_TESTS)
   integer :: deflate_levels(NUM_TESTS)
   integer :: zstd_levels(NUM_TESTS)
   integer :: i, t, lat, lon
   real :: base_temp, seasonal, spatial
   real, parameter :: PI = 3.14159265359
   logical :: zstd_available
   
   print '(A, I0, A, I0, A, I0, A, F0.2, A)', "Compression: ", &
        NTIME, "x", NLAT, "x", NLON, " floats (", &
        (NTIME * NLAT * NLON * 4.0) / 1048576.0, " MB)"
   
   ! Allocate and generate realistic temperature data
   allocate(data(NLON, NLAT, NTIME))
   
   do t = 1, NTIME
      do lat = 1, NLAT
         do lon = 1, NLON
            base_temp = 15.0 - (lat - NLAT/2) * 0.5
            seasonal = 10.0 * sin(2.0 * PI * t / NTIME)
            spatial = 5.0 &
                 * sin(2.0 * PI * lon / NLON) &
                 * cos(2.0 * PI * lat / NLAT)
            data(lon, lat, t) = base_temp + seasonal + spatial
         end do
      end do
   end do
   
   ! Define compression tests
   test_names = [character(len=64) :: &
        "Uncompressed (baseline)", &
        "Shuffle only", &
        "Deflate level 1 (preferred)", &
        "Deflate level 5", &
        "Deflate level 9", &
        "Shuffle + Deflate 1 (recommended)", &
        "Zstandard level 1", &
        "Zstandard level 3", &
        "Zstandard level 9", &
        "Shuffle + Zstandard 3", &
        "Shuffle + Zstandard 9"]

   filenames = [character(len=128) :: &
        "f_compress_none.nc", &
        "f_compress_shuffle.nc", &
        "f_compress_deflate1.nc", &
        "f_compress_deflate5.nc", &
        "f_compress_deflate9.nc", &
        "f_compress_shuffle_deflate1.nc", &
        "f_compress_zstd1.nc", &
        "f_compress_zstd3.nc", &
        "f_compress_zstd9.nc", &
        "f_compress_shuffle_zstd3.nc", &
        "f_compress_shuffle_zstd9.nc"]

   shuffle_flags = [0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1]
   deflate_flags = [0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0]
   deflate_levels = [0, 0, 1, 5, 9, 1, 0, 0, 0, 0, 0]
   zstd_levels = [-1, -1, -1, -1, -1, -1, 1, 3, 9, 3, 9]
   
   ! Probe for runtime Zstandard support
   zstd_available = check_zstd_support()
   if (.not. zstd_available) print '(A)', "Zstandard not available; skipping zstd tests."

   ! Run all tests
   do i = 1, NUM_TESTS
      if (zstd_levels(i) >= 0 .and. .not. zstd_available) cycle
      call create_compressed_file( &
           trim(filenames(i)), &
           shuffle_flags(i), &
           deflate_flags(i), &
           deflate_levels(i), &
           zstd_levels(i), &
           data, write_times(i), file_sizes(i))
      call read_compressed_file( &
           trim(filenames(i)), &
           shuffle_flags(i), &
           deflate_flags(i), &
           deflate_levels(i), &
           zstd_levels(i), data, &
           read_times(i))
   end do
   
   ! Calculate compression ratios
   do i = 1, NUM_TESTS
      compression_ratios(i) = real(file_sizes(1), 8) / real(file_sizes(i), 8)
   end do
   
   ! Find best zlib and best zstd by compression ratio
   block
     integer :: best_zlib, best_zstd
     best_zlib = -1
     best_zstd = -1
     do i = 1, NUM_TESTS
        if (zstd_levels(i) >= 0 .and. .not. zstd_available) cycle
        if (deflate_flags(i) == 1) then
           if (best_zlib == -1 .or. compression_ratios(i) > compression_ratios(best_zlib)) then
              best_zlib = i
           end if
        end if
        if (zstd_levels(i) >= 0) then
           if (best_zstd == -1 .or. compression_ratios(i) > compression_ratios(best_zstd)) then
              best_zstd = i
           end if
        end if
     end do

     print '(A)', ""
     print '(A)', "Results: write/read (s), size (MB), ratio"
     do i = 1, NUM_TESTS
        if (zstd_levels(i) >= 0 .and. .not. zstd_available) cycle
        print '(A, F0.3, A, F0.3, A, F0.2, A, F0.2, A)', &
             trim(test_names(i)) // ": ", write_times(i), "/", read_times(i), &
             ", ", real(file_sizes(i), 8) / 1048576.0_8, " MB, ", &
             compression_ratios(i), "x"
     end do

     if (best_zlib > 0 .and. best_zstd > 0) then
        print '(A)', ""
        print '(A, A, F0.2, A, A, F0.2, A)', "Best zlib: ", &
             trim(test_names(best_zlib)) // " (", compression_ratios(best_zlib), &
             "x); best zstd: ", trim(test_names(best_zstd)) // " (", &
             compression_ratios(best_zstd), "x)"
     end if
   end block

   print '(A)', ""
   print '(A)', "Use shuffle+deflate 1 for compatibility; zstd level 3 for best ratio."

   deallocate(data)
   
contains

   function check_zstd_support() result(zstd_available)
      logical :: zstd_available
      integer :: ncid, varid, dimid, retval, zstd_ret

      zstd_available = .false.
      retval = nf90_create("zstd_probe.nc", NF90_CLOBBER + NF90_NETCDF4, ncid)
      if (retval /= nf90_noerr) return

      retval = nf90_def_dim(ncid, "x", 1, dimid)
      if (retval == nf90_noerr) retval = nf90_def_var(ncid, "v", NF90_FLOAT, (/dimid/), varid)
      zstd_ret = nf90_def_var_zstandard(ncid, varid, 3)

      retval = nf90_close(ncid)
      call system("rm -f zstd_probe.nc")
      zstd_available = (retval == nf90_noerr .and. zstd_ret == nf90_noerr)
   end function check_zstd_support

   subroutine create_compressed_file( &
        filename, &
        shuffle, deflate, deflate_level, &
        zstd_level, data, write_time, file_size)
      character(len=*), intent(in) :: filename
      integer, intent(in) :: shuffle, deflate, deflate_level, zstd_level
      real, intent(in) :: data(:,:,:)
      real(8), intent(out) :: write_time
      integer(int64), intent(out) :: file_size
      
      integer :: ncid, varid
      integer :: time_dimid, lat_dimid, lon_dimid
      integer :: dimids(NDIMS)
      integer :: retval
      integer(int64) :: start_count, end_count, count_rate
      integer :: start_idx(NDIMS), count_idx(NDIMS)
      integer :: chunksizes(NDIMS)

      call system_clock(start_count, count_rate)
      
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

      ! Set consistent chunking for all compression tests
      chunksizes = (/ 90, 45, 10 /)
      retval = nf90_def_var_chunking(ncid, varid, NF90_CHUNKED, chunksizes)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      if (zstd_level >= 0) then
         if (shuffle == 1) then
            retval = nf90_def_var_deflate(ncid, varid, 1, 0, 0)
            if (retval /= nf90_noerr) call handle_err(retval)
         end if
         retval = nf90_def_var_zstandard(ncid, varid, zstd_level)
         if (retval /= nf90_noerr) call handle_err(retval)
      else if (deflate == 1 .or. shuffle == 1) then
         retval = nf90_def_var_deflate(ncid, &
              varid, shuffle, deflate, &
              deflate_level)
         if (retval /= nf90_noerr) call handle_err(retval)
      end if
      
      ! Set fill value: nf90_def_var_fill()
      ! registers the sentinel returned for
      ! unwritten elements. In chunked/compressed
      ! variables, unwritten chunks are stored as
      ! fill values; in classic format it is stored as a _FillValue attribute.
      retval = nf90_def_var_fill(ncid, varid, 0, FILL_VALUE)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      retval = nf90_enddef(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Write all time steps except the last one (partial write).
      ! The unwritten last time step will return FILL_VALUE when read back,
      ! demonstrating fill value behavior with chunked/compressed storage.
      ! Fortran dim order: (lon, lat, time); omit last time index.
      start_idx = (/ 1, 1, 1 /)
      count_idx = (/ NLON, NLAT, NTIME - 1 /)
      retval = nf90_put_var(ncid, varid, &
           data(:,:,1:NTIME-1), &
           start=start_idx, count=count_idx)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      retval = nf90_close(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      call system_clock(end_count)
      write_time = real(end_count - start_count, 8) / real(count_rate, 8)
      
      inquire(file=filename, size=file_size)
      
   end subroutine create_compressed_file
   
   subroutine read_compressed_file(filename, &
        expected_shuffle, expected_deflate, &
        expected_level, expected_zstd, original_data, read_time)
      character(len=*), intent(in) :: filename
      integer, intent(in) :: expected_shuffle, expected_deflate, expected_level, expected_zstd
      real, intent(in) :: original_data(:,:,:)
      real(8), intent(out) :: read_time
      
      integer :: ncid, varid
      integer :: retval
      integer(int64) :: start_count, end_count, count_rate
      real, allocatable :: data(:,:,:)
      integer :: shuffle, deflate, deflate_level
      integer :: zstd, zstd_level
      integer :: errors, i
      integer :: no_fill
      real :: fill_value_in
      
      allocate(data(NLON, NLAT, NTIME))
      
      call system_clock(start_count, count_rate)
      
      retval = nf90_open(filename, NF90_NOWRITE, ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      retval = nf90_inq_varid(ncid, "temperature", varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      if (expected_zstd >= 0) then
         retval = nf90_inq_var_zstandard(ncid, varid, zstd, zstd_level)
         if (retval /= nf90_noerr) call handle_err(retval)
         if (zstd /= 1 .or. zstd_level /= expected_zstd) then
            print *, "Error: Zstandard settings mismatch"
            stop 2
         end if
      else
         retval = nf90_inq_var_deflate(ncid, &
              varid, shuffle, deflate, &
              deflate_level)
         if (retval /= nf90_noerr) call handle_err(retval)
         if (shuffle /= expected_shuffle .or. deflate /= expected_deflate .or. &
             (deflate == 1 .and. deflate_level /= expected_level)) then
            print *, "Error: Compression settings mismatch"
            stop 2
         end if
      end if
      
      ! Verify fill value using nf90_inq_var_fill()
      retval = nf90_inq_var_fill(ncid, varid, no_fill, fill_value_in)
      if (retval /= nf90_noerr) call handle_err(retval)
      if (abs(fill_value_in - FILL_VALUE) > 1.0e-6) then
         print *, "Error: fill value = ", &
              fill_value_in, &
              ", expected ", FILL_VALUE
         stop 2
      end if
      
      retval = nf90_get_var(ncid, varid, data)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      retval = nf90_close(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      call system_clock(end_count)
      read_time = real(end_count - start_count, 8) / real(count_rate, 8)
      
      ! Validate written data (first NTIME-1
      ! time steps, check first 100 lon points)
      errors = 0
      do i = 1, min(100, NLON)
         if (abs(data(i,1,1) - original_data(i,1,1)) > 0.001) then
            errors = errors + 1
         end if
      end do
      
      ! Verify unwritten last time step returns fill value
      do i = 1, min(10, NLON)
         if (data(i, 1, NTIME) /= FILL_VALUE) then
            print *, "Error: unwritten (", &
                 i, ",1,", NTIME, &
                 ") = ", data(i,1,NTIME), &
                 ", expected ", FILL_VALUE
            errors = errors + 1
         end if
      end do
      
      if (errors > 0) then
         print *, "*** FAILED: ", errors, " validation errors"
         stop 2
      end if
      
      deallocate(data)
      
   end subroutine read_compressed_file
   
   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop 2
   end subroutine handle_err
   
end program f_compression
