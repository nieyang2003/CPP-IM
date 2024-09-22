package controllers

import (
	"errors"
	"gate_server/dao/mysql"
	"gate_server/dao/redis"
	"gate_server/logic"
	"gate_server/models"
	"gate_server/pkg/jwt"
	"strconv"
	"strings"

	"github.com/gin-gonic/gin"
	"go.uber.org/zap"
)

func RegisterHandler(c *gin.Context) {
	var req models.RegisterRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		ReturnError(c, CodeInvalidParams, "json格式错误")
		return
	}
	// 格式
	if len(req.UserName) == 0 || len(req.Password) == 0 ||
		len(req.Confirm) == 0 || len(req.VarifyCode) == 0 {
		ReturnError(c, CodeInvalidParams, "消息格式错误")
		return
	}
	// 验证密码是否相同
	if req.Password != req.Confirm {
		ReturnError(c, CodePasswordErr, "两次输入的密码不同")
		return
	}
	// 从redis中验证注册码
	varify_code, err := redis.GetVarifyCode(req.Email)
	if err != nil {
		ReturnError(c, CodeVarifyCodeErr, "验证码错误")
		return
	}
	// 验证码是否相等
	if req.VarifyCode != varify_code {
		ReturnError(c, CodeVarifyCodeErr, "验证码错误")
		return
	}
	// 注册
	err = mysql.Register(&req)
	if errors.Is(err, mysql.ErrorUserExist) {
		ReturnError(c, CodeUserExist, "用户已注册")
		return
	}
	if err != nil {
		zap.L().Error("mysql.Register() failed", zap.Error(err))
		ReturnError(c, CodeServerBusy, err.Error())
		return
	}

	ReturnSuccess(c, "注册成功")
}

func LoginHandler(c *gin.Context) {
	var req models.LoginRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		ReturnError(c, CodeInvalidParams, "json格式错误")
		return
	}
	// 格式
	if len(req.Email) == 0 || len(req.Password) == 0 {
		ReturnError(c, CodeInvalidParams, "消息格式错误")
		return
	}
	// 登录
	uid, err := mysql.Login(&req)
	if err != nil {
		if errors.Is(err, mysql.ErrorUserNotExist) {
			ReturnError(c, CodeUserNotExist, "用户不存在")
		} else if errors.Is(err, mysql.ErrorPassWrong) {
			ReturnError(c, CodePasswordErr, "密码错误")
		} else {
			ReturnError(c, CodeServerBusy, "内部错误")
			zap.L().Error("err: ", zap.Error(err))
		}
		return
	}
	// 聊天服务器
	msg_server_resp, err := logic.DispatchMsgServer(uid, c.ClientIP())
	if err != nil {
		ReturnError(c, CodeServerBusy, "获取msg server失败")
		zap.L().Error("", zap.Error(err))
		return
	}
	// 返回
	zap.L().Debug("登录成功")
	// 生成Token
	aToken, rToken, err := jwt.GenToken(uid)
	if err != nil {
		ReturnError(c, CodeServerBusy, "生成token失败")
		zap.L().Error("", zap.Error(err))
		return
	}
	msg := gin.H{
		"uid":                 strconv.FormatUint(uid, 10),
		"msg_server_location": msg_server_resp.GetLocation(),
		"token":               msg_server_resp.GetToken(),
		"name":                "TODO",
		"avatar":              "TODO",
		"sex":                 0,
		"access_token":        aToken,
		"refresh_token":       rToken,
	}
	ReturnSuccess(c, msg)
}

func GetVerifyCodeHandler(c *gin.Context) {
	var req models.GetVerifyCodeRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		ReturnError(c, CodeInvalidParams, "消息格式错误")
		return
	}
	// TODO: 判断用户是否存在
	if err := logic.GetVerifyCode(req.Email, true); err != nil {
		ReturnError(c, CodeServerBusy, "发送失败，请重试")
		return
	}
	ReturnSuccess(c, "发送成功")
}

func RefreshTokenHandler(c *gin.Context) {
	rt := c.Query("refresh_token")
	// 客户端携带Token有三种方式 1.放在请求头 2.放在请求体 3.放在URI
	// 这里假设Token放在Header的Authorization中，并使用Bearer开头
	// 这里的具体实现方式要依据你的实际业务情况决定
	authHeader := c.Request.Header.Get("Authorization")
	if authHeader == "" {
		ReturnError(c, CodeInvalidToken, "请求头缺少Auth Token")
		c.Abort()
		return
	}
	// 按空格分割
	parts := strings.SplitN(authHeader, " ", 2)
	if !(len(parts) == 2 && parts[0] == "Bearer") {
		ReturnError(c, CodeInvalidToken, "Token格式不对")
		c.Abort()
		return
	}
	aToken, rToken, err := jwt.RefreshToken(parts[1], rt)
	if err != nil {
		ReturnError(c, CodeInvalidToken, err.Error())
		zap.L().Error("jwt.RefreshToken", zap.Error(err))
		c.Abort()
		return
	}
	msg := gin.H{
		"access_token":  aToken,
		"refresh_token": rToken,
	}
	ReturnSuccess(c, msg)
}
