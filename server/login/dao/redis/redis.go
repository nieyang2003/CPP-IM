package redis

import (
	"fmt"
	"gate_server/settings"

	"github.com/go-redis/redis"
)

var client *redis.Client

var emailKeyPrefix = "ychat::email::"
var passKeyPrefix = "ychat::user_pass::"

func Init(cfg *settings.RedisConfig) (err error) {
	client = redis.NewClient(&redis.Options{
		Addr:         fmt.Sprintf("%s:%d", cfg.Host, cfg.Port),
		Password:     cfg.Password, // no password set
		DB:           cfg.DB,       // use default DB
		PoolSize:     cfg.PoolSize,
		MinIdleConns: cfg.MinIdleConns,
	})

	_, err = client.Ping().Result()
	if err != nil {
		return err
	}
	return nil
}
