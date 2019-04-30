# SatOrbitParallel
Parallelize  Satellite Orbital Collision/Link Detection

Parallel Code in SatOrbitACC.cpp
Old code developed for serial and prior to OpenACC optimization in mainSatOrbit.cpp

Run instructions:
>make
>./get_gpu_node.sh
>./submit_gpu.sh

This will run the OpenACC code and produce results in a QRLOGIN file

Also, possible to run it via ./SatOrbitACC

It is possible to run the serial version of the code with ./SatOrbitSerial

Also, the old code not-optimized for parallel can be found in mainSatOrbit.cpp