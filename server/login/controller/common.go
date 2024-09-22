package controllers

import (
	"net/http"

	"github.com/gin-gonic/gin"
	"go.uber.org/zap"
)

type ErrResp struct {
	Code ErrorCode   `json:"code"`
	Msg  interface{} `json:"msg"`
}

func ReturnError(c *gin.Context, code ErrorCode, msg interface{}) {
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
