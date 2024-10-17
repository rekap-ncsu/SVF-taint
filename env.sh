#!/bin/bash

# set the

set -e

PROJECTHOME=$(pwd)
sysOS=`uname -s`
MajorLLVMVer=16
LLVMVer=${MajorLLVMVer}.0.0


LLVMHome="llvm-${LLVMVer}.obj"
Z3Home="z3.obj"

export SVF_DIR="/home/jpangia/research/SVF-fork"
export LLVM_DIR=$SVF_DIR/$LLVMHome
export Z3_DIR=$SVF_DIR/$Z3Home

export PATH=$SVF_DIR/Release-build/bin:$PATH
export PATH=$LLVM_DIR/bin:$PATH
export PATH=$PROJECTHOME/bin:$PATH

echo "LLVM_DIR="$LLVM_DIR
echo "SVF_DIR="$SVF_DIR
echo "Z3_DIR="$Z3_DIR
