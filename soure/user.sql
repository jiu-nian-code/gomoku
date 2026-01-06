create database if not exists gomoku_db;
use gomoku_db;
create table if not exists user
(
    id int primary key auto_increment comment "用户ID",
    name varchar(32) unique comment "用户名",
    password varchar(50) comment "密码",
    score int comment "得分",
    total_games int comment "游戏总场数",
    win_games int comment "游戏胜利场数"
);

create table if not exists matches
(
    match_id bigint unsigned primary key auto_increment comment "对局ID",
    uid int comment "用户id",
    oppo_uid int comment "对方id",
    cur_score int comment "当前总分",
    is_win bool comment "输赢",
    is_white bool comment "是否是白棋",
    cur_get int comment "当前得分",
    cur_date timestamp
);

create table if not exists set
{
    
};

-- 测试用户
insert into user(name, password, score, total_games, win_games) value("田所浩二", MD5("HOMOhomo@114514"), 1000, 0, 0);
insert into user(name, password, score, total_games, win_games) value("佐佐木淳平", MD5("YADAmoyada@114514"), 1000, 0, 0);