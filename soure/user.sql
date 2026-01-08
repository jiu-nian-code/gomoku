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
    white_uid int comment "白棋用户id",
    black_uid int comment "黑棋用户id",
    white_cur_score int comment "白棋用户当前总分",
    black_cur_score int comment "黑棋用户当前总分",
    white_cur_get int comment "白棋用户当前得分",
    black_cur_get int comment "黑棋用户当前得分",
    is_white_win bool comment "是否是白棋赢",
    cur_date timestamp
);

create table if not exists matches_step
(
    match_id bigint unsigned comment "对局id",
    x int comment "x坐标",
    y int comment "y坐标",
    step int unsigned comment "步数",
    foreign key (match_id) references matches(match_id)
);

create table if not exists user_avatar
(
    avatar_id int primary key auto_increment comment "头像ID",
    id int comment "用户ID",
    foreign key (id) references user(id)
);

-- 测试用户
insert into user(name, password, score, total_games, win_games) value("田所浩二", MD5("HOMOhomo@114514"), 1000, 0, 0);
insert into user(name, password, score, total_games, win_games) value("佐佐木淳平", MD5("YADAmoyada@114514"), 1000, 0, 0);