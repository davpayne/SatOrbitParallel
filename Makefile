EXECS= mainSatOrbit SatOrbitACC
CCX = g++ # g++
CCXFLAGS = -g O2 
ACCEL_TYPE = PGI-tesla
ACCX = pgc++
ACCXFLAGS = -DUSE_DOUBLE -Minfo=accel -fast -acc -ta=tesla:cc60

all: ${EXECS}

SatOrbit: mainSatOrbit.cpp
	${CCX} ${CCXFLAGS} -o mainSatOrbit mainSatOrbit.cpp

SatOrbitACC: SatOrbitACC.cpp
	${ACCX} ${ACCXFLAGS} -o SatOrbitACC SatOrbitACC.cpp 
# mainSatOrbit.cpp mainSatOrbit.o -o $@
clean:
	rm -f ${EXECS}