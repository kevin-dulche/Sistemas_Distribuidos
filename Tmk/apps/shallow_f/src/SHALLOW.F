C
C     BENCHMARK WEATHER PREDICTION PROGRAM FOR COMPARING THE
C     PREFORMANCE OF CURRENT SUPERCOMPUTERS. THE MODEL IS
C     BASED OF THE PAPER - THE DYNAMICS OF FINITE-DIFFERENCE
C     MODELS OF THE SHALLOW-WATER EQUATIONS, BY ROBERT SADOURNY
C     J. ATM. SCIENCES, VOL 32, NO 4, APRIL 1975.
C
C     CODE BY PAUL N. SWARZTRAUBER, NATIONAL CENTER FOR
C     ATMOSPHERIC RESEARCH, BOULDER, CO,  OCTOBER 1984.
C
C     MODIFIED BY R. K. SATO, NCAR, APRIL 7, 1986 FOR MULTITASKING.
C
      INCLUDE 'Tmk_fortran.h'
C
      PARAMETER (N1=1024, N2=256, N3=1032)
      COMMON  /Tmk_shared_common/ U(N1,N2),t1(N3),
     1        V(N1,N2),t2(N3),
     2        P(N1,N2),t3(N3),CU(N1,N2),t4(N3),CV(N1,N2),t5(N3),
     3        Z(N1,N2),t6(N3),H(N1,N2),
     4        t7(N3),UNEW(N1,N2),t8(N3),VNEW(N1,N2),t9(N3),PNEW(N1,N2),
     5        t10(N3),UOLD(N1,N2),t11(N3),VOLD(N1,N2),
     6	      t12(N3),POLD(N1,N2),t13(N3),PSI(N1,N2)
      COMMON /SHTIME/ T100, T200, T300
      REAL*8 t100,t200,t300
      COMMON /CONS/ DT,TDT,DX,DY,A,ALPHA,ITMAX,MPRINT,M,N,MP1,
     1              NP1,EL,PI,TPI,DI,DJ,PCF,
     2		    NBEG,NEND,NBEGACT,NENDACT
      REAL*8 MFS100,MFS200,MFS300,TOTTIM
      REAL*8 timer,t00,tstart,tend,ctime
      REAL*8 times
      COMMON /tt/ times(15)
C 
C Gene Wagenbreth, Sept 2, 1992 
C temporary array to hold shrunk arrays for IO
C
      REAL io_temp(N1)
C
CAPR$ PARTITION PSI(*,FULLBLOCK)
CAPR$ PARTITION P(*,FULLBLOCK)
CAPR$ PARTITION V(*,FULLBLOCK)
CAPR$ PARTITION U(*,FULLBLOCK)
CAPR$ PARTITION POLD(*,FULLBLOCK)
CAPR$ PARTITION VOLD(*,FULLBLOCK)
CAPR$ PARTITION UOLD(*,FULLBLOCK)
CAPR$ PARTITION H(*,FULLBLOCK)
CAPR$ PARTITION CU(*,FULLBLOCK)
CAPR$ PARTITION Z(*,FULLBLOCK)
CAPR$ PARTITION CV(*,FULLBLOCK)
CAPR$ PARTITION PNEW(*,FULLBLOCK)
CAPR$ PARTITION VNEW(*,FULLBLOCK)
CAPR$ PARTITION UNEW(*,FULLBLOCK)
CAPR$ PARTITION IO_TEMP(FULLBLOCK)
C
C       REQUEST PROCESSORS FOR MICROTASKING
CMIC$ GETCPUS 
C
c***    assign standard output to local file 
C       open(unit=6,file=
C    1'shallow.out')
	   do i=1,15
		times(i)=0.0
	   end do
	   times(1)=times(1)-timer()
C
C       INITIALIZE CONSTANTS AND ARRAYS
C
      call Tmk_startup()

           times(2)=times(2)-timer()
      CALL INITAL
      CALL tmk_barrier(0)
           times(2)=times(2)+timer()

           times(3)=times(3)-timer()
