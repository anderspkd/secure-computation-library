#!/usr/bin/bash

if ! command -v bear &>/dev/null; then
    echo "Cannot generate compile_commands.json because bear is missing"
    exit 1
fi

if ! [ -f CMakeLists.txt ]; then
    echo "No CMakeLists.txt found"
    echo "Run this script from the project root"
    exit 1
fi

if [ $# -eq "0" ] || ! [ -d $1 ]; then
    echo "Usage: $0 [path_to_debug_build_directory]"
    exit 1
fi

build_dir="${1}"
project_root=$(pwd)

build_compile_commands () {
    cd "${build_dir}"
    make -s clean
    bear -- make -s -j4
    cd $project_root
}

build_compile_commands
cp "${build_dir}compile_commands.json" .
