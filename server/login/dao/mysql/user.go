package mysql

import (
	"crypto/md5"
	"database/sql"
	"encoding/hex"
	"gate_server/models"
	"gate_server/pkg/snowflake"
)

// md5加密
func encryptMD5(data []byte) string {
	h := md5.New()
	h.Write(data)
	return hex.EncodeToString(h.Sum(nil))
}

// * 把redis放在这里了

func Register(user *models.RegisterRequest) (err error) {
	sqlStr := "select count(user_id) from user where username = ?"
	var count int64
	// 查询
	err = db.Get(&count, sqlStr, user.UserName)
	if err != nil && err != sql.ErrNoRows {
		return err
	}
	// 用户是否存在
	if count > 0 {
		return ErrorUserExist
	}
	// 生成user id
	userID, err := snowflake.GetID()
	if err != nil {
		return err
	}
	// 生成加密密码
	pass := encryptMD5([]byte(user.Password))
	// 将用户插入数据库
	sqlStr = "insert into user(user_id, username, email, password, sex) values (?,?,?,?,?)"
	_, err = db.Exec(sqlStr, userID, user.UserName, user.Email, pass, user.Sex)
	return
}

func Login(user *models.LoginRequest) (uint64, error) {
	sqlStr := "select user_id, email, password from user where email = ?"
	var dbInfo models.User
	err := db.Get(&dbInfo, sqlStr, user.Email)
	if err != nil && err != sql.ErrNoRows {
		// 查询数据库出错
		return 0, err
	}
	if err == sql.ErrNoRows {
		// 用户不存在
		return 0, ErrorUserNotExist
	}
	userPass := encryptMD5([]byte(user.Password))
	if userPass != dbInfo.Password {
		return 0, ErrorPassWrong
	}
	return dbInfo.UserID, nil
}
