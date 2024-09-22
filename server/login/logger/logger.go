package logger

import (
	"gate_server/settings"
	"net"
	"net/http"
	"net/http/httputil"
	"os"
	"runtime/debug"
	"strings"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/natefinch/lumberjack"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var lg *zap.Logger

// 初始化日志配置
func Init(config *settings.LogConfig, mode string) (err error) {
	// 获取日志写入器，配置日志文件
	writeSyncer := getLogWriter(config.Filename, config.MaxSize, config.MaxBackups, config.MaxAge)
	// 获取日志编码器，指定日志的输出格式
	encoder := getEncoder()
	// 解析日志级别
	var level = new(zapcore.Level)
	err = level.UnmarshalText([]byte(config.Level))
	if err != nil {
		return
	}
	var core zapcore.Core
	if mode == "dev" {
		// 进入开发模式，日志输出到终端
		consoleEncoder := zapcore.NewConsoleEncoder(zap.NewDevelopmentEncoderConfig())
		core = zapcore.NewTee(
			// 同时将日志写入文件和输出到终端
			zapcore.NewCore(encoder, writeSyncer, level),
			zapcore.NewCore(consoleEncoder, zapcore.Lock(os.Stdout), zapcore.DebugLevel),
		)
	} else {
		// 非开发模式，仅将日志写入文件
		core = zapcore.NewCore(encoder, writeSyncer, level)
	}
	// 创建 logger 实例，并将调用者信息（如文件名、行号）添加到日志中
	lg = zap.New(core, zap.AddCaller())
	// 将初始化的 logger 设置为全局 logger
	zap.ReplaceGlobals(lg)
	return
}

// 创建并返回一个日志写入器，使用 Lumberjack 实现日志轮转
func getLogWriter(filename string, maxSize, maxBackup, maxAge int) zapcore.WriteSyncer {
	// 创建一个 Lumberjack Logger，用于处理日志文件的写入和轮转
	lumberJackLogger := &lumberjack.Logger{
		Filename:   filename,  // 日志文件的路径
		MaxSize:    maxSize,   // 日志文件的最大大小（单位：MB）
		MaxBackups: maxBackup, // 保留的旧日志文件的最大数量
		MaxAge:     maxAge,    // 日志文件保留的最大天数
	}

	// 使用 zapcore.AddSync 将 lumberjack.Logger 转换为 zapcore.WriteSyncer
	// 这样 zap 的日志系统可以使用这个日志写入器
	return zapcore.AddSync(lumberJackLogger)
}

func getEncoder() zapcore.Encoder {
	encoderConfig := zap.NewProductionEncoderConfig()
	encoderConfig.EncodeTime = zapcore.ISO8601TimeEncoder
	encoderConfig.TimeKey = "time"
	encoderConfig.EncodeLevel = zapcore.CapitalLevelEncoder
	encoderConfig.EncodeDuration = zapcore.SecondsDurationEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	return zapcore.NewJSONEncoder(encoderConfig)
}

func GinLogger() gin.HandlerFunc {
	return func(c *gin.Context) {
		start := time.Now()
		path := c.Request.URL.Path
		query := c.Request.URL.RawQuery
		c.Next()

		cost := time.Since(start)
		lg.Info(path,
			zap.Int("status", c.Writer.Status()),
			zap.String("method", c.Request.Method),
			zap.String("path", path),
			zap.String("query", query),
			zap.String("ip", c.ClientIP()),
			zap.String("user-agent", c.Request.UserAgent()),
			zap.String("errors", c.Errors.ByType(gin.ErrorTypePrivate).String()),
			zap.Duration("cost", cost),
		)
	}
}

// GinRecovery recover掉项目可能出现的panic，并使用zap记录相关日志
func GinRecovery(stack bool) gin.HandlerFunc {
	return func(c *gin.Context) {
		defer func() {
			if err := recover(); err != nil {
				// Check for a broken connection, as it is not really a
				// condition that warrants a panic stack trace.
				var brokenPipe bool
				if ne, ok := err.(*net.OpError); ok {
					if se, ok := ne.Err.(*os.SyscallError); ok {
						if strings.Contains(strings.ToLower(se.Error()), "broken pipe") || strings.Contains(strings.ToLower(se.Error()), "connection reset by peer") {
							brokenPipe = true
						}
					}
				}

				httpRequest, _ := httputil.DumpRequest(c.Request, false)
				if brokenPipe {
					lg.Error(c.Request.URL.Path,
						zap.Any("error", err),
						zap.String("request", string(httpRequest)),
					)
					// If the connection is dead, we can't write a status to it.
					c.Error(err.(error)) // nolint: errcheck
					c.Abort()
					return
				}

				if stack {
					lg.Error("[Recovery from panic]",
						zap.Any("error", err),
						zap.String("request", string(httpRequest)),
						zap.String("stack", string(debug.Stack())),
					)
				} else {
					lg.Error("[Recovery from panic]",
						zap.Any("error", err),
						zap.String("request", string(httpRequest)),
					)
				}
				c.AbortWithStatus(http.StatusInternalServerError)
			}
		}()
		c.Next()
	}
}
