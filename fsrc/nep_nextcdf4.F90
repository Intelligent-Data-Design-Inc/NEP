!> @file
!! @brief Fortran bit-buffer interfaces for NEXTCDF-4.
!! @author Edward Hartnett, Intelligent Data Design, Inc.
module nep_nextcdf4
  use iso_c_binding
  use netcdf, only: nf90_noerr, nf90_einval, nf90_echar, nf90_ebadtype, nf90_enomem, nf90_erange, nf90_enotvar, &
       nf90_char, nf90_float, nf90_double, nf90_inquire_variable, nf90_inquire_dimension
  implicit none
  private
  integer, parameter, public :: nf90_nextcdf4 = ior(64,ishft(9,19)), nf90_netcdf4_model = 67108864
  integer, parameter, public :: nf90_float16=17, nf90_bfloat16=18, nf90_float8_e4m3=19, nf90_float8_e5m2=20
  integer, parameter, public :: nf90_float6_e2m3=21, nf90_float6_e3m2=22, nf90_float4_e2m1=23
  integer, parameter, public :: nf90_complex=24, nf90_doublecomplex=25
  integer, parameter, public :: nf90_bitfield8=26, nf90_bitfield16=27, nf90_bitfield32=28, nf90_bitfield64=29
  integer, parameter, public :: nf90_std_ref=30
  public :: nf90_nextcdf4_initialize
  public :: nf90_get_var_float16, nf90_put_var_float16, nf90_get_var_bfloat16, nf90_put_var_bfloat16
  !> Read IEEE binary16 bit patterns into a scalar or rank-1–7 buffer.
  !! @param ncid File/group ID from NetCDF-Fortran.
  !! @param varid One-based variable ID.
  !! @param values Raw integer(c_int16_t) buffer; unselected values are preserved.
  !! @param start Optional one-based indices in Fortran order.
  !! @param count Optional leading memory rectangle extents.
  !! @param stride Optional positive file strides in Fortran order.
  !! @return NetCDF status, with completed results retained on nf90_erange.
  !! @see nextcdf4_fortran
  !! @author Edward Hartnett, Intelligent Data Design, Inc.
  interface nf90_get_var_float16
    module procedure nep_get_float16_0, nep_get_float16_1, nep_get_float16_2, nep_get_float16_3
    module procedure nep_get_float16_4, nep_get_float16_5, nep_get_float16_6, nep_get_float16_7
  end interface
  !> Write IEEE binary16 bit patterns from a scalar or rank-1–7 buffer.
  !! @param ncid File/group ID from NetCDF-Fortran.
  !! @param varid One-based variable ID.
  !! @param values Borrowed raw integer(c_int16_t) input buffer.
  !! @param start Optional one-based indices in Fortran order.
  !! @param count Optional leading memory rectangle extents.
  !! @param stride Optional positive file strides in Fortran order.
  !! @return NetCDF status; range-warning writes are completed, not rolled back.
  !! @see nextcdf4_fortran
  !! @author Edward Hartnett, Intelligent Data Design, Inc.
  interface nf90_put_var_float16
    module procedure nep_put_float16_0, nep_put_float16_1, nep_put_float16_2, nep_put_float16_3
    module procedure nep_put_float16_4, nep_put_float16_5, nep_put_float16_6, nep_put_float16_7
  end interface
  !> Read bfloat16 bit patterns into a scalar or rank-1–7 buffer.
  !! @param ncid File/group ID from NetCDF-Fortran.
  !! @param varid One-based variable ID.
  !! @param values Raw integer(c_int16_t) buffer; unselected values are preserved.
  !! @param start Optional one-based indices in Fortran order.
  !! @param count Optional leading memory rectangle extents.
  !! @param stride Optional positive file strides in Fortran order.
  !! @return NetCDF status, with completed results retained on nf90_erange.
  !! @see nextcdf4_fortran
  !! @author Edward Hartnett, Intelligent Data Design, Inc.
  interface nf90_get_var_bfloat16
    module procedure nep_get_bfloat16_0, nep_get_bfloat16_1, nep_get_bfloat16_2, nep_get_bfloat16_3
    module procedure nep_get_bfloat16_4, nep_get_bfloat16_5, nep_get_bfloat16_6, nep_get_bfloat16_7
  end interface
  !> Write bfloat16 bit patterns from a scalar or rank-1–7 buffer.
  !! @param ncid File/group ID from NetCDF-Fortran.
  !! @param varid One-based variable ID.
  !! @param values Borrowed raw integer(c_int16_t) input buffer.
  !! @param start Optional one-based indices in Fortran order.
  !! @param count Optional leading memory rectangle extents.
  !! @param stride Optional positive file strides in Fortran order.
  !! @return NetCDF status; range-warning writes are completed, not rolled back.
  !! @see nextcdf4_fortran
  !! @author Edward Hartnett, Intelligent Data Design, Inc.
  interface nf90_put_var_bfloat16
    module procedure nep_put_bfloat16_0, nep_put_bfloat16_1, nep_put_bfloat16_2, nep_put_bfloat16_3
    module procedure nep_put_bfloat16_4, nep_put_bfloat16_5, nep_put_bfloat16_6, nep_put_bfloat16_7
  end interface
  interface
    integer(c_int) function c_initialize() bind(C,name='nep_f_initialize')
      import c_int
    end function
    integer(c_int) function c_transfer(n,v,t,w,m,s,c,stride,p) bind(C,name='nep_f_transfer')
      import c_int,c_ptr
      integer(c_int), value :: n,v,t,w,m
      type(c_ptr), value :: s,c,stride,p
    end function
  end interface
