#!/bin/bash
echo "usage vai.sh -B|V|N -0|1 -M|S|L"
echo "-B[only build]: build"
echo "-V[only view]: monitor"
echo "-N[new]: build app-flash monitor"
echo "-0=/dev/ttyUSB0 ; -1=/dev/ttyUSB1"
echo "-M= master ; -S=Slave; -L=Mobile"
PORTA=""

while getopts BVN01MSL option; do
case ${option}
in
0)  PORTA="-p/dev/ttyUSB0";;
1)  PORTA="-p/dev/ttyUSB1";;
B)  BUILD1="build"; BUILD2=""; BUILD3=""; PORTA="";;
V)  BUILD1="monitor"; BUILD2=""; BUILD3="";;
N)  BUILD1="build"; BUILD2="app-flash"; BUILD3="monitor";;
M)  STATION="#define DEVOPS_THIS_IS_STATION_MASTER";;
S)  STATION="#define DEVOPS_THIS_IS_STATION_SLAVE";;
L)  STATION="#define DEVOPS_THIS_IS_STATION_MOBILE";;
esac
done


echo "BUILD1: ${BUILD1} BUILD2: ${BUILD2} BUILD3: ${BUILD3}"
echo "PORT: ${PORTA}"
echo "STATION: ${STATION}"

echo ${STATION} > "./components/eventengine/include/tipostazione.h"

CMD="/home/ccattaneo/esp/esp-idf/tools/idf.py"
echo "${CMD}" "${PORTA}" "${BUILD1}" "${BUILD2}" "${BUILD3}"
"${CMD}" "${PORTA}" "${BUILD1}" "${BUILD2}" "${BUILD3}"
