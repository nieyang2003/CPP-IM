# 博客
架构参考
https://cloud.tencent.com/developer/article/1853605
https://juejin.cn/post/6999584342531899428
grpc异步编程
https://www.cnblogs.com/jinyunshaobing/p/16697028.html
# Build
## 第三方库
...
# Run
```
# wsl启动相关服务
redis-server /usr/local/redis/redis.conf
sudo service mysql start
sudo systemctl status mongod
sudo systemctl start mongod
sudo kafka-server-start.sh -daemon /opt/kafka/config/server.properties
```

file: 文件上传服务
login: 做短连接相关内容，注册登录，搜索好友，查看信息，更新信息
msg: 接受或推送实时消息（收发消息，加入、退出群聊，创建会话，添加、删除好友），写入消息队列
route: 路由(TODO: 服务注册与发现)
storage: 存储数据
transfer: 消费消息队列信息，写入数据库，生成时间戳、seq号
varify: 邮件验证码服务
...(视频通话)