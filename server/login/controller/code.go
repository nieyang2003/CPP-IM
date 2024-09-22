package controllers

type ErrorCode int32

const (
	CodeSuccess           ErrorCode = 1000
	CodeInvalidParams     ErrorCode = 1001
	CodeUserExist         ErrorCode = 1002
	CodeUserNotExist      ErrorCode = 1003
	CodeInvalidPassword   ErrorCode = 1004
	CodeServerBusy        ErrorCode = 1005
	CodeInvalidToken      ErrorCode = 1006
	CodeInvalidAuthFormat ErrorCode = 1007
	CodeNotLogin          ErrorCode = 1008
	CodePasswordErr       ErrorCode = 1009
	CodeVarifyCodeErr     ErrorCode = 1010
)

var msgFlags = map[ErrorCode]string{
	CodeSuccess:           "success",
	CodeInvalidParams:     "请求参数错误",
	CodeUserExist:         "用户名重复",
	CodeUserNotExist:      "用户不存在",
	CodeInvalidPassword:   "用户名或密码错误",
	CodeServerBusy:        "服务繁忙",
	CodeInvalidToken:      "无效的Token",
	CodeInvalidAuthFormat: "认证格式有误",
	CodeNotLogin:          "未登录",
	CodePasswordErr:       "密码错误",
	CodeVarifyCodeErr:     "验证码错误",
}

func (c ErrorCode) Msg() string {
	msg, ok := msgFlags[c]
	if ok {
		return msg
	}
	return msgFlags[CodeServerBusy]
}
