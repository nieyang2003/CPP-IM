CREATE DATABASE IF NOT EXISTS ychat;

SET NAMES utf8mb4;				/* 使用 utf8mb4 字符集 */
SET FOREIGN_KEY_CHECKS = 0;     /* 暂时禁用了外键检查 */
USE `ychat`;

-- 创建user表
DROP TABLE IF EXISTS `user`;
CREATE TABLE `user` (
    `id` bigint(20) NOT NULL AUTO_INCREMENT,
    `user_id` bigint(20) NOT NULL,
	`email` varchar(64) COLLATE utf8mb4_general_ci,
    `username` varchar(64) COLLATE utf8mb4_general_ci NOT NULL,
    `password` varchar(64) COLLATE utf8mb4_general_ci NOT NULL,
    `sex` tinyint(4) NOT NULL DEFAULT '0',
    `create_time` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
    `update_time` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
	`avatar_url` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '0/avatar/default',
    PRIMARY KEY (`id`),
    UNIQUE KEY `idx_email` (`email`) USING BTREE,
    UNIQUE KEY `idx_user_id` (`user_id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- 创建user收件箱


-- 创建user发件箱


-- user好友关系

