!> @file f_dump_nc4_metadata.f90
!! @brief Read a NetCDF-4 file and print all metadata including user-defined types
!!
!! Fortran equivalent of dump_nc4_metadata.c. Reads a filename from the command
!! line, opens the file, and prints all metadata:
!! - User-defined types (enum, opaque)
!! - All dimensions (name and length, noting unlimited dimensions)
!! - All global attributes (name, type, and value)
!! - All variables (name, type, dimensions, and attributes)
!! - Groups (recursively)
!!
!! Note: Fortran does not handle VLEN, String, or Compound types portably,
!! so those type classes are reported by name only without detail.
!!
!! **Learning Objectives:**
!! - Use nf90_inq_typeids() to discover user-defined types
!! - Use nf90_inq_user_type() to determine type class
!! - Use nf90_inq_enum(), nf90_inq_enum_member(), nf90_inq_opaque()
!! - Use nf90_inq_grps() to discover groups
!! - Recursively traverse the group hierarchy
!!
!! **Prerequisites:** Basic Fortran programming, familiarity with NetCDF-4
!!
!! **Compilation:**
!! @code
!! gfortran -o f_dump_nc4_metadata f_dump_nc4_metadata.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_dump_nc4_metadata f_user_types.nc
!! @endcode
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026

program f_dump_nc4_metadata
   use netcdf
   implicit none

   character(len=256) :: filename
   integer :: ncid, retval

   ! Get filename from command line.
   if (command_argument_count() /= 1) then
      write(*, '(A)') "Usage: f_dump_nc4_metadata <netcdf_file>"
      stop 1
   end if
   call get_command_argument(1, filename)

   ! Open the file.
   retval = nf90_open(trim(filename), NF90_NOWRITE, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   write(*, '(A,A)') "File: ", trim(filename)

   ! Print the root group.
   call print_group(ncid, "/", "")

   ! Close the file.
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

contains

   ! Return a human-readable string for a NetCDF type.
   ! For user-defined types, look up the name.
   function type_name(ncid, xtype) result(tname)
      integer, intent(in) :: ncid, xtype
      character(len=NF90_MAX_NAME) :: tname
      integer :: ret, dummy_size, dummy_base, dummy_nfields, dummy_class

      select case (xtype)
      case (NF90_BYTE)
         tname = "byte"
      case (NF90_CHAR)
         tname = "char"
      case (NF90_SHORT)
         tname = "short"
      case (NF90_INT)
         tname = "int"
      case (NF90_FLOAT)
         tname = "float"
      case (NF90_DOUBLE)
         tname = "double"
      case (NF90_UBYTE)
         tname = "ubyte"
      case (NF90_USHORT)
         tname = "ushort"
      case (NF90_UINT)
         tname = "uint"
      case (NF90_INT64)
         tname = "int64"
      case (NF90_UINT64)
         tname = "uint64"
      case (NF90_STRING)
         tname = "string"
      case default
         ! User-defined type - use nf90_inq_user_type (not nf90_inq_type,
         ! which has a size_t/integer mismatch on 64-bit systems).
         ret = nf90_inq_user_type(ncid, xtype, tname, dummy_size, &
              dummy_base, dummy_nfields, dummy_class)
         if (ret /= nf90_noerr) tname = "unknown"
      end select
   end function type_name

   ! Print the value(s) of an attribute.
   subroutine print_att_value(ncid, varid, att_name, att_type, att_len)
      integer, intent(in) :: ncid, varid, att_type, att_len
      character(len=*), intent(in) :: att_name
      integer :: retval, i
      character(len=1024) :: text_val
      integer(kind=1), allocatable :: byte_vals(:)
      integer(kind=2), allocatable :: short_vals(:)
      integer, allocatable :: int_vals(:)
      real, allocatable :: float_vals(:)
      double precision, allocatable :: double_vals(:)

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
         write(*, '(A)') "(user-defined type)"
      end select
   end subroutine print_att_value

   ! Print all attributes for a given variable (or NF90_GLOBAL).
   subroutine print_attributes(ncid, varid, natts, indent)
      integer, intent(in) :: ncid, varid, natts
      character(len=*), intent(in) :: indent
      integer :: a, retval, att_type, att_len
      character(len=NF90_MAX_NAME) :: att_name

      do a = 1, natts
         retval = nf90_inq_attname(ncid, varid, a, att_name)
         if (retval /= nf90_noerr) call handle_err(retval)
         retval = nf90_inquire_attribute(ncid, varid, att_name, att_type, att_len)
         if (retval /= nf90_noerr) call handle_err(retval)

         write(*, '(A,A,A,A,A,I0,A)', advance='no') indent, trim(att_name), &
              ": type ", trim(type_name(ncid, att_type)), ", length ", att_len, &
              ", value: "
         call print_att_value(ncid, varid, att_name, att_type, att_len)
      end do
   end subroutine print_attributes

   ! Print user-defined types in a group.
   subroutine print_user_types(ncid, indent)
      integer, intent(in) :: ncid
      character(len=*), intent(in) :: indent
      integer :: retval, ntypes, t
      integer :: typeids(100)
      character(len=NF90_MAX_NAME) :: tname
      integer :: type_size, base_type, nfields, type_class
      integer :: num_members, m, member_value
      character(len=NF90_MAX_NAME) :: member_name

      retval = nf90_inq_typeids(ncid, ntypes, typeids)
      if (retval /= nf90_noerr) call handle_err(retval)

      if (ntypes == 0) return

      write(*, '(A)') ""
      write(*, '(A,A)') indent, "User-Defined Types:"

      do t = 1, ntypes
         retval = nf90_inq_user_type(ncid, typeids(t), tname, type_size, &
              base_type, nfields, type_class)
         if (retval /= nf90_noerr) call handle_err(retval)

         select case (type_class)
         case (NF90_ENUM)
            retval = nf90_inq_enum(ncid, typeids(t), tname, base_type, &
                 type_size, num_members)
            if (retval /= nf90_noerr) call handle_err(retval)

            write(*, '(A,A,A,A,A,A,A,I0,A)') indent, "  ", trim(tname), &
                 ": enum of ", trim(type_name(ncid, base_type)), ", ", &
                 "", num_members, " member(s)"

            do m = 1, num_members
               retval = nf90_inq_enum_member(ncid, typeids(t), m, &
                    member_name, member_value)
               if (retval /= nf90_noerr) call handle_err(retval)
               write(*, '(A,A,A,A,I0)') indent, "    ", &
                    trim(member_name), " = ", member_value
            end do

         case (NF90_OPAQUE)
            retval = nf90_inq_opaque(ncid, typeids(t), tname, type_size)
            if (retval /= nf90_noerr) call handle_err(retval)
            write(*, '(A,A,A,A,I0,A)') indent, "  ", trim(tname), &
                 ": opaque, ", type_size, " bytes"

         case (NF90_COMPOUND)
            write(*, '(A,A,A,A,I0,A,I0,A)') indent, "  ", trim(tname), &
                 ": compound, ", type_size, " bytes, ", nfields, " field(s)"

         case (NF90_VLEN)
            write(*, '(A,A,A,A)') indent, "  ", trim(tname), ": vlen"

         case default
            write(*, '(A,A,A,A,I0)') indent, "  ", trim(tname), &
                 ": unknown type class ", type_class
         end select
      end do
   end subroutine print_user_types

   ! Print metadata for a single group (and recurse into subgroups).
   recursive subroutine print_group(ncid, group_name, indent)
      integer, intent(in) :: ncid
      character(len=*), intent(in) :: group_name, indent
      integer :: retval, ndims, nvars, ngatts, unlimdimid
      integer :: d, v
      character(len=NF90_MAX_NAME) :: dim_name, var_name
      integer :: dim_len
      integer :: var_type, var_ndims, var_natts
      integer :: var_dimids(NF90_MAX_VAR_DIMS)
      character(len=NF90_MAX_NAME) :: var_type_name
      integer :: ngroups, g
      integer :: grpids(100)
      character(len=NF90_MAX_NAME) :: grp_name
      character(len=256) :: sub_indent

      ! Get top-level counts.
      retval = nf90_inquire(ncid, ndims, nvars, ngatts, unlimdimid)
      if (retval /= nf90_noerr) call handle_err(retval)

      write(*, '(A,A,A)') indent, "Group: ", trim(group_name)
      write(*, '(A,A,I0)') indent, "Number of dimensions: ", ndims
      write(*, '(A,A,I0)') indent, "Number of variables: ", nvars
      write(*, '(A,A,I0)') indent, "Number of global attributes: ", ngatts
      if (unlimdimid >= 0) then
         write(*, '(A,A,I0)') indent, "Unlimited dimension id: ", unlimdimid
      else
         write(*, '(A,A)') indent, "No unlimited dimension"
      end if

      ! Print user-defined types.
      call print_user_types(ncid, indent)

      ! Print dimensions.
      write(*, '(A)') ""
      write(*, '(A,A)') indent, "Dimensions:"
      do d = 1, ndims
         retval = nf90_inquire_dimension(ncid, d, dim_name, dim_len)
         if (retval /= nf90_noerr) call handle_err(retval)
         if (d - 1 == unlimdimid) then
            write(*, '(A,A,A,A,I0,A)') indent, "  ", trim(dim_name), &
                 " = ", dim_len, " (unlimited)"
         else
            write(*, '(A,A,A,A,I0)') indent, "  ", trim(dim_name), &
                 " = ", dim_len
         end if
      end do

      ! Print global attributes.
      if (ngatts > 0) then
         write(*, '(A)') ""
         write(*, '(A,A)') indent, "Global Attributes:"
         call print_attributes(ncid, NF90_GLOBAL, ngatts, indent // "  ")
      end if

      ! Print variables.
      write(*, '(A)') ""
      write(*, '(A,A)') indent, "Variables:"
      do v = 1, nvars
         retval = nf90_inquire_variable(ncid, v, name=var_name, xtype=var_type, &
              ndims=var_ndims, dimids=var_dimids, nAtts=var_natts)
         if (retval /= nf90_noerr) call handle_err(retval)

         var_type_name = type_name(ncid, var_type)
         write(*, '(A,A,A,A,A,A,I0,A)', advance='no') indent, "  ", &
              trim(var_name), ": type ", &
              trim(var_type_name), ", ", var_ndims, &
              " dimension(s)"

         ! Print dimension names for this variable.
         if (var_ndims > 0) then
            write(*, '(A)', advance='no') " ("
            do d = 1, var_ndims
               if (d > 1) write(*, '(A)', advance='no') ", "
               retval = nf90_inquire_dimension(ncid, var_dimids(d), dim_name)
               if (retval /= nf90_noerr) call handle_err(retval)
               write(*, '(A)', advance='no') trim(dim_name)
            end do
            write(*, '(A)', advance='no') ")"
         end if
         write(*, '(A,I0,A)') ", ", var_natts, " attribute(s)"

         ! Print variable attributes.
         if (var_natts > 0) then
            call print_attributes(ncid, v, var_natts, indent // "    ")
         end if
      end do

      ! Recurse into subgroups.
      retval = nf90_inq_grps(ncid, ngroups, grpids)
      if (retval /= nf90_noerr) call handle_err(retval)

      if (ngroups > 0) then
         sub_indent = indent // "  "

         do g = 1, ngroups
            retval = nf90_inq_grpname(grpids(g), grp_name)
            if (retval /= nf90_noerr) call handle_err(retval)
            write(*, '(A)') ""
            call print_group(grpids(g), trim(grp_name), trim(sub_indent))
         end do
      end if
   end subroutine print_group

   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop 2
   end subroutine handle_err

end program f_dump_nc4_metadata
