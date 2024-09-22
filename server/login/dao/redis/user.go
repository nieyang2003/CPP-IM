package redis

// 查询用户邮箱验证码缓存
func GetVarifyCode(email string) (string, error) {
	key := emailKeyPrefix + email
	val, err := client.Get(key).Result()
	return val, err
}

// 用户是否在缓存中
func CheckUserExist() {

}
