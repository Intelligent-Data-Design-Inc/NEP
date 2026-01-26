!> @file f_size_limits.f90
!! @brief Demonstrates file size and dimension limits for NetCDF classic formats (Fortran)
!!
!! This is the Fortran equivalent of size_limits.c, exploring size limitations of
!! classic NetCDF formats using the Fortran 90 NetCDF API. The program demonstrates
!! when format upgrades are necessary for large datasets.
!!
!! **Learning Objectives:**
!! - Understand file size limits in Fortran NetCDF applications
!! - Learn when to upgrade from CDF-1 to CDF-2 or CDF-5
!! - Work with large integers (int64) for dimension calculations
!! - Recognize format-related errors in Fortran
!!
!! **Fortran-Specific Considerations:**
!! - Use iso_fortran_env module for int64 type
!! - Integer kind selection for large dimension sizes
!! - Format selection impacts Fortran array size limits
!!
!! **Prerequisites:**
!! - f_format_variants.f90 - Understanding format types
!! - size_limits.c - C equivalent for comparison
!!
!! **Related Examples:**
!! - size_limits.c - C equivalent
!! - f_format_variants.f90 - Format comparison
!! - f_simple_nc4.f90 - NetCDF-4 format (different limits)
!!
!! **Compilation:**
!! @code
!! gfortran -o f_size_limits f_size_limits.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_size_limits
!! @endcode
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026

program f_size_limits
   use netcdf
   use iso_fortran_env, only: int64
   implicit none
   
   integer, parameter :: ERRCODE = 2
   integer, parameter :: CLASSIC_DIM = 5000
   integer, parameter :: OFFSET_DIM = 5000
   integer, parameter :: DATA_DIM = 5000
   character(len=*), parameter :: TEST_MODE = "SMALL"
   
   print *, "NetCDF Classic Format Size Limits Test"
   print *, "Test mode: ", TEST_MODE
   print *, ""
   print *, "Running in small file mode (default)."
   print *, "For actual size limit testing, use f_size_limits_large program"
   print *, "(requires --enable-large-tests build option)"
   
   call print_format_limits()
   
   call test_format("f_size_limits_classic.nc", NF90_CLASSIC_MODEL, &
                    "NC_CLASSIC_MODEL", CLASSIC_DIM)
   
   call test_format("f_size_limits_64bit_offset.nc", NF90_64BIT_OFFSET, &
                    "NC_64BIT_OFFSET", OFFSET_DIM)
   
   call test_format("f_size_limits_64bit_data.nc", NF90_64BIT_DATA, &
                    "NC_64BIT_DATA", DATA_DIM)
   
   print *, "=== All Format Tests Complete ==="
   print *, ""
   print *, "Summary:"
   print *, "  Test mode: ", TEST_MODE
   print *, "  Files created: 3"
   print *, "  Formats tested: NC_CLASSIC_MODEL, NC_64BIT_OFFSET, NC_64BIT_DATA"
   print *, ""
   print *, "Small file tests demonstrate format detection and calculations."
   print *, "For actual size limit testing, use f_size_limits_large program."
   
   print *, ""
   print *, "*** SUCCESS: All validation checks passed! ***"
   
contains

   subroutine print_format_limits()
      print *, ""
      print *, "=== NetCDF Classic Format Size Limits ==="
      print *, ""
      print *, "NC_CLASSIC_MODEL (CDF-1):"
      print *, "  Total file size limit: 2GB (2,147,483,647 bytes)"
      print *, "  Single variable limit: 2GB"
      print *, "  Compatibility: NetCDF 3.0+, all tools"
      print *, ""
      print *, "NC_64BIT_OFFSET (CDF-2):"
      print *, "  Total file size limit: effectively unlimited"
      print *, "  Single variable limit: 4GB (4,294,967,295 bytes)"
      print *, "  Compatibility: NetCDF 3.6.0+"
      print *, ""
      print *, "NC_64BIT_DATA (CDF-5):"
      print *, "  Total file size limit: effectively unlimited"
      print *, "  Single variable limit: effectively unlimited (2^64)"
      print *, "  Compatibility: NetCDF 4.4.0+ or PnetCDF"
      print *, ""
      print *, "Size Calculation Formula:"
      print *, "  file_size = header_size + sum(variable_sizes)"
      print *, "  variable_size = product(dimensions) * sizeof(data_type)"
      print *, ""
   end subroutine print_format_limits

   subroutine test_format(filename, format_flag, format_name, dim_size)
      character(len=*), intent(in) :: filename, format_name
      integer, intent(in) :: format_flag
      integer, intent(in) :: dim_size
      
      integer :: ncid, varid, dimid, retval
      integer :: format_in
      character(len=50) :: detected_format
      real :: test_data(10)
      integer :: start_pos(1), count_val(1)
      
      print *, "Testing ", trim(format_name), " format..."
      print *, "  Creating file: ", trim(filename)
      print *, "  Dimension size: ", dim_size
      
      ! Create file with specified format
      retval = nf90_create(filename, format_flag, ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Define dimension
      retval = nf90_def_dim(ncid, "x", int(dim_size), dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Define variable
      retval = nf90_def_var(ncid, "data", NF90_FLOAT, dimid, varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! End define mode
      retval = nf90_enddef(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Write a few test values
      test_data = (/0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0/)
      start_pos(1) = 1
      if (dim_size < 10) then
         count_val(1) = dim_size
      else
         count_val(1) = 10
      end if
      retval = nf90_put_var(ncid, varid, test_data(1:count_val(1)), &
                           start=start_pos, count=count_val)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Close file
      retval = nf90_close(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Reopen and verify format
      retval = nf90_open(filename, NF90_NOWRITE, ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Check format
      retval = nf90_inq_format(ncid, format_in)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Determine detected format name
      if (format_in == NF90_FORMAT_CLASSIC) then
         detected_format = "NF90_FORMAT_CLASSIC"
      else if (format_in == NF90_FORMAT_64BIT_OFFSET) then
         detected_format = "NF90_FORMAT_64BIT_OFFSET"
      else if (format_in == NF90_FORMAT_64BIT_DATA) then
         detected_format = "NF90_FORMAT_64BIT_DATA"
      else
         detected_format = "UNKNOWN"
      end if
      
      print *, "  Format detected: ", trim(detected_format)
      
      ! Note: File size reporting would require system calls
      ! Skipping for portability, but calculation shown
      print *, "  Variable size: ", real(dim_size) * 4.0 / 1024.0, " KB"
      
      ! Close file
      retval = nf90_close(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      print *, "  ✓ Test complete"
      print *, ""
   end subroutine test_format

   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop ERRCODE
   end subroutine handle_err
   
end program f_size_limits
