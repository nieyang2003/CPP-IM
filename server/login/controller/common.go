package controllers

import (
	"net/http"

	"github.com/gin-gonic/gin"
	"go.uber.org/zap"
)

type ErrResp struct {
	Code int32       `json:"code"`
	Msg  interface{} `json:"msg"`
}

const (
	CodeSuccess int32 = 0
)

func ReturnError(c *gin.Context, code int32, msg interface{}) {
	json := &ErrResp{
		Code: code,
		Msg:  msg,
	}
	zap.L().Error(msg.(string))
	c.JSON(http.StatusOK, json)
}

func ReturnSuccess(c *gin.Context, msg interface{}) {
	json := &ErrResp{
		Code: CodeSuccess,
		Msg:  msg,
	}
	c.JSON(http.StatusOK, json)
}
