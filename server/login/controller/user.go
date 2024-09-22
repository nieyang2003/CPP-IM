package controllers

import (
	"errors"
	"gate_server/dao/mysql"
	"gate_server/dao/redis"
	"gate_server/logic"
	"gate_server/models"
	"strconv"

	"github.com/gin-gonic/gin"
	"go.uber.org/zap"
)

func RegisterHandler(c *gin.Context) {
	var req models.RegisterRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		ReturnError(c, 2, "json格式错误")
		return
	}
	// 格式
	if len(req.UserName) == 0 || len(req.Password) == 0 ||
		len(req.Confirm) == 0 || len(req.VarifyCode) == 0 {
		ReturnError(c, 2, "消息格式错误")
		return
	}
	// 验证密码是否相同
	if req.Password != req.Confirm {
		ReturnError(c, 2, "两次输入的密码不同")
		return
	}
	// 从redis中验证注册码
	varify_code, err := redis.GetVarifyCode(req.Email)
	if err != nil {
		ReturnError(c, 2, "验证码错误")
		return
	}
	// 验证码是否相等
	if req.VarifyCode != varify_code {
		ReturnError(c, 2, "验证码错误")
		return
	}
	// 注册
	err = mysql.Register(&req)
	if errors.Is(err, mysql.ErrorUserExist) {
		ReturnError(c, 2, "用户已注册")
		return
	}
	if err != nil {
		zap.L().Error("mysql.Register() failed", zap.Error(err))
		ReturnError(c, 2, err.Error())
		return
	}

	ReturnSuccess(c, "注册成功")
}

func LoginHandler(c *gin.Context) {
	var req models.LoginRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		ReturnError(c, 2, "json格式错误")
		return
	}
	// 格式
	if len(req.Email) == 0 || len(req.Password) == 0 {
		ReturnError(c, 2, "消息格式错误")
		return
	}
	// 登录
	uid, err := mysql.Login(&req)
	if err != nil {
		if errors.Is(err, mysql.ErrorUserNotExist) {
			ReturnError(c, 2, "用户不存在")
		} else if errors.Is(err, mysql.ErrorPassWrong) {
			ReturnError(c, 2, "密码错误")
		} else {
			ReturnError(c, 2, "内部错误")
			zap.L().Error("err: ", zap.Error(err))
		}
		return
	}
	// 聊天服务器
	msg_server_resp, err := logic.DispatchMsgServer(uid, c.ClientIP())
	if err != nil {
		ReturnError(c, 2, "获取msg server失败")
		zap.L().Error("", zap.Error(err))
		return
	}
	// 返回
	zap.L().Debug("登录成功")
	msg := gin.H{
		"uid":                 strconv.FormatUint(uid, 10),
		"msg_server_location": msg_server_resp.GetLocation(),
		"token":               msg_server_resp.GetToken(),
	}
	ReturnSuccess(c, msg)
}

func GetVerifyCodeHandler(c *gin.Context) {
	var req models.GetVerifyCodeRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		ReturnError(c, 1, "消息格式错误")
		return
	}
	// TODO: 判断用户是否存在
	if err := logic.GetVerifyCode(req.Email, true); err != nil {
		ReturnError(c, 1, "发送失败，请重试")
		return
	}
	ReturnSuccess(c, "发送成功")
}
