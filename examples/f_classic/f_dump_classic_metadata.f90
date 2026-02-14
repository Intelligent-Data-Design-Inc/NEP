!> @file f_dump_classic_metadata.f90
!! @brief Read a NetCDF file and print all metadata (Fortran)
!!
!! This example demonstrates how to use the NetCDF Fortran inquiry functions
!! to discover and print all metadata in a NetCDF file without prior knowledge
!! of its contents. It reads a filename from the command line, opens the file,
!! and prints:
!! - All dimensions (name and length, noting unlimited dimensions)
!! - All global attributes (name, type, and value)
!! - All variables (name, type, dimensions, and attributes)
!!
!! This is a useful pattern for building tools that inspect arbitrary NetCDF files.
!!
!! **Learning Objectives:**
!! - Use nf90_inquire() to discover file structure
!! - Iterate over dimensions with nf90_inquire_dimension()
!! - Iterate over variables with nf90_inquire_variable()
!! - Iterate over attributes with nf90_inq_attname() and nf90_inquire_attribute()
!! - Read attribute values of different types
!! - Handle unlimited dimensions
!! - Accept command-line arguments in Fortran
!!
!! **Prerequisites:** Basic Fortran programming, familiarity with NetCDF concepts
!!
!! **Compilation:**
!! @code
!! gfortran -o f_dump_classic_metadata f_dump_classic_metadata.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_dump_classic_metadata f_coord_vars.nc
!! @endcode
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026

