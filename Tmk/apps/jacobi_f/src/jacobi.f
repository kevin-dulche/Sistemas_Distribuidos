C
C $Id: Jacobi.f,v 10.2 1997/06/02 05:28:37 alc Exp $
C
      program jacobi

      include 'Tmk_fortran.h'

      integer SIZE
      integer SIZE_1

      parameter (ITER = 100)
      parameter (SIZE = 1022)
      parameter (SIZE_1 = SIZE + 1)

      real a(0:SIZE_1, 0:SIZE_1)
      real b(0:SIZE_1, 0:SIZE_1)

      common /Tmk_shared_common/ b

      integer begin, end

      integer i, j, k

      real*8 time, timer

      call Tmk_startup()

      begin = (SIZE*Tmk_proc_id)/Tmk_nprocs + 1
      end   = (SIZE*(1 + Tmk_proc_id))/Tmk_nprocs
C
C       -- Initialization --
C
      if (Tmk_proc_id.eq.0) then
         do j = 0, SIZE+1
            b(j,      0) = 1.0
            b(j, SIZE+1) = 1.0
         enddo
         do i = 1, SIZE
            b(0,      i) = 1.0
            b(SIZE+1, i) = 1.0
         enddo
      endif

      write (6, 1) ITER, SIZE, SIZE
 1    format ('Performing ', I4, ' iterations on a ', I4, ' by ', I4,
     *' array')
C
C	-- Timing starts before the main loop --
C
      call Tmk_barrier(0)

      time = timer()

      do k = 1, ITER

         do j = begin, end
            do i = 1, SIZE
               a(i, j) = (b(i - 1, j) + b(i + 1, j) + b(i, j - 1) + b(i, 
     *j + 1)) / 4
            enddo
         enddo

         call Tmk_barrier(0)

         do j = begin, end
            do i = 1, SIZE
               b(i, j) = a(i, j)
            enddo
         enddo

         call Tmk_barrier(0)

      enddo

      write (6, 2) timer() - time
 2    format ('Test completed'/'Elapsed time = ', F6.2)

      call Tmk_exit(0)

      end
