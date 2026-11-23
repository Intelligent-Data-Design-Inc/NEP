! Author: Edward Hartnett, Intelligent Data Design, Inc.
program ftst_nextcdf4_types
  use iso_c_binding
  use netcdf
  use nep_nextcdf4
  implicit none
  integer :: n,dim,v,t,e,mode,i,status,values(16), d2, coord, ud
  integer(c_int16_t) :: one(1),readback(1)
  character(len=3) :: text
  character(len=32) :: typename
  integer :: base, bytes, members, typeids(2), ntypes, j, class
  real :: real_data(3)
  interface
    integer(c_int) function constants(p) bind(C,name='nep_test_constants')
      import c_int
      integer(c_int), intent(in) :: p(16)
    end function
  end interface
  values=[nf90_nextcdf4,nf90_netcdf4_model,nf90_float16,nf90_bfloat16, &
       nf90_float8_e4m3,nf90_float8_e5m2,nf90_float6_e2m3,nf90_float6_e3m2,nf90_float4_e2m1, &
       nf90_complex,nf90_doublecomplex,nf90_bitfield8,nf90_bitfield16,nf90_bitfield32,nf90_bitfield64,nf90_std_ref]
  if (constants(int(values,c_int)) /= 0) stop 1
  call check(nf90_nextcdf4_initialize())
  do i=1,3
    mode=nf90_nextcdf4
    if (i==2) mode=ior(mode,nf90_netcdf4_model)
    if (i==3) mode=ior(mode,nf90_classic_model)
    call check(nf90_create('types.nc',mode,n))
    call check(nf90_def_dim(n,'x',3,dim))
    call check(nf90_def_var(n,'text',nf90_char,[dim],v))
    call check(nf90_def_var(n,'numeric',nf90_float,[dim],e))
    status=nf90_def_var(n,'half',nf90_float16,[dim],t)
    if (i==1) then
      call check(status)
    else
      if (status /= nf90_enotnc4) stop 2
    endif
    status=nf90_def_compound(n,4,'record',t)
    if (i==3) then
      if (status /= nf90_enotnc4) stop 3
    else
      call check(status)
      call check(nf90_insert_compound(n,t,'member',0,nf90_int))
      call check(nf90_def_enum(n,nf90_int,'enum',ud))
      call check(nf90_insert_enum(n,ud,'one',1))
    endif
    call check(nf90_enddef(n))
    call check(nf90_put_var(n,v,'abc'))
    call check(nf90_put_var(n,e,[1.,2.,3.]))
    call check(nf90_close(n))
    call check(nf90_open('types.nc',nf90_nextcdf4,n))
    call check(nf90_get_var(n,v,text))
    if (text /= 'abc') stop 4
    call check(nf90_get_var(n,e,real_data))
    if (any(real_data /= [1.,2.,3.])) stop 5
    if (i/=3) then
      call check(nf90_inq_typeids(n,ntypes,typeids))
      if (ntypes /= 2) stop 9
      do j=1,ntypes
        call check(nf90_inq_user_type(n,typeids(j),typename,bytes,base,members,class))
        if (typename == 'record') then
          if (class /= nf90_compound .or. bytes /= 4 .or. members /= 1) stop 9
        else if (typename == 'enum') then
          if (class /= nf90_enum .or. base /= nf90_int .or. members /= 1) stop 9
        else
          stop 9
        endif
      enddo
    endif
    call check(nf90_close(n))
  enddo
  call check(nf90_create('append.nc',nf90_nextcdf4,n))
  call check(nf90_def_dim(n,'coord',nf90_unlimited,dim))
  call check(nf90_def_var(n,'coord',nf90_float16,[dim],coord))
  call check(nf90_enddef(n))
  one=int(z'3c00',c_int16_t)
  call check(nf90_put_var_float16(n,coord,one,start=[1]))
  call check(nf90_put_var_float16(n,coord,one,start=[2]))
  call check(nf90_get_var_float16(n,coord,readback,start=[2]))
  if (any(one /= readback)) stop 6
  call check(nf90_close(n))
  call check(nf90_open('append.nc',nf90_nextcdf4,n))
  call check(nf90_get_var_float16(n,coord,readback,start=[2]))
  if (any(one /= readback)) stop 7
  call check(nf90_close(n))
  include 'rank_calls.inc'
contains
  subroutine check(s)
    integer,intent(in) :: s
    if(s/=nf90_noerr) then
      print *, trim(nf90_strerror(s))
      stop 8
    endif
  end subroutine
  include 'rank_tests.inc'
end program
