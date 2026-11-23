! Author: Edward Hartnett, Intelligent Data Design, Inc.
program consumer
  use iso_c_binding
  use netcdf
  use nep_nextcdf4
  implicit none
  integer :: n,d,v
  integer(c_int16_t) :: bits(2),out(2)
  bits=[int(z'3c00',c_int16_t),int(z'bc00',c_int16_t)]
  call check(nf90_nextcdf4_initialize())
  call check(nf90_create('consumer.nc',nf90_nextcdf4,n))
  call check(nf90_def_dim(n,'x',2,d))
  call check(nf90_def_var(n,'half',nf90_float16,[d],v))
  call check(nf90_enddef(n))
  call check(nf90_put_var_float16(n,v,bits))
  call check(nf90_close(n))
  call check(nf90_open('consumer.nc',nf90_nextcdf4,n))
  call check(nf90_get_var_float16(n,v,out))
  if(any(bits/=out)) stop 2
  call check(nf90_close(n))
contains
  subroutine check(s)
    integer,intent(in) :: s
    if(s/=nf90_noerr) then
      print *, trim(nf90_strerror(s))
      stop 1
    endif
  end subroutine
end program
