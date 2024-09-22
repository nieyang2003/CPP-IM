CREATE KEYSPACE IF NOT EXISTS ychat WITH REPLICATION = {'class': 'SimpleStrategy', 'replication_factor': 1};

/* 用户发件箱，7天删除 */
CREATE TABLE IF NOT EXISTS ychat.user_outbox (
    uid bigint,
    sendtime bigint,
    msgid text,
    PRIMARY KEY ((user_id), sendtime, msgid)
) WITH default_time_to_live = 604800;

/* 用户收件箱，7天删除 */
CREATE TABLE IF NOT EXISTS ychat.user_inbox (
    uid bigint,
    seq bigint,
	msgid text,
	PRIMARY KEY (uid, seq)
) WITH default_time_to_live = 604800;

/* 消息表，7天删除 */
CREATE TABLE IF NOT EXISTS ychat.messages (
    msgid text,
    data blob,
	PRIMARY KEY (msgid)
) WITH default_time_to_live = 605800;