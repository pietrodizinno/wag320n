#!/bin/sh
. Rules.mk
mkdir -m 777 -p kernel
rm -f kernel/bcmdriver
rm -f kernel/src
rm -f env
ln -sf $BCMBSP_DIR/env ./env
ln -sf ../$BCMBSP_DIR/bcmdrivers kernel/bcmdriver
ln -sf ../$BCMBSP_DIR/kernel/linux kernel/src
