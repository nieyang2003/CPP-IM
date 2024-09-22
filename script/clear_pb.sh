#!/bin/bash
CUR_DIR=$(dirname $0)
SOURCE_PATH="$CUR_DIR/../server"

rm -f $SOURCE_PATH/login/pkg/grpc/*.pb.go
rm -f $SOURCE_PATH/varify/*.pb.h
rm -f $SOURCE_PATH/varify/*.pb.cc
rm -f $SOURCE_PATH/route/*.pb.h
rm -f $SOURCE_PATH/route/*.pb.cc
rm -f $SOURCE_PATH/msg_gate/*.pb.h
rm -f $SOURCE_PATH/msg_gate/*.pb.cc
rm -f $SOURCE_PATH/msg_logic/*.pb.h
rm -f $SOURCE_PATH/msg_logic/*.pb.cc
rm -f $SOURCE_PATH/transfer/*.pb.h
rm -f $SOURCE_PATH/transfer/*.pb.cc

echo "OK"