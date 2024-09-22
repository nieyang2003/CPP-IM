#!/bin/bash
CUR_DIR=$(dirname $0)
SOURCE_PATH="$CUR_DIR/../server"

# go
rm $SOURCE_PATH/login/pkg/grpc/*.pb.go
# cpp
rm $SOURCE_PATH/proto/*.pb.h
rm $SOURCE_PATH/proto/*.pb.cc
# Qt
