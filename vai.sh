#!/bin/bash
echo "usage vai.sh -B|V|N -0|1 -M|S|L"
echo "-B[only build]: build"
echo "-V[only view]: monitor"
echo "-N[new]: build app-flash monitor"
echo "-0=/dev/ttyUSB0 ; -1=/dev/ttyUSB1"
echo "-M= master ; -S=Slave; -L=Mobile"
PORTA_="-p/dev/ttyUSB0"

#default is STATION_MASTER
STATION_="#define DEVOPS_THIS_IS_STATION_MASTER"

while getopts BVN01MSL option; do
case ${option}
in
0)  PORTA_="-p/dev/ttyUSB0";;
1)  PORTA_="-p/dev/ttyUSB1";;
B)  BUILD1_="build";;
V)  BUILD1_="monitor";;
N)  BUILD1_="build"; BUILD2_="app-flash"; BUILD3_="monitor";;
M)  STATION_="#define DEVOPS_THIS_IS_STATION_MASTER";;
S)  STATION_="#define DEVOPS_THIS_IS_STATION_SLAVE";;
L)  STATION_="#define DEVOPS_THIS_IS_STATION_MOBILE";;
esac
done

echo "BUILD1: ${BUILD1_} BUILD2: ${BUILD2_} BUILD3: ${BUILD3_}"
echo "PORT: ${PORTA_}"
echo "STATION: ${STATION_}"

echo ${STATION_} > "./components/eventengine/include/tipostazione.h"
CMD_="/home/ccattaneo/esp/esp-idf/tools/idf.py"

if [ ${#BUILD2_} -gt 0 ] #app-flash
then
    #echo "Caso1"
    echo "${CMD_}" "${PORTA_}" "${BUILD1_}" "${BUILD2_}" "${BUILD3_}"
    ("${CMD_}" "${PORTA_}" "${BUILD1_}" "${BUILD2_}" "${BUILD3_}")
    exit 0
fi

if [ ${#BUILD1_} -gt 5 ] #monitor
then
    #echo "Caso2"
    echo "${CMD_}" "${PORTA_}" "${BUILD1_}"
    ("${CMD_}" "${PORTA_}" "${BUILD1_}")
    exit 0
fi

echo "Caso3"
echo "${CMD_}" "${BUILD1_}"
("${CMD_}" "${BUILD1_}")
exit 0