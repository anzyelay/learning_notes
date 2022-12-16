# OpenWrt

- [OpenWrt](#openwrt)
  - [一、各模块简介](#一各模块简介)
  - [二、BASE SYSTEM](#二base-system)
    - [2.1 basic configuration](#21-basic-configuration)

## 一、各模块简介

模块|全称|作用
|:-:|:-|:-|
[uci](https://openwrt.org/docs/guide-user/base-system/uci)| unified configuration interface| 其目标是将系统中各种服务原本需以不同形式的命令且处于不同目录的配置项，以**统一**的接口形式（相同的配置目录/etc/config, 统一的命令行控制如：uci show wireless）归集操控起来，用来完成各项服务的配置，而不用单独使用各服务的配置项，比如配置iptables, ifconfig, uhttpd等。
luCI |　a web interface for UCI，base on LUA and its openwrt | 提供一个Web页面来实现uci的配置，使用LUA编写

## 二、[BASE SYSTEM](https://openwrt.org/docs/guide-user/base-system/start)

### 2.1 basic configuration

- uci： 使用命令行的形式配置
- luci: 使用web页面的形式配置
- classic Linux config： 基本的linux配置方式，不是所有的功能都兼容到uci里，需要单独配置
  
