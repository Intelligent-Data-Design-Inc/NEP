!> @file
!!
!! This is the Fortran wrapper for the NCCOMPRESS library.
!!
!! @author Edward Hartnett
!! @date Nov 13, 2025
!! @copyright Intelligent Data Design, Inc. All rights reserved.

module ncsqueeze

#if BUILD_BZIP2

  !> Interface to C function to set BZIP2 compression.
  interface
     integer(C_INT) function nc_def_var_bzip2(ncid, varid, level) bind(c)
       use iso_c_binding
       integer(C_INT), value :: ncid, varid, level
     end function nc_def_var_bzip2
  end interface

  !> Interface to C function to inquire about BZIP2 compression.
  interface
     integer(C_INT) function nc_inq_var_bzip2(ncid, varid, bzip2p, levelp) bind(c)
       use iso_c_binding
       integer(C_INT), value :: ncid, varid
       integer(C_INT), intent(inout):: bzip2p, levelp
     end function nc_inq_var_bzip2
  end interface

#endif

#if BUILD_LZ4

  !> Interface to C function to set LZ4 compression.
  interface
     integer(C_INT) function nc_def_var_lz4(ncid, varid, level) bind(c)
       use iso_c_binding
       integer(C_INT), value :: ncid, varid, level
     end function nc_def_var_lz4
  end interface

  !> Interface to C function to inquire about LZ4 compression.
  interface
     integer(C_INT) function nc_inq_var_lz4(ncid, varid, lz4p, levelp) bind(c)
       use iso_c_binding
       integer(C_INT), value :: ncid, varid
       integer(C_INT), intent(inout):: lz4p, levelp
     end function nc_inq_var_lz4
  end interface

#endif

  ! !> Interface to C function to set JPEG compression.
  ! interface
  !    function nc_def_var_jpeg(ncid, varid, quality_factor, nx, ny, rgb) bind(c)
  !      use iso_c_binding
  !      integer(C_INT), value :: ncid, varid, quality_factor, nx, ny, rgb
  !    end function nc_def_var_jpeg
  ! end interface

  ! !> Interface to C function to inquire about JPEG compression.
  ! interface
  !    function nc_inq_var_jpeg(ncid, varid, jpegp, quality_factorp, nxp, nyp, rgbp) bind(c)
  !      use iso_c_binding
  !      integer(C_INT), value :: ncid, varid
  !      integer(C_INT), intent(inout):: jpegp, quality_factorp, nxp, nyp, rgbp
  !    end function nc_inq_var_jpeg
  ! end interface

  ! !> Interface to C function to set LZF compression.
  ! interface
  !    function nc_def_var_lzf(ncid, varid) bind(c)
  !      use iso_c_binding
  !      integer(C_INT), value :: ncid, varid
  !    end function nc_def_var_lzf
  ! end interface

  ! !> Interface to C function to inquire about LZF compression.
  ! interface
  !    function nc_inq_var_lzf(ncid, varid, lzfp) bind(c)
  !      use iso_c_binding
  !      integer(C_INT), value :: ncid, varid
  !      integer(C_INT), intent(inout):: lzfp
  !    end function nc_inq_var_lzf
  ! end interface

contains
#if BUILD_BZIP2
  !> Turn on BZIP2 compression for a variable.
  !!
  !! This is the Fortran wrapper for the C function `nc_def_var_bzip2`,
  !! which enables BZIP2 compression for a variable in a netCDF-4/HDF5 file.
  !!
  !! @param ncid File or group ID.
  !! @param varid Variable ID (1-based Fortran indexing).
  !! @param level Compression level, from 1 to 9. Sets the block size to
  !!              100k, 200k, ..., 900k when compressing (9 is the BZIP2
  !!              default).
  !!
  !! @return 0 for success, netCDF error code otherwise.
  function nf90_def_var_bzip2(ncid, varid, level) result(status)
    use iso_c_binding
    implicit none
    integer, intent(in) :: ncid, varid, level
    integer :: status

    ! C varids start at 0, fortran at 1.
    status = nc_def_var_bzip2(ncid, varid - 1, level)
  end function nf90_def_var_bzip2

  !> Learn whether BZIP2 compression is on for a variable, and, if so,
  !! the compression level.
  !!
  !! This is the Fortran wrapper for the C function `nc_inq_var_bzip2`.
  !!
  !! @param ncid   File or group ID.
  !! @param varid  Variable ID (1-based Fortran indexing).
  !! @param bzip2p On return, set to 1 if BZIP2 is in use for this
  !!               variable, and 0 otherwise.
  !! @param levelp On return, set to the compression level (from 1 to 9)
  !!               if BZIP2 is in use.
  !!
  !! @return 0 for success, netCDF error code otherwise.
  function nf90_inq_var_bzip2(ncid, varid, bzip2p, levelp) result(status)
    use iso_c_binding
    implicit none
    integer, intent(in) :: ncid, varid
    integer, intent(inout) :: bzip2p, levelp
    integer :: status

    ! C varids start at 0, fortran at 1.
    status = nc_inq_var_bzip2(ncid, varid - 1, bzip2p, levelp)
  end function nf90_inq_var_bzip2
#endif

