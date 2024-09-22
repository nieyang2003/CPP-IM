package routers

import (
	controllers "gate_server/controller"
	"gate_server/logger"
	"net/http"

	"github.com/gin-gonic/gin"
)

func SetupRouter() *gin.Engine {
	r := gin.Default()
	// 使用默认的 Gin 引擎，包含 Logger 和 Recovery 中间件
	r.Use(logger.GinLogger(), logger.GinRecovery(true))

	// 创建 API v1 版本的路由组
	// v1 := r.Group("/api/v1")
	// 配置不需要认证的路由
	r.POST("/login", controllers.LoginHandler)                   // 登录接口
	r.POST("/register", controllers.RegisterHandler)             // 注册接口
	r.POST("/get_varify_code", controllers.GetVerifyCodeHandler) // 获取验证码
	// 为后续路由添加 JWT 认证中间件

	// 配置 404 路由，处理未匹配的请求路径
	r.NoRoute(func(c *gin.Context) {
		c.JSON(http.StatusOK, gin.H{
			"msg": "404",
		})
	})
	return r
}
