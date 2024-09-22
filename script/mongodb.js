use ychat;
db.createCollection("user_inbox");
db.user_inbox.createIndex({ expiration_date: 1 }, { expireAfterSeconds: 604800 });

db.createCollection("group_inbox");
db.group_inbox.createIndex({ expiration_date: 1 }, { expireAfterSeconds: 604800 });

/*
-- 插入一条群离线消息
db.group_inbox.insert({
    group_id: 54321,
    seq: 1,
    data: "test test test",
    expiration_date: new Date(Date.now() + 7 * 24 * 60 * 60 * 1000) // 7天后过期
});
*/