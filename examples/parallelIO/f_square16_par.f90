! f_square16_par.f90 - Parallel I/O hello: initialize MPI, print rank and size, exit 0.
! Edward Hartnett, Intelligent Data Design, Inc.
! v1.9.0 Sprint 3
program f_square16_par
  use mpi
  implicit none
  integer :: rank, size, ierr

  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)

  print *, 'Hello from rank', rank, 'of', size

  call MPI_Finalize(ierr)
end program f_square16_par