program f_dump_classic_metadata
   use netcdf
   implicit none

   character(len=256) :: filename
   integer :: ncid, retval
   integer :: ndims, nvars, ngatts, unlimdimid
   integer :: d, v, a
   character(len=NF90_MAX_NAME) :: dim_name, var_name, att_name
   integer :: dim_len
   integer :: var_type, var_ndims, var_natts
   integer :: var_dimids(NF90_MAX_VAR_DIMS)
   integer :: att_type, att_len
   integer :: i

   ! Get filename from command line.
   if (command_argument_count() /= 1) then
      write(*, '(A)') "Usage: f_dump_classic_metadata <netcdf_file>"
      stop 1
   end if
   call get_command_argument(1, filename)

   ! Open the file.
   retval = nf90_open(trim(filename), NF90_NOWRITE, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Get top-level counts.
   retval = nf90_inquire(ncid, ndims, nvars, ngatts, unlimdimid)
   if (retval /= nf90_noerr) call handle_err(retval)

   write(*, '(A,A)') "File: ", trim(filename)
   write(*, '(A,I0)') "Number of dimensions: ", ndims
   write(*, '(A,I0)') "Number of variables: ", nvars
   write(*, '(A,I0)') "Number of global attributes: ", ngatts
   if (unlimdimid >= 0) then
      write(*, '(A,I0)') "Unlimited dimension id: ", unlimdimid
   else
      write(*, '(A)') "No unlimited dimension"
   end if

   ! Print dimensions.
   write(*, '(A)') ""
   write(*, '(A)') "Dimensions:"
   do d = 1, ndims
      retval = nf90_inquire_dimension(ncid, d, dim_name, dim_len)
      if (retval /= nf90_noerr) call handle_err(retval)
      if (d == unlimdimid) then
         write(*, '(A,A,A,I0,A)') "  ", trim(dim_name), " = ", dim_len, " (unlimited)"
      else
         write(*, '(A,A,A,I0)') "  ", trim(dim_name), " = ", dim_len
      end if
   end do

   ! Print global attributes.
   if (ngatts > 0) then
      write(*, '(A)') ""
      write(*, '(A)') "Global Attributes:"
      do a = 1, ngatts
         retval = nf90_inq_attname(ncid, NF90_GLOBAL, a, att_name)
         if (retval /= nf90_noerr) call handle_err(retval)
         call print_attribute(ncid, NF90_GLOBAL, att_name, "  ")
      end do
   end if

   ! Print variables.
   write(*, '(A)') ""
   write(*, '(A)') "Variables:"
   do v = 1, nvars
      retval = nf90_inquire_variable(ncid, v, var_name, var_type, var_ndims, &
                                      var_dimids, var_natts)
      if (retval /= nf90_noerr) call handle_err(retval)

      ! Print variable header with type and dimension count.
      write(*, '(A,A,A,A,A,I0,A)', advance='no') "  ", trim(var_name), &
           ": type ", trim(type_name(var_type)), ", ", var_ndims, " dimension(s)"

      ! Print dimension names for this variable.
      if (var_ndims > 0) then
         write(*, '(A)', advance='no') " ("
         do d = 1, var_ndims
            retval = nf90_inquire_dimension(ncid, var_dimids(d), dim_name)
            if (retval /= nf90_noerr) call handle_err(retval)
            if (d > 1) write(*, '(A)', advance='no') ", "
            write(*, '(A)', advance='no') trim(dim_name)
         end do
         write(*, '(A)', advance='no') ")"
      end if
      write(*, '(A,I0,A)') ", ", var_natts, " attribute(s)"

      ! Print variable attributes.
      do a = 1, var_natts
         retval = nf90_inq_attname(ncid, v, a, att_name)
         if (retval /= nf90_noerr) call handle_err(retval)
         call print_attribute(ncid, v, att_name, "    ")
      end do
   end do

   ! Close the file.
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

contains

   ! Return a human-readable string for a NetCDF type.
   function type_name(xtype) result(name)
      integer, intent(in) :: xtype
      character(len=10) :: name
      select case (xtype)
      case (NF90_BYTE)
         name = "byte"
      case (NF90_CHAR)
         name = "char"
      case (NF90_SHORT)
         name = "short"
      case (NF90_INT)
         name = "int"
      case (NF90_FLOAT)
         name = "float"
      case (NF90_DOUBLE)
         name = "double"
      case default
         name = "unknown"
      end select
   end function type_name

   ! Print a single attribute (name, type, length, value).
   subroutine print_attribute(ncid, varid, att_name, indent)
      integer, intent(in) :: ncid, varid
      character(len=*), intent(in) :: att_name, indent
      integer :: att_type, att_len, retval
      character(len=1024) :: text_val
      integer(kind=1), allocatable :: byte_vals(:)
      integer(kind=2), allocatable :: short_vals(:)
      integer, allocatable :: int_vals(:)
      real, allocatable :: float_vals(:)
      double precision, allocatable :: double_vals(:)
      integer :: i

      retval = nf90_inquire_attribute(ncid, varid, att_name, att_type, att_len)
      if (retval /= nf90_noerr) call handle_err(retval)

      write(*, '(A,A,A,A,A,I0,A)', advance='no') indent, trim(att_name), &
           ": type ", trim(type_name(att_type)), ", length ", att_len, ", value: "

      select case (att_type)
      case (NF90_CHAR)
         text_val = ' '
         retval = nf90_get_att(ncid, varid, att_name, text_val)
         if (retval /= nf90_noerr) call handle_err(retval)
         write(*, '(A,A,A)') '"', text_val(1:att_len), '"'

      case (NF90_BYTE)
         allocate(byte_vals(att_len))
         retval = nf90_get_att(ncid, varid, att_name, byte_vals)
         if (retval /= nf90_noerr) call handle_err(retval)
         do i = 1, att_len
            if (i > 1) write(*, '(A)', advance='no') ", "
            write(*, '(I0)', advance='no') byte_vals(i)
         end do
         write(*, '(A)') ""
         deallocate(byte_vals)

      case (NF90_SHORT)
         allocate(short_vals(att_len))
         retval = nf90_get_att(ncid, varid, att_name, short_vals)
         if (retval /= nf90_noerr) call handle_err(retval)
         do i = 1, att_len
            if (i > 1) write(*, '(A)', advance='no') ", "
            write(*, '(I0)', advance='no') short_vals(i)
         end do
         write(*, '(A)') ""
         deallocate(short_vals)

      case (NF90_INT)
         allocate(int_vals(att_len))
         retval = nf90_get_att(ncid, varid, att_name, int_vals)
         if (retval /= nf90_noerr) call handle_err(retval)
         do i = 1, att_len
            if (i > 1) write(*, '(A)', advance='no') ", "
            write(*, '(I0)', advance='no') int_vals(i)
         end do
         write(*, '(A)') ""
         deallocate(int_vals)

      case (NF90_FLOAT)
         allocate(float_vals(att_len))
         retval = nf90_get_att(ncid, varid, att_name, float_vals)
         if (retval /= nf90_noerr) call handle_err(retval)
         do i = 1, att_len
            if (i > 1) write(*, '(A)', advance='no') ", "
            write(*, '(G0)', advance='no') float_vals(i)
         end do
         write(*, '(A)') ""
         deallocate(float_vals)

      case (NF90_DOUBLE)
         allocate(double_vals(att_len))
         retval = nf90_get_att(ncid, varid, att_name, double_vals)
         if (retval /= nf90_noerr) call handle_err(retval)
         do i = 1, att_len
            if (i > 1) write(*, '(A)', advance='no') ", "
            write(*, '(G0)', advance='no') double_vals(i)
         end do
         write(*, '(A)') ""
         deallocate(double_vals)

      case default
         write(*, '(A)') "(unsupported type)"
      end select
   end subroutine print_attribute

   subroutine handle_err(status)
      integer, intent(in) :: status
      write(*, '(A,A)') "Error: ", trim(nf90_strerror(status))
      stop 2
   end subroutine handle_err

end program f_dump_classic_metadata
