EXECS= mainSatOrbit
CCX = g++ # pgcc
CCXFLAGS = -g O2 #-DUSE_DOUBLE
#ACCEL_TYPE = PGI-tesla

all: ${EXECS}

SatOrbit: mainSatOrbit.cpp
	${CCX} ${CCXFLAGS} -o mainSatOrbit mainSatOrbit.cpp

clean:
	rm -f ${EXECS}