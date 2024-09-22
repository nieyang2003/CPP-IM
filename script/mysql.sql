CREATE DATABASE IF NOT EXISTS ychat;

SET NAMES utf8mb4;				/* 使用 utf8mb4 字符集 */
SET FOREIGN_KEY_CHECKS = 0;     /* 暂时禁用了外键检查 */
USE `ychat`;

-- user表
DROP TABLE IF EXISTS `user`;
CREATE TABLE `user` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `user_id` BIGINT UNSIGNED NOT NULL COMMENT '雪花算法生成',
	`email` VARCHAR(64) COLLATE utf8mb4_general_ci,
    `username` VARCHAR(64) COLLATE utf8mb4_general_ci NOT NULL,
    `password` VARCHAR(64) COLLATE utf8mb4_general_ci NOT NULL,
    `sex` TINYINT(4) NOT NULL DEFAULT '0',
    `create_time` TIMESTAMP  NULL DEFAULT CURRENT_TIMESTAMP,
    `update_time` TIMESTAMP  NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
	`avatar_url` VARCHAR(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '0/avatar/default',
    PRIMARY KEY (`id`),
    UNIQUE KEY `idx_email` (`email`) USING BTREE,
    UNIQUE KEY `idx_user_id` (`user_id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- 好友关系表
DROP TABLE IF EXISTS `friend`;
CREATE TABLE `friend` (
	`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
	`uid` BIGINT UNSIGNED NOT NULL,
	`friend_uid` BIGINT UNSIGNED NOT NULL,
	`note` VARCHAR(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT NULL,
	PRIMARY KEY (`id`),
	UNIQUE KEY `idx_user_id` (`uid`, `friend_uid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- 好友操作表
DROP TABLE IF EXISTS `friend_action_log`;
CREATE TABLE `friend_action_log`  (
	`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '日志ID，主键',
	`from_uid` BIGINT UNSIGNED NOT NULL COMMENT '操作的用户',
	`to_uid` BIGINT UNSIGNED NOT NULL,
	`action_type` SMALLINT NOT NULL COMMENT '操作类型，0-请求，1-删除，2-添加成功，3-拒绝',
	`action_time` TIMESTAMP  NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
	PRIMARY KEY (`id`) USING BTREE,
	INDEX `from_to_uid`(`from_uid` ASC, `to_uid` ASC) USING BTREE
) ENGINE=InnoDB CHARACTER SET = utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 群关系表
DROP TABLE IF EXISTS `group_info`;
CREATE TABLE `group_info` (
	`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `group_id` BIGINT UNSIGNED NOT NULL COMMENT '群组ID 雪花算法生成',
    `name` VARCHAR(255) NOT NULL COMMENT '群组名称',
    `owner_uid` BIGINT UNSIGNED NOT NULL COMMENT '群主的用户ID',
    `create_time`timestamp NULL DEFAULT CURRENT_TIMESTAMP,
    `description` TEXT COMMENT '群组描述',
    PRIMARY KEY (`id`),
	UNIQUE KEY `idx_group_id` (`group_id`) USING BTREE,
	FOREIGN KEY (`owner_uid`) REFERENCES `user` (`user_id`) ON DELETE CASCADE
) ENGINE=InnoDB CHARACTER SET = utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 群成员表
CREATE TABLE `group_member` (
    `group_id` BIGINT UNSIGNED NOT NULL COMMENT '群组ID，外键',
    `user_id` BIGINT UNSIGNED NOT NULL COMMENT '用户ID，外键',
    `join_time` BIGINT UNSIGNED NOT NULL COMMENT '加入群组的时间，时间戳（毫秒）',
    `role` SMALLINT NOT NULL DEFAULT 0 COMMENT '用户角色，0表示普通成员，1表示管理员，2表示群主',
    PRIMARY KEY (`group_id`, `user_id`),
    FOREIGN KEY (`group_id`) REFERENCES `group_info` (`group_id`) ON DELETE CASCADE,
    FOREIGN KEY (`user_id`) REFERENCES `user` (`user_id`) ON DELETE CASCADE
) ENGINE=InnoDB CHARACTER SET = utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 群操作记录表
CREATE TABLE `group_action_log` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '日志ID，主键',
    `group_id` BIGINT UNSIGNED NOT NULL COMMENT '群组ID，外键',
    `user_id` BIGINT UNSIGNED NOT NULL COMMENT '执行操作的用户ID，外键',
    `action_type` SMALLINT NOT NULL COMMENT '操作类型，0-创建，1-删除，2-添加，3-进入，4退出',
	`action_time` TIMESTAMP  NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    `description` TEXT COMMENT '操作描述',
    PRIMARY KEY (`id`),
    FOREIGN KEY (`group_id`) REFERENCES `group_info` (`group_id`) ON DELETE CASCADE,
    FOREIGN KEY (`user_id`) REFERENCES `user` (`user_id`) ON DELETE CASCADE
) ENGINE=InnoDB CHARACTER SET = utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 消息表
DROP TABLE IF EXISTS `messages`;
CREATE TABLE `messages` (
    `msgid` VARCHAR(64) COLLATE utf8mb4_general_ci NOT NULL COMMENT '消息ID，主键',
    `data` BLOB COMMENT 'proto数据',
    PRIMARY KEY (`msgid`),
    UNIQUE KEY `idx_msgid` (`msgid`) USING BTREE
) ENGINE=InnoDB CHARACTER SET = utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 用户信箱
DROP TABLE IF EXISTS `user_box`;
CREATE TABLE `user_box` (
	`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
	`from_uid` BIGINT UNSIGNED NOT NULL,
    `to_uid` BIGINT UNSIGNED NOT NULL,
    `sendtime` BIGINT NOT NULL,
	`msgid` varchar(64) COLLATE utf8mb4_general_ci NOT NULL,
	PRIMARY KEY (`id`),
    UNIQUE KEY `idx_from_to_sendtime` (`from_uid`, `to_uid`, `sendtime`) USING BTREE,
    FOREIGN KEY (`from_uid`) REFERENCES `user` (`user_id`) ON DELETE CASCADE,
    FOREIGN KEY (`to_uid`) REFERENCES `user` (`user_id`) ON DELETE CASCADE,
	FOREIGN KEY (`msgid`) REFERENCES `messages` (`msgid`) ON DELETE CASCADE
) ENGINE=InnoDB CHARACTER SET = utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 群信箱
DROP TABLE IF EXISTS `group_box`;
CREATE TABLE `group_box` (
	`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
	`gid` BIGINT UNSIGNED NOT NULL,
	`sendtime` BIGINT NOT NULL,
	`msgid` varchar(64) COLLATE utf8mb4_general_ci NOT NULL,
	PRIMARY KEY (`id`),
    UNIQUE KEY `idx_gid_sendtime_msgid` (`gid`, `sendtime`, `msgid`) USING BTREE,
	FOREIGN KEY (`msgid`) REFERENCES `messages` (`msgid`) ON DELETE CASCADE
) ENGINE=InnoDB CHARACTER SET = utf8mb4 COLLATE=utf8mb4_unicode_ci;