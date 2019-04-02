EXECS= mainSatOrbit
CCX = g++
CCXFLAGS = -g O2

all: ${EXECS}

SatOrbit: mainSatOrbit.cpp
	${CCX} ${CCXFLAGS} -o mainSatOrbit mainSatOrbit.cpp

clean:
	rm -f ${EXECS}