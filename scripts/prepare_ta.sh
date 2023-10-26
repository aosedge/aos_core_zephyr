#!/bin/bash

while getopts s:g:b:f:h flag
do
    case "${flag}" in
        s)
            SOURCE_DIR=${OPTARG}
            ;;
        g)
            GEN_DIR=${OPTARG}
            ;;
        b)
            TA_DIR=${OPTARG}
        ;;
        f)
            FROM=${OPTARG}
        ;;
        h)
            echo "options:"
            echo " -t - application source top dir"
            echo " -f - ta binary source dir"
            echo " -b - directory with ta binaries in app src"
            echo " -g - directory with generated files"
            exit
            ;;
    esac
done

[ -d $TA_DIR ] || mkdir $TA_DIR
[ -d $GEN_DIR ] || mkdir $GEN_DIR

rm -rf $TA_DIR/*

cp $SOURCE_DIR/$FROM/*.ta $TA_DIR
$SOURCE_DIR/scripts/gen_ta_src.py -s $TA_DIR -p -o $GEN_DIR/ta_table
