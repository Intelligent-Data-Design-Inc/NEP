! f_square16_par.f90 - Parallel NetCDF I/O example: 4 ranks write 16x16 dataset.
! Each rank writes an 8x8 quadrant filled with its rank number.
! Edward Hartnett, Intelligent Data Design, Inc.
! v1.9.0 Sprint 4
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
      data(j, i) = rank
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
  ! rank 0: (1, 1)   rank 1: (1, 9)
  ! rank 2: (9, 1)   rank 3: (9, 9)
  arr_start(1) = (rank / 2) * QUAD_SIZE + 1  ! row offset
  arr_start(2) = mod(rank, 2) * QUAD_SIZE + 1  ! column offset
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
      if (read_data(j, i) /= rank) then
        print *, 'Rank', rank, ': Data mismatch at ', arr_start(1)+i-1, ',', arr_start(2)+j-1, &
                 ': expected', rank, 'got', read_data(j, i)
        call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if
    end do
  end do

  if (rank == 0) then
    print *, 'Parallel I/O write and read-back successful. File: f_square16_par.nc'
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
