! Author: Edward Hartnett, Intelligent Data Design, Inc.
program ftst_nextcdf4_small_float
  use iso_c_binding
  use netcdf
  use nep_nextcdf4
  implicit none
  integer :: ncid, dims(2), v, b, f, d, scalar, typed_half, typed_bhalf, charvar
  integer(c_int16_t) :: a(2,3), out(4,3), bits, empty(0,3)
  real(c_float) :: reals(2,3)
  real(c_double) :: doubles(2,3)
  a = reshape([int(z'0000',c_int16_t), int(z'8000',c_int16_t), int(z'3c00',c_int16_t), &
       int(z'bc00',c_int16_t), int(z'7c00',c_int16_t), int(z'7e01',c_int16_t)], [2,3])
  call check(nf90_nextcdf4_initialize())
  call check(nf90_create('small.nc', nf90_nextcdf4, ncid))
  call check(nf90_def_dim(ncid,'x',2,dims(1)))
  call check(nf90_def_dim(ncid,'y',3,dims(2)))
  call check(nf90_def_var(ncid,'half',nf90_float16,dims,v))
  call check(nf90_def_var(ncid,'bhalf',nf90_bfloat16,dims,b))
  call check(nf90_def_var(ncid,'float',nf90_float,dims,f))
  call check(nf90_def_var(ncid,'double',nf90_double,dims,d))
  call check(nf90_def_var(ncid,'scalar',nf90_float16,scalar))
  call check(nf90_def_var(ncid,'typed_half',nf90_float16,dims,typed_half))
  call check(nf90_def_var(ncid,'typed_bhalf',nf90_bfloat16,dims,typed_bhalf))
  call check(nf90_def_var(ncid,'char',nf90_char,dims,charvar))
  call check(nf90_enddef(ncid))
  call check(nf90_put_var_float16(ncid,v,a))
  call check(nf90_put_var_bfloat16(ncid,b,a))
  bits = int(z'3c00',c_int16_t)
  call check(nf90_put_var_float16(ncid,scalar,bits))
  call check(nf90_get_var_float16(ncid,scalar,bits,start=[integer::]))
  if (bits /= int(z'3c00',c_int16_t)) stop 2
  call check(nf90_put_var_float16(ncid,scalar,bits,count=[integer::]))
  call check(nf90_get_var_float16(ncid,scalar,bits))
  call check(nf90_put_var_bfloat16(ncid,b,a,start=[1,1],count=[2,3]))
  call check(nf90_put_var_float16(ncid,v,a,start=[1,1],stride=[1,1]))
  out = -123
  call check(nf90_get_var_float16(ncid,v,out(1:4:2,:)))
  if (any(out(1:4:2,:) /= a) .or. any(out(2:4:2,:) /= -123)) stop 3
  out = -123
  call check(nf90_get_var_float16(ncid,v,out,count=[1,2]))
  if (any(out(1:1,1:2) /= a(1:1,1:2)) .or. any(out(2:4,:) /= -123)) stop 4
  call check(nf90_get_var_bfloat16(ncid,b,out(1:2,:),start=[1,1],stride=[1,1]))
  if (any(out(1:2,:) /= a)) stop 5
  call check(nf90_get_var_float16(ncid,v,empty,count=[0,3]))
  out = -123
  call check(nf90_get_var_float16(ncid,v,out,count=[2,2],stride=[1,2]))
  if (any(out(1:2,1:2) /= a(:,1:3:2)) .or. any(out(3:4,:) /= -123)) stop 12
  call check(nf90_put_var_float16(ncid,v,a(:,1:3:2),count=[2,2],stride=[1,2]))
  call check(nf90_put_var_bfloat16(ncid,b,a(:,1:3:2),count=[2,2],stride=[1,2]))
  call check(nf90_get_var_bfloat16(ncid,b,out,count=[2,2],stride=[1,2]))
  if (any(out(1:2,1:2) /= a(:,1:3:2))) stop 13
  call expect(nf90_get_var_float16(ncid,v,out,count=[huge(1),huge(1)]),nf90_einval)
  call expect(nf90_get_var_float16(ncid,charvar,empty,count=[0,3]),nf90_echar)
  out = -123
  call expect(nf90_get_var_float16(ncid,v,out,count=[1,1],start=[3,1]),nf90_eedge)
  if (any(out /= -123)) stop 14
  call expect(nf90_get_var_float16(ncid,v,out),nf90_einval)
  call expect(nf90_get_var_float16(ncid,v,bits),nf90_einval)
  call expect(nf90_get_var_float16(ncid,v,out,start=[1]),nf90_einval)
  call expect(nf90_get_var_float16(ncid,v,out,start=[0,1]),nf90_einval)
  call expect(nf90_get_var_float16(ncid,v,out,count=[-1,1]),nf90_einval)
  call expect(nf90_get_var_float16(ncid,v,out,stride=[0,1]),nf90_einval)
  call expect(nf90_get_var_float16(-1,v,out),nf90_ebadid)
  call expect(nf90_get_var_float16(ncid,999,out),nf90_enotvar)
  call expect(nf90_get_var_float16(ncid,-huge(1)-1,out),nf90_enotvar)
  call expect(nf90_get_var_bfloat16(ncid,v,out(1:2,:)),nf90_ebadtype)
  reals = 1.0
  doubles = 1.0
  call check(nf90_put_var(ncid,typed_half,reals))
  call check(nf90_put_var(ncid,typed_bhalf,doubles))
  reals = 0.0
  doubles = 0.0
  call check(nf90_get_var(ncid,typed_half,doubles))
  call check(nf90_get_var(ncid,typed_bhalf,reals))
  if (any(doubles /= 1.0_c_double) .or. any(reals /= 1.0_c_float)) stop 15
  call check(nf90_put_var(ncid,f,reals))
  call check(nf90_put_var(ncid,d,doubles))
  call check(nf90_get_var_float16(ncid,f,out(1:2,:)))
  if (any(out(1:2,:) /= int(z'3c00',c_int16_t))) stop 6
  call check(nf90_get_var_bfloat16(ncid,d,out(1:2,:)))
  if (any(out(1:2,:) /= int(z'3f80',c_int16_t))) stop 7
  call check(nf90_put_var_float16(ncid,d,a))
  call check(nf90_put_var_bfloat16(ncid,f,a))
  reals = 70000.0
  call check(nf90_put_var(ncid,f,reals))
  call expect(nf90_get_var_float16(ncid,f,out(1:2,:)),nf90_erange)
  if (any(out(1:2,:) /= int(z'7c00',c_int16_t))) stop 8
  call check(nf90_close(ncid))
  call check(nf90_open('small.nc',nf90_nextcdf4,ncid))
  call check(nf90_get_var_float16(ncid,v,out(1:2,:)))
  if (any(out(1:2,:) /= a)) stop 9
  call check(nf90_get_var_bfloat16(ncid,b,out(1:2,:)))
  if (any(out(1:2,:) /= a)) stop 10
  call check(nf90_close(ncid))
contains
  subroutine check(status)
    integer, intent(in) :: status
    if (status /= nf90_noerr) then
      print *, trim(nf90_strerror(status))
      stop 1
    endif
  end subroutine
  subroutine expect(status,want)
    integer, intent(in) :: status,want
    if (status /= want) then
      print *, 'expected',want,'got',status
      stop 11
    endif
  end subroutine
end program