C
C     PRINT INITIAL VALUES
C
      WRITE(6,390) N,M,DX,DY,DT,ALPHA,ITMAX
  390 FORMAT(' NUMBER OF POINTS IN THE X DIRECTION', I8/
     1       ' NUMBER OF POINTS IN THE Y DIRECTION', I8/
     2       ' GRID SPACING IN THE X DIRECTION    ', F8.0/
     3       ' GRID SPACING IN THE Y DIRECTION    ', F8.0/
     4       ' TIME STEP                          ', F8.0/
     5       ' TIME FILTER PARAMETER              ', F8.3/
     6       ' NUMBER OF ITERATIONS               ', I8)
      MNMIN = MIN0(M,N)
      print *, "MNMIN is ", MNMIN
C 
C Gene Wagenbreth, Sept 2, 1992 
C move shrunk arrays to temporary for IO
C
      if (Tmk_proc_id.eq.0) then
      do i=1,MNMIN
        io_temp(i)=pold(i,i)
      end do
C     write(6,391) (io_temp(i),i=1,MNMIN)
C     WRITE(6,391) (POLD(I,I),I=1,MNMIN)
  391 FORMAT(/' INITIAL DIAGONAL ELEMENTS OF P ', //(8E15.7))
      do i=1,MNMIN
        io_temp(i)=uold(i,i)
      end do
C     write(6,392) (io_temp(i),i=1,MNMIN)
C     WRITE(6,392) (UOLD(I,I),I=1,MNMIN)
  392 FORMAT(/' INITIAL DIAGONAL ELEMENTS OF U ', //(8E15.7))
      do i=1,MNMIN
        io_temp(i)=vold(i,i)
      end do
C     write(6,393) (io_temp(i),i=1,MNMIN)
C     WRITE(6,393) (VOLD(I,I),I=1,MNMIN)
  393 FORMAT(/' INITIAL DIAGONAL ELEMENTS OF V ', //(8E15.7))
      endif
C        DETERMINE OVERHEAD OF TIMING CALLS
      TSTART = TIMER()
      T00 = TIMER() - TSTART
C        GET TIME STAMP
      CALL TIME0
C        INITIAL CALL TO CPU CLOCK
C      TSTART = TIMER()
      TIME = 0.
      NCYCLE = 0
           times(3)=times(3)+timer()
   90 NCYCLE = NCYCLE + 1
      if (NCYCLE .eq. 1) then 
      	TSTART = TIMER()
      endif
C
C     COMPUTE CAPITAL  U, CAPITAL V, Z AND H
C
           times(4)=times(4)-timer()
      CALL CALC1
      CALL tmk_barrier(0)
           times(4)=times(4)+timer()
C
C     COMPUTE NEW VALUES U,V AND P
C
           times(5)=times(5)-timer()
      CALL CALC2
      CALL tmk_barrier(0)
           times(5)=times(5)+timer()
C
      if (Tmk_proc_id.eq.0) then
      TIME = TIME + DT
      IF(MOD(NCYCLE,MPRINT) .NE. 0) GO TO 370
	   times(6)=times(6)-timer()
      PTIME = TIME/3600.
      WRITE(6,350) NCYCLE,PTIME
  350 FORMAT(//' CYCLE NUMBER',I5,' MODEL TIME IN  HOURS', F6.2)
      do i=1,MNMIN
        io_temp(i)=pnew(i,i)
      end do
C     write(6,355) (io_temp(i),i=1,MNMIN)
C     WRITE(6,355) (PNEW(I,I),I=1,MNMIN)
  355 FORMAT(/' DIAGONAL ELEMENTS OF P ', //(8E15.7))
      do i=1,MNMIN
        io_temp(i)=unew(i,i)
      end do
C     write(6,360) (io_temp(i),i=1,MNMIN)
C     WRITE(6,360) (UNEW(I,I),I=1,MNMIN)
  360 FORMAT(/' DIAGONAL ELEMENTS OF U ', //(8E15.7))
      do i=1,MNMIN
        io_temp(i)=vnew(i,i)
      end do
C     write(6,360) (io_temp(i),i=1,MNMIN)
C     WRITE(6,365) (VNEW(I,I),I=1,MNMIN)
  365 FORMAT(/' DIAGONAL ELEMENTS OF V ', //(8E15.7))
C       COMPUTE MFLOPS RATES FOR THE THREE MAJOR LOOPS
      MFS100 = 0
      MFS200 = 0
      MFS300 = 0
      T100 = T100 - T00
      T200 = T200 - T00
      T300 = T300 - T00
      IF(T100.GT.0 ) MFS100 = 24.*M*N*1.E-6/T100
      IF(T200.GT.0 ) MFS200 = 26.*M*N*1.E-6/T200
      IF(T300.GT.0 ) MFS300 = 15.*M*N*1.E-6/T300
C
      TEND = TIMER()
      ctime = 0.0
      CTIME = dmax1(ctime, TEND - TSTART - T00)
      TCYC = CTIME/FLOAT(NCYCLE)
      WRITE(6,375) NCYCLE,CTIME,TCYC,
     >       T100,MFS100,T200,MFS200,T300,MFS300
  375 FORMAT(' CYCLE NUMBER',I5,' TOTAL COMPUTER TIME', E15.6,
     1       ' TIME PER CYCLE', E15.6 /
     2       ' TIME AND MEGAFLOPS FOR LOOP 100 ', E15.6,F8.3/
     3       ' TIME AND MEGAFLOPS FOR LOOP 200 ', E15.6,F8.3/
     4       ' TIME AND MEGAFLOPS FOR LOOP 300 ', E15.6,F8.3/ )
	   times(6)=times(6)+timer()
      endif
370   CONTINUE
C        TEST FOR END OF RUN
      IF(NCYCLE .GE. ITMAX) THEN
	   times(9)=times(9)-timer()
      TEND = TIMER()
      TOTTIM = (t100+t200+t300)*ITMAX
C        TIME STAMP THE END OF RUN
      CALL TIME0
C        COMPUTE THE MFLOPS FOR THE RUN
      if (Tmk_proc_id.eq.0) then
      XFLOPS = N*M*ITMAX*65*1.E-6/TOTTIM
      WRITE(6,9030) TOTTIM,XFLOPS
9030  FORMAT(///'  TOTAL CPU TIME FOR JOB = ', E15.5,/
     1          '  OVERALL MFLOPS RATE    = ', E15.5)
C     WRITE(6,9010)
9010  FORMAT(1H1)
c*** write values of final iteration to unit 20 
c     open(unit=20, file='shal.out')
c     WRITE(20,350) NCYCLE,PTIME
c     WRITE(20,355) (PNEW(I,I),I=1,MNMIN)
c     WRITE(20,360) (UNEW(I,I),I=1,MNMIN)
c     WRITE(20,365) (VNEW(I,I),I=1,MNMIN)
c***
	   times(1)=times(1)+timer()
	   print *,'Time for Entire run = ',times(1)
	   print *,'Time for Inital     = ',times(2)
	   print *,'Time for Start print= ',times(3)
	   print *,'Time for CALC1      = ',times(4)
	   print *,'Time for CALC2      = ',times(5)
	   print *,'Time for Prints     = ',times(6)
	   print *,'Time for CALC3Z     = ',times(7)
	   print *,'Time for CALC3      = ',times(8)
	   times(9)=times(9)+timer()
	   print *,'Time for Finish     = ',times(9)
      endif
      call tmk_exit(0)
      STOP
      ENDIF
C
C     TIME SMOOTHING AND UPDATE FOR NEXT CYCLE
C

      IF(NCYCLE .LE. 1) THEN
	   times(7)=times(7)-timer()
         CALL CALC3Z
	   times(7)=times(7)+timer()
      ELSE
	   times(8)=times(8)-timer()
         CALL CALC3
	   times(8)=times(8)+timer()
      ENDIF
      CALL tmk_barrier(0)
C
      GO TO 90
      END
      SUBROUTINE INITAL
C        INITIALIZE CONSTANTS AND ARRAYS
C           R. K. SATO 4/7/86
C
      INCLUDE 'Tmk_fortran.h'
C
      PARAMETER (N1=1024, N2=256, N3=1032)
      COMMON  /Tmk_shared_common/ U(N1,N2),t1(N3),
     1        V(N1,N2),t2(N3),
     2        P(N1,N2),t3(N3),CU(N1,N2),t4(N3),CV(N1,N2),t5(N3),
     3        Z(N1,N2),t6(N3),H(N1,N2),
     4        t7(N3),UNEW(N1,N2),t8(N3),VNEW(N1,N2),t9(N3),PNEW(N1,N2),
     5        t10(N3),UOLD(N1,N2),t11(N3),VOLD(N1,N2),
     6	      t12(N3),POLD(N1,N2),t13(N3),PSI(N1,N2)
C
      COMMON /CONS/ DT,TDT,DX,DY,A,ALPHA,ITMAX,MPRINT,M,N,MP1,
     1              NP1,EL,PI,TPI,DI,DJ,PCF,
     2		    NBEG,NEND,NBEGACT,NENDACT
C
C     NOTE BELOW THAT TWO DELTA T (TDT) IS SET TO DT ON THE FIRST
C     CYCLE AFTER WHICH IT IS RESET TO DT+DT.
C
C
C
C
C
C

      DT = 20.
      TDT = DT
C
      DX = .25E5
      DY = .25E5
      A = 1.E6
      ALPHA = .001
      ITMAX = 50
      MPRINT = 50
c     ITMAX = 10
c     MPRINT = 10
      M = N1 - 1
      N = N2 - 1
      NBEG = (Tmk_proc_id*(N2-2))/Tmk_nprocs + 2
      NEND = ((Tmk_proc_id + 1)*(N2-2))/Tmk_nprocs + 1
      NBEGACT = NBEG
      NENDACT = NEND
      if (Tmk_proc_id.eq.0) then 
	NBEGACT = NBEG-1
      endif
      if (Tmk_proc_id.eq.(Tmk_nprocs-1)) then
	NENDACT = NEND+1
      endif
      MP1 = M+1
      NP1 = N+1
      EL = FLOAT(N)*DX
      PI = 4.*ATAN(1.)
      TPI = PI+PI
      DI = TPI/FLOAT(M)
      DJ = TPI/FLOAT(N)
      PCF = PI*PI*A*A/(EL*EL)
C
C     INITIAL VALUES OF THE STREAM FUNCTION AND P
C
CAPR$ DO PAR ON PSI
      DO 50 J=NBEGACT,NENDACT
      DO 50 I=1,MP1
      PSI(I,J) = A*SIN((FLOAT(I)-.5)*DI)*SIN((FLOAT(J)-.5)*DJ)
      P(I,J) = PCF*(COS(2.*FLOAT(I-1)*DI)
     1                +COS(2.*FLOAT(J-1)*DJ))+50000.
   50 CONTINUE
C
C     INITIALIZE VELOCITIES
C
      CALL tmk_barrier(0)
CAPR$ DO PAR ON PSI<:,2~1>
      DO 60 J=NBEGACT,NEND
      DO 60 I=1,M
      U(I+1,J) = -(PSI(I+1,J+1)-PSI(I+1,J))/DY
      V(I,J+1) = (PSI(I+1,J+1)-PSI(I,J+1))/DX
   60 CONTINUE
C
C     PERIODIC CONTINUATION
C
CAPR$ DO PAR ON U<:,1~1>
      DO 70 J=NBEGACT,NEND
      U(1,J) = U(M+1,J)
      V(M+1,J+1) = V(1,J+1)
   70 CONTINUE
      CALL tmk_barrier(0)
c gw CAPR$ DO PAR ON U<:,1+N>
      if (Tmk_proc_id.eq.0) then
      DO 75 I=1,M
      U(I+1,N+1) = U(I+1,1)
      V(I,1) = V(I,N+1)
   75 CONTINUE
      U(1,N+1) = U(M+1,1)
      V(M+1,1) = V(1,N+1)
      endif
CAPR$ DO PAR ON P<:,1~1>
      DO 86 J=NBEGACT,NENDACT
      DO 86 I=1,MP1
      UOLD(I,J) = U(I,J)
      VOLD(I,J) = V(I,J)
      POLD(I,J) = P(I,J)
   86 CONTINUE
C        END OF INITIALIZATION
      RETURN
      END
CMIC$ MICRO
      SUBROUTINE CALC1
C
C        COMPUTE CAPITAL  U, CAPITAL V, Z AND H
C
      PARAMETER (N1=1024, N2=256, N3=1032)
      COMMON  /Tmk_shared_common/ U(N1,N2),t1(N3),
     1        V(N1,N2),t2(N3),
     2        P(N1,N2),t3(N3),CU(N1,N2),t4(N3),CV(N1,N2),t5(N3),
     3        Z(N1,N2),t6(N3),H(N1,N2),
     4        t7(N3),UNEW(N1,N2),t8(N3),VNEW(N1,N2),t9(N3),PNEW(N1,N2),
     5        t10(N3),UOLD(N1,N2),t11(N3),VOLD(N1,N2),
     6	      t12(N3),POLD(N1,N2),t13(N3),PSI(N1,N2)
C
      COMMON /SHTIME/ T100, T200, T300
      real *8 timer
      real *8 t100,t200,t300,tbeg,tend
      COMMON /CONS/ DT,TDT,DX,DY,A,ALPHA,ITMAX,MPRINT,M,N,MP1,
     1              NP1,EL,PI,TPI,DI,DJ,PCF,
     2		    NBEG,NEND,NBEGACT,NENDACT
C
C
C
C
C
      FSDX = 4./DX
      FSDY = 4./DY
      TBEG = TIMER()
CMIC$ DO GLOBAL
CAPR$ DO PAR ON CU<:,1~1>
      DO 100 J=NBEGACT,NEND
      DO 100 I=1,M
      CU(I+1,J) = .5*(P(I+1,J)+P(I,J))*U(I+1,J)
c     CV(I,J+1) = .5*(P(I,J+1)+P(I,J))*V(I,J+1)
c     Z(I+1,J+1) = (FSDX*(V(I+1,J+1)-V(I,J+1))-FSDY*(U(I+1,J+1)
c    1          -U(I+1,J)))/(P(I,J)+P(I+1,J)+P(I+1,J+1)+P(I,J+1))
      H(I,J) = P(I,J)+.25*(U(I+1,J)*U(I+1,J)+U(I,J)*U(I,J)
     1               +V(I,J+1)*V(I,J+1)+V(I,J)*V(I,J))
  100 CONTINUE
CAPR$ DO PAR ON CV<:,2~1>
      DO 102 J=NBEGACT,NEND
      DO 102 I=1,M
c     CU(I+1,J) = .5*(P(I+1,J)+P(I,J))*U(I+1,J)
      CV(I,J+1) = .5*(P(I,J+1)+P(I,J))*V(I,J+1)
      Z(I+1,J+1) = (FSDX*(V(I+1,J+1)-V(I,J+1))-FSDY*(U(I+1,J+1)
     1          -U(I+1,J)))/(P(I,J)+P(I+1,J)+P(I+1,J+1)+P(I,J+1))
c     H(I,J) = P(I,J)+.25*(U(I+1,J)*U(I+1,J)+U(I,J)*U(I,J)
c    1               +V(I,J+1)*V(I,J+1)+V(I,J)*V(I,J))
  102 CONTINUE
      TEND = TIMER()
      T100 = TEND -  TBEG
C
C     PERIODIC CONTINUATION
C
CAPR$ DO PAR ON CU<:,1~1>
      DO 110 J=NBEGACT,NEND
      CU(1,J) = CU(M+1,J)
c     CV(M+1,J+1) = CV(1,J+1)
c     Z(1,J+1) = Z(M+1,J+1)
      H(M+1,J) = H(1,J)
  110 CONTINUE
CAPR$ DO PAR ON CV<:,2~1>
      DO 112 J=NBEGACT,NEND
c     CU(1,J) = CU(M+1,J)
      CV(M+1,J+1) = CV(1,J+1)
      Z(1,J+1) = Z(M+1,J+1)
c     H(M+1,J) = H(1,J)
  112 CONTINUE
      CALL tmk_barrier(0)
      if (NBEGACT .eq. 1) then
c gw CAPR$ DO PAR ON CV<:,1>
      DO 115 I=1,M
c     CU(I+1,N+1) = CU(I+1,1)
      CV(I,1) = CV(I,N+1)
      Z(I+1,1) = Z(I+1,N+1)
c     H(I,N+1) = H(I,1)
  115 CONTINUE
      CV(M+1,1) = CV(1,N+1)
      Z(1,1) = Z(M+1,N+1)
      endif
      if (NENDACT .eq. NP1) then
c gw CAPR$ DO PAR ON CU<:,1+N>
      DO 117 I=1,M
      CU(I+1,N+1) = CU(I+1,1)
c     CV(I,1) = CV(I,N+1)
c     Z(I+1,1) = Z(I+1,N+1)
      H(I,N+1) = H(I,1)
  117 CONTINUE
      CU(1,N+1) = CU(M+1,1)
      H(M+1,N+1) = H(1,1)
      endif
C        END OF LOOP 100 CALCULATIONS
      RETURN
      END
CMIC$ MICRO
      SUBROUTINE CALC2
C
C        COMPUTE NEW VALUES OF U,V,P
C
      PARAMETER (N1=1024, N2=256, N3=1032)
      COMMON  /Tmk_shared_common/ U(N1,N2),t1(N3),
     1        V(N1,N2),t2(N3),
     2        P(N1,N2),t3(N3),CU(N1,N2),t4(N3),CV(N1,N2),t5(N3),
     3        Z(N1,N2),t6(N3),H(N1,N2),
     4        t7(N3),UNEW(N1,N2),t8(N3),VNEW(N1,N2),t9(N3),PNEW(N1,N2),
     5        t10(N3),UOLD(N1,N2),t11(N3),VOLD(N1,N2),
     6	      t12(N3),POLD(N1,N2),t13(N3),PSI(N1,N2)
C
      COMMON /SHTIME/ T100, T200, T300
      real *8 timer
      real *8 t100,t200,t300,tbeg,tend
      COMMON /CONS/ DT,TDT,DX,DY,A,ALPHA,ITMAX,MPRINT,M,N,MP1,
     1              NP1,EL,PI,TPI,DI,DJ,PCF,
     2		    NBEG,NEND,NBEGACT,NENDACT
C
C
C
C
      TDTS8 = TDT/8.
      TDTSDX = TDT/DX
      TDTSDY = TDT/DY
      TBEG = TIMER()
CMIC$ DO GLOBAL
CAPR$ DO PAR ON UNEW<:,1~1>
      DO 200 J=NBEGACT,NEND
      DO 200 I=1,M
      UNEW(I+1,J) = UOLD(I+1,J)+
     1    TDTS8*(Z(I+1,J+1)+Z(I+1,J))*(CV(I+1,J+1)+CV(I,J+1)+CV(I,J)
     2       +CV(I+1,J))-TDTSDX*(H(I+1,J)-H(I,J))
c     VNEW(I,J+1) = VOLD(I,J+1)-TDTS8*(Z(I+1,J+1)+Z(I,J+1))
c    1       *(CU(I+1,J+1)+CU(I,J+1)+CU(I,J)+CU(I+1,J))
c    2       -TDTSDY*(H(I,J+1)-H(I,J))
      PNEW(I,J) = POLD(I,J)-TDTSDX*(CU(I+1,J)-CU(I,J))
     1       -TDTSDY*(CV(I,J+1)-CV(I,J))
  200 CONTINUE
CAPR$ DO PAR ON VNEW<:,2~1>
      DO 202 J=NBEGACT,NEND
      DO 202 I=1,M
c     UNEW(I+1,J) = UOLD(I+1,J)+
c    1    TDTS8*(Z(I+1,J+1)+Z(I+1,J))*(CV(I+1,J+1)+CV(I,J+1)+CV(I,J)
c    2       +CV(I+1,J))-TDTSDX*(H(I+1,J)-H(I,J))
      VNEW(I,J+1) = VOLD(I,J+1)-TDTS8*(Z(I+1,J+1)+Z(I,J+1))
     1       *(CU(I+1,J+1)+CU(I,J+1)+CU(I,J)+CU(I+1,J))
     2       -TDTSDY*(H(I,J+1)-H(I,J))
c     PNEW(I,J) = POLD(I,J)-TDTSDX*(CU(I+1,J)-CU(I,J))
c    1       -TDTSDY*(CV(I,J+1)-CV(I,J))
  202 CONTINUE
      TEND = TIMER()
      T200 = TEND - TBEG
C
C     PERIODIC CONTINUATION
C
CAPR$ DO PAR ON UNEW<:,1~1>
      DO 210 J=NBEGACT,NEND
      UNEW(1,J) = UNEW(M+1,J)
c     VNEW(M+1,J+1) = VNEW(1,J+1)
      PNEW(M+1,J) = PNEW(1,J)
  210 CONTINUE
CAPR$ DO PAR ON VNEW<:,2~1>
      DO 212 J=NBEGACT,NEND
c     UNEW(1,J) = UNEW(M+1,J)
      VNEW(M+1,J+1) = VNEW(1,J+1)
c     PNEW(M+1,J) = PNEW(1,J)
  212 CONTINUE
      CALL tmk_barrier(0)
c gw CAPR$ DO PAR ON VNEW<:,1>
      if (NBEGACT .eq. 1) then
      DO 215 I=1,M
c     UNEW(I+1,N+1) = UNEW(I+1,1)
      VNEW(I,1) = VNEW(I,N+1)
c     PNEW(I,N+1) = PNEW(I,1)
  215 CONTINUE
      VNEW(M+1,1) = VNEW(1,N+1)
      endif
c gw CAPR$ DO PAR ON UNEW<:,1+N>
      if (NENDACT .eq. NP1) then
      DO 217 I=1,M
      UNEW(I+1,N+1) = UNEW(I+1,1)
c     VNEW(I,1) = VNEW(I,N+1)
      PNEW(I,N+1) = PNEW(I,1)
  217 CONTINUE
      UNEW(1,N+1) = UNEW(M+1,1)
      PNEW(M+1,N+1) = PNEW(1,1)
      endif
C
      RETURN
      END
      SUBROUTINE CALC3Z
C
C         TIME SMOOTHER FOR FIRST ITERATION
C
      PARAMETER (N1=1024, N2=256, N3=1032)
      COMMON  /Tmk_shared_common/ U(N1,N2),t1(N3),
     1        V(N1,N2),t2(N3),
     2        P(N1,N2),t3(N3),CU(N1,N2),t4(N3),CV(N1,N2),t5(N3),
     3        Z(N1,N2),t6(N3),H(N1,N2),
     4        t7(N3),UNEW(N1,N2),t8(N3),VNEW(N1,N2),t9(N3),PNEW(N1,N2),
     5        t10(N3),UOLD(N1,N2),t11(N3),VOLD(N1,N2),
     6	      t12(N3),POLD(N1,N2),t13(N3),PSI(N1,N2)
C
      COMMON /CONS/ DT,TDT,DX,DY,A,ALPHA,ITMAX,MPRINT,M,N,MP1,
     1              NP1,EL,PI,TPI,DI,DJ,PCF,
     2		    NBEG,NEND,NBEGACT,NENDACT
C
C
C
C
C
  310 TDT = TDT+TDT
CAPR$ DO PAR ON P<:,1~1>
      DO 400 J=NBEGACT,NENDACT
      DO 400 I=1,MP1
      UOLD(I,J) = U(I,J)
      VOLD(I,J) = V(I,J)
      POLD(I,J) = P(I,J)
      U(I,J) = UNEW(I,J)
      V(I,J) = VNEW(I,J)
      P(I,J) = PNEW(I,J)
  400 CONTINUE
      RETURN
      END
CMIC$ MICRO
      SUBROUTINE CALC3
C
C         TIME SMOOTHER
C
      PARAMETER (N1=1024, N2=256, N3=1032)
      COMMON  /Tmk_shared_common/ U(N1,N2),t1(N3),
     1        V(N1,N2),t2(N3),
     2        P(N1,N2),t3(N3),CU(N1,N2),t4(N3),CV(N1,N2),t5(N3),
     3        Z(N1,N2),t6(N3),H(N1,N2),
     4        t7(N3),UNEW(N1,N2),t8(N3),VNEW(N1,N2),t9(N3),PNEW(N1,N2),
     5        t10(N3),UOLD(N1,N2),t11(N3),VOLD(N1,N2),
     6	      t12(N3),POLD(N1,N2),t13(N3),PSI(N1,N2)
C
      COMMON /SHTIME/ T100, T200, T300
      real *8 timer
      real *8 t100,t200,t300,tbeg,tend
      COMMON /CONS/ DT,TDT,DX,DY,A,ALPHA,ITMAX,MPRINT,M,N,MP1,
     1              NP1,EL,PI,TPI,DI,DJ,PCF,
     2		    NBEG,NEND,NBEGACT,NENDACT
C
C
C
C
C
C        TIME SMOOTHING AND UPDATE FOR NEXT CYCLE
C
      TBEG = TIMER()
CMIC$ DO GLOBAL
CAPR$ DO PAR ON UOLD<:,1~1>
      DO 300 J=NBEGACT,NEND
      DO 300 I=1,M
      UOLD(I,J) = U(I,J)+ALPHA*(UNEW(I,J)-2.*U(I,J)+UOLD(I,J))
      VOLD(I,J) = V(I,J)+ALPHA*(VNEW(I,J)-2.*V(I,J)+VOLD(I,J))
      POLD(I,J) = P(I,J)+ALPHA*(PNEW(I,J)-2.*P(I,J)+POLD(I,J))
      U(I,J) = UNEW(I,J)
      V(I,J) = VNEW(I,J)
      P(I,J) = PNEW(I,J)
  300 CONTINUE
      TEND = TIMER()
      T300 = TEND - TBEG
C
C     PERIODIC CONTINUATION
C
CAPR$ DO PAR ON UOLD<:,1~1>
      DO 320 J=NBEGACT,NEND
      UOLD(M+1,J) = UOLD(1,J)
      VOLD(M+1,J) = VOLD(1,J)
      POLD(M+1,J) = POLD(1,J)
      U(M+1,J) = U(1,J)
      V(M+1,J) = V(1,J)
      P(M+1,J) = P(1,J)
  320 CONTINUE
      CALL tmk_barrier(0)
c gw CAPR$ DO PAR ON UOLD<:,1+N>
      if (NENDACT .eq. NP1) then
      DO 325 I=1,M
      UOLD(I,N+1) = UOLD(I,1)
      VOLD(I,N+1) = VOLD(I,1)
      POLD(I,N+1) = POLD(I,1)
      U(I,N+1) = U(I,1)
      V(I,N+1) = V(I,1)
      P(I,N+1) = P(I,1)
  325 CONTINUE
      UOLD(M+1,N+1) = UOLD(1,1)
      VOLD(M+1,N+1) = VOLD(1,1)
      POLD(M+1,N+1) = POLD(1,1)
      U(M+1,N+1) = U(1,1)
      V(M+1,N+1) = V(1,1)
      P(M+1,N+1) = P(1,1)
      endif
C
      RETURN
      END
      SUBROUTINE TIME0
C   Subroutine DATETM returns the date, time,  virtual cpu time, total
C   cpu time, and connect time.
C         CALL DATETM(DATTIM, ECPU, ETIME, ETCPU)
C   DATTIM is an array or character string of at least 23 characters in
C          length, containing the current time and date in the format:
C                  'DAY, MON DA YEAR  HR:MN'
C                  'TUE, JAN 01 1981  16:53'
C   ECPU   is elapsed virtual cpu time in seconds, since logon or since
C          the last accounting. Resolution is in hundredths of a second.
C   ETIME  is elapsed connect in seconds, since logon or since the last
C          accounting.  Resolution is in seconds.
C   ETCPU  is total cpu time, since logon or since last accounting. The
C          resolution is in hundredths of a second.
C
C        NEXT FOUR LINES ARE FOR AN IBM SYSTEM
C     CHARACTER*23 DATTIM
C     CALL DATETM (DATTIM, TBEG,ETIME,ETCPU)
C   print the date and wall clock time.
C     PRINT *,DATTIM
C       WALL CLOCK CALLS FOR CRAY XMP
c     IDATE = DATE()
c     ITIME = CLOCK()
c     WRITE(6,9010) IDATE, ITIME
c9010  FORMAT(//,10X, 2A15,/)
c       wall clock calls for the SUN 4
c     character*24 dattim
c     call fdate(dattim)
c     write(*,*) dattim
c     write(6,9010)
c9010  format(/'  insert date and time routine.')
      RETURN
      END
