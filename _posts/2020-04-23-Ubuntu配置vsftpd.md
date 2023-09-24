---
title: Ubuntu 配置 vsftpd
date: 2020-04-23 21:34:00 +0800
categories: [Linux]
tags: [ubuntu, vsftpd]    # TAG names should always be lowercase
---

## 1 安装vsptfd

`apt install vsftpd`

## 2 启动vsftpd服务

`systemctl start vsftpd.service`

## 3 创建 ftp 用户

### 3.1 创建用户

`useradd -m ftpuser`

### 3.2 设置密码

`passwd ftpuser`

## 4 新建用户清单文件

1. 创建文件 `/etc/vsftpd.user_list`
2. 在其中添加用户 ftpuser ，并且保存退出

## 5 编辑 vsftpd 配置文件

* 取消注释 `write_enable=YES`
* 添加信息 `userlist_enable=YES`
* 添加信息 `userlist_deny=NO`
* 添加信息 `userlist_file=/etc/vsftpd.user_list`

## 6 在 Windows 中连接 ftp 服务器

### 6.1 连接FTP服务器

`ftp [IP地址]`

### 6.2 查看当前目录

`pwd`： Linux当前目录

`lcd`： Windows当前目录

### 6.3 上传/下载文件

`get [文件名]` / `put [文件名]`