#if BUILD_LZ4
  !> Turn on LZ4 compression for a variable.
  !!
  !! This is the Fortran wrapper for the C function `nc_def_var_lz4`,
  !! which enables LZ4 compression for a variable in a netCDF-4/HDF5 file.
  !!
  !! @param ncid  File or group ID.
  !! @param varid Variable ID (1-based Fortran indexing).
  !! @param level Acceleration setting, from 1 to 9 (larger means faster
  !!              and less compressive).
  !!
  !! @return 0 for success, netCDF error code otherwise.
  function nf90_def_var_lz4(ncid, varid, level) result(status)
    use iso_c_binding
    implicit none
    integer, intent(in) :: ncid, varid, level
    integer :: status

    ! C varids start at 0, fortran at 1.
    status = nc_def_var_lz4(ncid, varid - 1, level)
  end function nf90_def_var_lz4

  !> Learn whether LZ4 compression is on for a variable, and, if so,
  !! the acceleration level.
  !!
  !! This is the Fortran wrapper for the C function `nc_inq_var_lz4`.
  !!
  !! @param ncid  File or group ID.
  !! @param varid Variable ID (1-based Fortran indexing).
  !! @param lz4p  On return, set to 1 if LZ4 is in use for this
  !!              variable, and 0 otherwise.
  !! @param levelp On return, set to the acceleration level (from 1 to 9)
  !!               if LZ4 is in use.
  !!
  !! @return 0 for success, netCDF error code otherwise.
  function nf90_inq_var_lz4(ncid, varid, lz4p, levelp) result(status)
    use iso_c_binding
    implicit none
    integer, intent(in) :: ncid, varid
    integer, intent(inout) :: lz4p, levelp
    integer :: status

    ! C varids start at 0, fortran at 1.
    status = nc_inq_var_lz4(ncid, varid - 1, lz4p, levelp)
  end function nf90_inq_var_lz4
#endif

  ! !> Set JPEG compression for a variable.
  ! !!
  ! !! @param ncid File or group ID.
  ! !! @param varid Variable ID.
  ! !! @param quality_factor Quality factor, between 1 and 100.
  ! !! @param nx size of X in image.
  ! !! @param ny size of Y in image.
  ! !! @param rgb color mode: 1 for RGB, 0 for MONO.
  ! !!
  ! !! @return 0 for success, error code otherwise.
  ! function nf90_def_var_jpeg(ncid, varid, quality_factor, nx, ny, rgb) result(status)
  !   use iso_c_binding
  !   implicit none
  !   integer, intent(in) :: ncid, varid, quality_factor, nx, ny, rgb
  !   integer :: status

  !   ! C varids start at 0, fortran at 1.
  !   status = nc_def_var_jpeg(ncid, varid - 1, quality_factor, nx, ny, rgb)
  ! end function nf90_def_var_jpeg

  ! !> Inquire about JPEG compression for a variable.
  ! !!
  ! !! @param ncid File or group ID.
  ! !! @param varid Variable ID.
  ! !! @param jpegp Pointer that gets 1 if JPEG is in use, 0
  ! !! otherwise. Ignored if NULL.
  ! !! @param quality_factorp Pointer to int which gets quality factor,
  ! !! between 1 and 100. Ignored if NULL.
  ! !! @param nxp Pointer to int which gets size of X in image.
  ! !! @param nyp Pointer to int which gets size of Y in image.
  ! !! @param rgbp Pointer to int which gets color mode: 1 for RGB, 0 for
  ! !! MONO.
  ! !!
  ! !! @return 0 for success, error code otherwise.
  ! function nf90_inq_var_jpeg(ncid, varid, jpegp, quality_factorp, nxp, nyp, rgbp) result(status)
  !   use iso_c_binding
  !   implicit none
  !   integer, intent(in) :: ncid, varid
  !   integer, intent(inout) :: jpegp, quality_factorp, nxp, nyp, rgbp
  !   integer :: status

  !   ! C varids start at 0, fortran at 1.
  !   status = nc_inq_var_jpeg(ncid, varid - 1, jpegp, quality_factorp, nxp, nyp, rgbp)
  ! end function nf90_inq_var_jpeg

  ! !> Set LZF compression for a variable.
  ! !!
  ! !! @param ncid File or group ID.
  ! !! @param varid Variable ID.
  ! !!
  ! !! @return 0 for success, error code otherwise.
  ! function nf90_def_var_lzf(ncid, varid) result(status)
  !   use iso_c_binding
  !   implicit none
  !   integer, intent(in) :: ncid, varid
  !   integer :: status

  !   ! C varids start at 0, fortran at 1.
  !   status = nc_def_var_lzf(ncid, varid - 1)
  ! end function nf90_def_var_lzf

  ! !> Inquire about LZF compression for a variable.
  ! !!
  ! !! @param ncid File or group ID.
  ! !! @param varid Variable ID.
  ! !! @param lzfp Pointer that gets 1 if LZF is in use, 0
  ! !! otherwise. Ignored if NULL.
  ! !!
  ! !! @return 0 for success, error code otherwise.
  ! function nf90_inq_var_lzf(ncid, varid, lzfp) result(status)
  !   use iso_c_binding
  !   implicit none
  !   integer, intent(in) :: ncid, varid
  !   integer, intent(inout) :: lzfp
  !   integer :: status

  !   ! C varids start at 0, fortran at 1.
  !   status = nc_inq_var_lzf(ncid, varid - 1, lzfp)
  ! end function nf90_inq_var_lzf

end module ncsqueeze
