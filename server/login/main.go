package main

import (
	"fmt"
	"gate_server/dao/mysql"
	"gate_server/dao/redis"
	"gate_server/logger"
	pb "gate_server/pkg/grpc"
	"gate_server/pkg/snowflake"
	"gate_server/routers"
	"gate_server/settings"

	"go.uber.org/zap"
)

func main() {
	// 读取配置
	if err := settings.Init(); err != nil {
		fmt.Printf("load config failed, err:%v\n", err)
		return
	}
	// 初始化日志
	if err := logger.Init(settings.Conf.LogConfig, settings.Conf.Mode); err != nil {
		fmt.Printf("init logger failed, err:%v\n", err)
		return
	}
	zap.L().Info("init logger success")
	// 验证码服务
	if err := pb.InitVarifyClientPool(settings.Conf.VarifyServerConfig); err != nil {
		fmt.Printf("init varify service failed, err:%v\n", err)
		return
	}
	defer pb.GetVarifyClientPool().Close()
	zap.L().Info("init varify service success")
	// 分发服务
	if err := pb.InitRouteClientPool(settings.Conf.RouteServerConfig); err != nil {
		fmt.Printf("init route service failed, err:%v\n", err)
		return
	}
	defer pb.GetRouteClientPool().Close()
	zap.L().Info("init route service success")
	// snowflake
	if err := snowflake.Init(uint16(settings.Conf.SnowFlakeConfig.MachineID)); err != nil {
		fmt.Printf("init snowflake failed, err:%v\n", err)
	}
	zap.L().Info("init snowflake success")
	// Redis
	if err := redis.Init(settings.Conf.RedisConfig); err != nil {
		fmt.Printf("init redis failed, err:%v\n", err)
		return
	}
	zap.L().Info("init redis success")
	// MySQL
	if err := mysql.Init(settings.Conf.MySQLConfig); err != nil {
		fmt.Printf("init mysql failed, err:%v\n", err)
		return
	}
	defer mysql.Close() // 程序退出关闭数据库连接
	zap.L().Info("init mysql success")
	// 启动
	r := routers.SetupRouter()
	err := r.Run(fmt.Sprintf(":%d", settings.Conf.Port))
	if err != nil {
		fmt.Printf("run server failed, err:%v\n", err)
		return
	}
}
