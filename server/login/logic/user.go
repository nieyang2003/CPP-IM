package logic

import (
	"context"
	pb "gate_server/pkg/grpc"
	"time"

	"go.uber.org/zap"
)

func GetVerifyCode(email string, send bool) error {
	// 获取
	client := pb.GetVarifyClientPool().Get()
	defer pb.GetVarifyClientPool().Put(client)
	// 创建一个带有超时的 context
	ctx, cancel := context.WithTimeout(context.Background(), 3*time.Second)
	defer cancel()
	// 发送
	_, err := client.GetVarifyCode(ctx, &pb.GetVarifyCodeRequest{Email: email, IfSend: send})
	if err != nil {
		zap.L().Error("发送验证码失败", zap.Error(err))
	}
	return err
}

func DispatchMsgServer(uid uint64, ip string) (*pb.DispatchMsgServerResponse, error) {
	client := pb.GetRouteClientPool().Get()
	defer pb.GetRouteClientPool().Put(client)
	// 创建一个带有超时的 context
	ctx, cancel := context.WithTimeout(context.Background(), 3*time.Second)
	defer cancel()
	resp, err := client.DispatchMsgServer(ctx, &pb.DispatchMsgServerRequest{Uid: uid, Ip: ip})
	return resp, err
}
