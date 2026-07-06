!> @file f_square16_par.f90
!! @brief Parallel NetCDF-4 I/O with MPI using
!! a 2x2 rank decomposition (Fortran)
!!
!! Fortran equivalent of square16_par.c, showing
!! how to use NetCDF-4 parallel I/O with MPI
!! from Fortran 90. Four MPI ranks cooperate to
!! write a single 16x16 integer
!! array, each contributing an 8x8 quadrant filled with its own rank number.
!!
!! The program demonstrates the fundamental parallel I/O pattern: open a file
!! collectively, define the full dataset shape once, then have each rank write
!! only its local portion using hyperslab
!! selections (start/count). After writing,
!! it performs a parallel read-back to verify correctness.
!!
!! **Learning Objectives:**
!! - Open/create NetCDF files in parallel with
!!   nf90_create_par() and nf90_open_par()
!! - Use nf90_var_par_access() to enable collective I/O mode (NF90_COLLECTIVE)
!! - Compute per-rank hyperslab offsets for a
!!   2D domain decomposition (1-based indexing)
!! - Write and read variable subsets with
!!   nf90_put_var()/nf90_get_var() via start/count
!! - Verify parallel write correctness with a coordinated read-back
!!
!! **Key Concepts:**
!! - **Collective I/O**: All ranks participate
!!   in every I/O call (NF90_COLLECTIVE),
!!   which allows the MPI-IO layer to optimize access patterns
!! - **Independent I/O**: Alternative mode (NF90_INDEPENDENT) where each rank
!!   calls I/O functions independently; less
!!   common for performance-critical code
!! - **Domain Decomposition**: Dividing a global array among MPI ranks; here a
!!   simple 2x2 block decomposition maps rank → quadrant
!! - **Hyperslab**: A rectangular sub-region selected by start() and count()
!!   passed to nf90_put_var / nf90_get_var; Fortran uses 1-based indexing
!! - **MPI_INFO_NULL**: Passed when no MPI-IO hints are needed; production code
!!   may supply hints (e.g., striping) via
!!   MPI_Info objects for better performance
!! - **Column-major storage**: Fortran arrays are stored column-major, so the
!!   innermost dimension varies fastest; dimension ordering is the reverse of C
!!
!! **Rank Layout (2×2 grid):**
!! @code
!!   col 0-7    col 8-15
!!  +----------+----------+
!!  | rank 0   | rank 1   |  rows 0-7
!!  +----------+----------+
!!  | rank 2   | rank 3   |  rows 8-15
!!  +----------+----------+
!! @endcode
!!
!! **Prerequisites:**
!! - f_simple_nc4.f90 - NetCDF-4 format basics
!! - square16_par.c - C equivalent
!! - An MPI installation and NetCDF-C/Fortran
!!   libraries built with parallel support
!!
!! **Related Examples:**
!! - square16_par.c - C equivalent
!!
!! **Compilation:**
!! @code
!! mpif90 -o f_square16_par f_square16_par.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! mpirun -n 4 ./f_square16_par
!! @endcode
!!
!! **Expected Output:**
!! @code
!!  Parallel I/O write and read-back successful. File: f_square16_par.nc
!! @endcode
!!
!! @note Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to Writing
!! High-Performance Programs for Scientific Data Management, Second Edition"
!! (https://www.amazon.com/dp/B0H7Q1Z75L)
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date June 3, 2026

program f_square16_par
  use netcdf
  use mpi
  implicit none

  integer, parameter :: NDIMS = 2
  integer, parameter :: QUAD_SIZE = 8
  integer, parameter :: DIM_SIZE = 16

  integer :: rank, size, ierr
  integer :: ncid, varid, dimids(NDIMS)
  integer :: data(QUAD_SIZE, QUAD_SIZE)
  integer :: arr_start(NDIMS), count(NDIMS)
  integer :: read_data(QUAD_SIZE, QUAD_SIZE)
  integer :: i, j
  integer :: err2

  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)

  ! Require exactly 4 processes for 2x2 grid
  if (size /= 4) then
    if (rank == 0) then
      print *, 'Error: This program requires exactly 4 MPI processes, got', size
    end if
    call MPI_Finalize(ierr)
    stop 1
  end if

  ! Fill 8x8 array with rank number
  do i = 1, QUAD_SIZE
    do j = 1, QUAD_SIZE
      data(i, j) = rank
    end do
  end do

  ! Create file with parallel NetCDF-4
  err2 = nf90_create_par('f_square16_par.nc', ior(NF90_NETCDF4, NF90_MPIIO), &
                        MPI_COMM_WORLD, MPI_INFO_NULL, ncid)
  if (err2 /= NF90_NOERR) call handle_err(err2, rank)

  ! Define dimensions
  err2 = nf90_def_dim(ncid, 'i', DIM_SIZE, dimids(1))
  if (err2 /= NF90_NOERR) call handle_err(err2, rank)
  err2 = nf90_def_dim(ncid, 'j', DIM_SIZE, dimids(2))
  if (err2 /= NF90_NOERR) call handle_err(err2, rank)

  ! Define variable
  err2 = nf90_def_var(ncid, 'sample_data', NF90_INT, dimids, varid)
  if (err2 /= NF90_NOERR) call handle_err(err2, rank)

  ! End define mode
  err2 = nf90_enddef(ncid)
  if (err2 /= NF90_NOERR) call handle_err(err2, rank)

  ! Enable collective I/O
  err2 = nf90_var_par_access(ncid, varid, NF90_COLLECTIVE)
  if (err2 /= NF90_NOERR) call handle_err(err2, rank)

  ! Calculate start position (Fortran is 1-based)
  ! rank 0: (1, 1)   rank 1: (9, 1)
  ! rank 2: (1, 9)   rank 3: (9, 9)
  arr_start(2) = (rank / 2) * QUAD_SIZE + 1  ! row offset (dim 2 in output)
  arr_start(1) = mod(rank, 2) * QUAD_SIZE + 1  ! column offset (dim 1 in output)
  count(1) = QUAD_SIZE
  count(2) = QUAD_SIZE

  ! Collective write
  err2 = nf90_put_var(ncid, varid, data, start=arr_start, count=count)
  if (err2 /= NF90_NOERR) call handle_err(err2, rank)

  ! Close file
  err2 = nf90_close(ncid)
  if (err2 /= NF90_NOERR) call handle_err(err2, rank)

  ! Parallel read-back verification
  err2 = nf90_open_par('f_square16_par.nc', NF90_MPIIO, &
                       MPI_COMM_WORLD, MPI_INFO_NULL, ncid)
  if (err2 /= NF90_NOERR) call handle_err(err2, rank)
  err2 = nf90_inq_varid(ncid, 'sample_data', varid)
  if (err2 /= NF90_NOERR) call handle_err(err2, rank)
  err2 = nf90_var_par_access(ncid, varid, NF90_COLLECTIVE)
  if (err2 /= NF90_NOERR) call handle_err(err2, rank)
  err2 = nf90_get_var(ncid, varid, read_data, start=arr_start, count=count)
  if (err2 /= NF90_NOERR) call handle_err(err2, rank)
  err2 = nf90_close(ncid)
  if (err2 /= NF90_NOERR) call handle_err(err2, rank)

  ! Verify data
  do i = 1, QUAD_SIZE
    do j = 1, QUAD_SIZE
      if (read_data(i, j) /= rank) then
        print *, 'Rank', rank, &
                 ': Data mismatch at ', &
                 arr_start(2)+i-1, ',', &
                 arr_start(1)+j-1, &
                 ': expected', rank, &
                 'got', read_data(i, j)
        call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if
    end do
  end do

  if (rank == 0) then
    print *, 'Parallel I/O write and', &
             ' read-back successful.', &
             ' File: f_square16_par.nc'
  end if

  call MPI_Finalize(ierr)

contains

  subroutine handle_err(err, rank)
    integer, intent(in) :: err, rank
    integer :: ierr
    if (err /= NF90_NOERR) then
      if (rank == 0) print *, 'NetCDF error: ', nf90_strerror(err)
      call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
    end if
  end subroutine handle_err

end program f_square16_par
