#!/bin/bash
GRPC_CPP_PLUGIN=$(which grpc_cpp_plugin)
if [ -z "$GRPC_CPP_PLUGIN" ]; then
	echo "can not found `grpc_cpp_plugin`"
	exit 1
fi

CUR_DIR=$(dirname $0)
SOURCE_PATH="$CUR_DIR/../server"

PTOTO_PATH="$SOURCE_PATH/proto"
GO_OUT_PATH="$SOURCE_PATH/login/pkg/grpc"

# go
protoc --proto_path=$PTOTO_PATH --go_out=$GO_OUT_PATH --go-grpc_out=$GO_OUT_PATH $PTOTO_PATH/varify.proto
protoc --proto_path=$PTOTO_PATH --go_out=$GO_OUT_PATH --go-grpc_out=$GO_OUT_PATH $PTOTO_PATH/route.proto

# cpp
# cpp
PROTO_FILE="msg push route varify"
for file in $PROTO_FILE; do
	protoc --proto_path=$PTOTO_PATH --cpp_out=$PTOTO_PATH $PTOTO_PATH/$file.proto
	protoc --proto_path=$PTOTO_PATH --grpc_out=$PTOTO_PATH --plugin=protoc-gen-grpc=$GRPC_CPP_PLUGIN $PTOTO_PATH/$file.proto
done

# qt
# protoc --proto_path=C:\Users\Admin\Desktop\test --plugin=protoc-gen-qtprotobuf=C:\Qt\6.7.2\mingw_64\bin\qtprotobufgen.exe --qtprotobuf_out=C:\Users\Admin\Desktop\test C:\Users\Admin\Desktop\test\msg.proto
# 如果生成的文件有bug，需要手动替换
# #include "google/protobuf/empty.qpb.h"
# #include <QtProtobufWellKnownTypes/empty.qpb.h>

echo "OK"