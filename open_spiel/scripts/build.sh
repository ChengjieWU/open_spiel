#!/bin/bash

# Copyright 2019 DeepMind Technologies Ltd. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# The following builds open_spiel and executes the tests using the `python`
# command. The version under 'python' is automatically detected.
#
# It assumes:
#   - we are at the root of the project
#   - `install.sh` has been run
#   - the Python dependencies has been installed, e.g. using
#     `pip install --upgrade -r ../requirements.txt`.
#
# Using a virtualenv is recommended but not mandatory.
#
set -e  # exit when any command fails
set -x

MYDIR="$(dirname "$(realpath "$0")")"
source "${MYDIR}/global_variables.sh"

CXX=`which clang++`
if [ ! -x $CXX ]
then
  echo -n "clang++ not found in the path (the clang C++ compiler is needed to "
  echo "compile OpenSpiel). Exiting..."
  exit 1
fi

NPROC=nproc
if [[ "$OSTYPE" == "darwin"* ]]; then  # Mac OSX
  NPROC="sysctl -n hw.physicalcpu"
fi

MAKE_NUM_PROCS=$(${NPROC})
let TEST_NUM_PROCS=4*${MAKE_NUM_PROCS}

PYVERSION=$(python3 -c 'import sys; print(sys.version.split(" ")[0])')

BUILD_DIR="build"
mkdir -p $BUILD_DIR
cd $BUILD_DIR

echo "Building and testing in $PWD using 'python' (version $PYVERSION)."

cmake -DPython_TARGET_VERSION=${PYVERSION} -DCMAKE_CXX_COMPILER=${CXX} ../open_spiel
make -j$MAKE_NUM_PROCS