contains
  !> Register NEXTCDF-4 explicitly without relying on .ncrc autoload.
  !! @return nf90_noerr on success, nf90_einternal on registration failure.
  !! @author Edward Hartnett, Intelligent Data Design, Inc.
  integer function nf90_nextcdf4_initialize() result(status)
    status = c_initialize()
  end function

  subroutine prepare(ncid,varid,xtype,extents,start,count,stride,cs,cc,ct,edges,method,n,status)
    integer, intent(in) :: ncid,varid,xtype
    integer(c_size_t), intent(in) :: extents(:)
    integer, optional, intent(in) :: start(:),count(:),stride(:)
    integer(c_size_t), intent(out) :: cs(7),cc(7),edges(7),n
    integer(c_intptr_t), intent(out) :: ct(7)
    integer(c_int), intent(out) :: method
    integer, intent(out) :: status
    integer :: rank, r, i, stored, ids(7), len
    logical :: whole
    r = size(extents)
    cs = 0; cc = 1; ct = 1; edges = 1; n = 1; method = 0
    status = nf90_enotvar
    if (varid < 1) return
    status = nf90_inquire_variable(ncid,varid,xtype=stored,ndims=rank)
    if (status /= nf90_noerr) return
    status = nf90_einval
    if (rank /= r) return
    if (present(start)) then
      if (size(start) /= r) return
      if (any(start < 1)) return
      cs(1:r) = int(start(r:1:-1),c_size_t)-1
    endif
    if (present(count)) then
      if (size(count) /= r) return
      if (any(count < 0)) return
      edges(1:r) = int(count,c_size_t)
    else
      edges(1:r) = extents
    endif
    if (any(edges(1:r) > extents)) return
    cc(1:r) = edges(r:1:-1)
    if (present(stride)) then
      if (size(stride) /= r) return
      if (any(stride < 1)) return
      ct(1:r) = int(stride(r:1:-1),c_intptr_t)
    endif
    whole = .not.(present(start).or.present(count).or.present(stride))
    if (whole .and. r > 0) then
      status = nf90_inquire_variable(ncid,varid,dimids=ids(1:r))
      if (status /= nf90_noerr) return
      do i=1,r
        status = nf90_inquire_dimension(ncid,ids(i),len=len)
        if (status /= nf90_noerr) return
        status = nf90_einval
        if (int(len,c_size_t) /= extents(i)) return
      enddo
    endif
    do i=1,r
      status = nf90_einval
      if (edges(i) == 0) then
        n = 0
        exit
      endif
      if (n > huge(n)/2/edges(i)) return
      n = n*edges(i)
    enddo
    status = nf90_echar
    if (stored == nf90_char) return
    status = nf90_ebadtype
    if (stored /= xtype .and. stored /= nf90_float .and. stored /= nf90_double) return
    if (.not.whole) then
      method = 2
      if (r == 0) method = 1
      if (present(start).and..not.present(count).and..not.present(stride)) then
        if (all(edges(1:r) == 1)) method = 1
      endif
      if (present(stride)) method = 3
    endif
    status = nf90_noerr
  end subroutine
  include 'procedures.inc'
end module nep_nextcdf4
