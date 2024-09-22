package settings

import (
	"fmt"

	"github.com/fsnotify/fsnotify"
	"github.com/spf13/viper"
)

// 全局变量，保存应用程序的配置信息
var Conf = new(AppConfig)

// 映射配置文件中的相关配置信息
type AppConfig struct {
	Mode                string `mapstructure:"mode"`
	Port                int    `mapstructure:"port"`
	*VarifyServerConfig `mapstructure:"varify_server"`
	*RouteServerConfig  `mapstructure:"route_server"`
	*LogConfig          `mapstructure:"log"`
	*RedisConfig        `mapstructure:"redis"`
	*MySQLConfig        `mapstructure:"mysql"`
	*SnowFlakeConfig    `mapstructure:"snowflake"`
}

type VarifyServerConfig struct {
	Target     string `mapstructure:"target"`
	MaxClients int    `mapstructure:"max_clients"`
}

type RouteServerConfig struct {
	Target     string `mapstructure:"target"`
	MaxClients int    `mapstructure:"max_clients"`
}

// 映射配置文件中的日志
type LogConfig struct {
	Level      string `mapstructure:"level"`
	Filename   string `mapstructure:"filename"`
	MaxSize    int    `mapstructure:"max_size"`
	MaxAge     int    `mapstructure:"max_age"`
	MaxBackups int    `mapstructure:"max_backups"`
}

type RedisConfig struct {
	Host         string `mapstructure:"host"`
	Password     string `mapstructure:"password"`
	Port         int    `mapstructure:"port"`
	DB           int    `mapstructure:"db"`
	PoolSize     int    `mapstructure:"pool_size"`
	MinIdleConns int    `mapstructure:"min_idle_conns"`
}

type MySQLConfig struct {
	Host         string `mapstructure:"host"`
	User         string `mapstructure:"user"`
	Password     string `mapstructure:"password"`
	DB           string `mapstructure:"db"`
	Port         int    `mapstructure:"port"`
	MaxOpenConns int    `mapstructure:"max_open_conns"`
	MaxIdleConns int    `mapstructure:"max_idle_conns"`
}

type SnowFlakeConfig struct {
	MachineID int `mapstructure:"machine_id"`
}

// Init 初始化配置，读取配置文件并监控其变化
func Init() error {
	// 设置配置文件路径和名称
	viper.SetConfigFile("./config/config.yaml")
	// 监控配置文件的变化
	viper.WatchConfig()
	// 当配置文件发生更改时，触发该回调函数
	viper.OnConfigChange(func(in fsnotify.Event) {
		fmt.Println("配置文件被修改")
		viper.Unmarshal(&Conf)
	})
	// 读取配置文件
	err := viper.ReadInConfig()
	if err != nil {
		// 如果读取配置文件失败，则抛出错误并退出
		panic(fmt.Errorf("ReadInConfig failed, err: %v", err))
	}
	// 将配置文件中的内容解析到 Conf 变量中
	if err := viper.Unmarshal(&Conf); err != nil {
		// 如果解析失败，则抛出错误并退出
		panic(fmt.Errorf("unmarshal to Conf failed, err:%v", err))
	}
	// 返回错误（如果有），调用方可以处理该错误
	return err
}
