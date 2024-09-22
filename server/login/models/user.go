package models

// 客户端注册 http请求
type RegisterRequest struct {
	UserName   string `json:"username"`
	Email      string `json:"email"`
	Password   string `json:"password"`
	Confirm    string `json:"confirm"`
	Sex        int    `json:"sex"`
	VarifyCode string `json:"varify_code"`
}

// 客户端登录 http请求
type LoginRequest struct {
	Email    string `json:"email"`
	Password string `json:"password"`
}

type GetVerifyCodeRequest struct {
	Email string `json:"email"`
}

type User struct {
	UserID   uint64 `json:"user_id" db:"user_id"`
	Email    string `json:"email" db:"email"`
	UserName string `json:"username" db:"username"`
	Password string `json:"password" db:"password"`
	Sex      int    `json:"sex" db:"sex"`
}
