#!/bin/bash

BUILDROOT_PATH="$(dirname $0)/buildroot"

cd ${BUILDROOT_PATH}
make distclean
cd -
