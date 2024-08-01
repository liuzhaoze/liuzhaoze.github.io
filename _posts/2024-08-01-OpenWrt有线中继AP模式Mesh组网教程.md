---
title: OpenWrt 有线中继 AP 模式 Mesh 组网教程
date: 2024-08-01 15:22:27 +0800
categories: [软件]
tags: [openwrt]    # TAG names should always be lowercase
---

> 参考资料：
>
> - [OpenWrt之有线中继无缝漫游mesh组网详解](https://blog.csdn.net/a924282761/article/details/135991716)
> - [OpenWrt设置开启802.11r、纯AP模式、自动剔除弱信号](https://blog.csdn.net/weixin_52027058/article/details/128701905)

## 1 网络环境

网络拓扑如下：

![网络拓扑](/assets/img/OpenWrtAPMesh/NetworkTopology.png)

本教程中将光猫/主路由器的 DHCP 地址范围设置为 `192.168.1.100` 到 `192.168.1.254` 。`192.168.1.2` 到 `192.168.1.99` 用于静态 IP 地址分配。

OpenWrt 设备实际的 WAN 口 MAC 地址以及 LAN 网桥（ br-lan ）的 MAC 地址需要在安装好 OpenWrt 后进入后台查看。具体页面位于 `网络 - 接口` 中。

![网络接口1](/assets/img/OpenWrtAPMesh/网络接口1.png)

![网络接口2](/assets/img/OpenWrtAPMesh/网络接口2.png)

## 2 配置静态 IP

为了方便后续配置，我们为 OpenWrt 设备的 WAN 口和 LAN 网桥分别配置静态 IP 地址。

以光猫为例，静态 IP 地址的设置界面位于 `网络 - LAN 侧地址配置 - IPv4 DHCP 静态 IP 配置` 中。

![静态IP](/assets/img/OpenWrtAPMesh/静态IP.png)

本教程按照以下配置进行设置：

|设备|MAC 地址|IP 地址|
|:-:|:-:|:-:|
|AP 1 WAN 口|4C:C6:4C:11:11:0A|192.168.1.10|
|AP 2 WAN 口|4C:C6:4C:22:22:0A|192.168.1.11|
|AP 1 LAN 网桥|4C:C6:4C:11:11:0B|192.168.1.20|
|AP 2 LAN 网桥|4C:C6:4C:22:22:0B|192.168.1.21|

## 3 配置 OpenWrt 防火墙

为了实现通过 WAN/LAN 口 IP 访问 OpenWrt 后台，需要配置防火墙规则，放行 WAN 口入站数据。

将电脑连接至 OpenWrt 设备的 LAN 口，进入 OpenWrt 后台进行如下设置。

![防火墙1](/assets/img/OpenWrtAPMesh/防火墙1.png)

![防火墙2](/assets/img/OpenWrtAPMesh/防火墙2.png)

## 4 OpenWrt 接口设置

将 OpenWrt 设备的 WAN 口连接至光猫/主路由器的 LAN 口，并且将电脑连接至光猫/主路由器的 LAN 口。

经过[静态 IP 地址配置](#2-配置静态-ip)后，两台 OpenWrt 设备的 WAN 口应当分别获取了 `192.168.1.10` 和 `192.168.1.11` IP 地址。[配置好 OpenWrt 设备的防火墙](#3-配置-openwrt-防火墙)后，电脑应当可以通过上述 IP 地址访问 OpenWrt 后台。

进入 `网络 - 接口` 页面配置 OpenWrt 设备的接口。 **注意：在配置过程中只能点击“保存”，不要点击“保存并应用”。** 否则可能导致电脑无法访问 OpenWrt 后台。此情况发生时，请长按 Reset 键重置 OpenWrt 设备，重新配置。

### 4.1 配置 WAN/WAN6 接口

编辑 WAN/WAN6 接口：

![编辑 WAN 接口](/assets/img/OpenWrtAPMesh/编辑WAN接口.png)

将 WAN/WAN6 接口的协议设置为`不配置协议`并保存：

![WAN 接口协议1](/assets/img/OpenWrtAPMesh/WAN接口协议1.png)

![WAN 接口协议2](/assets/img/OpenWrtAPMesh/WAN接口协议2.png)

关闭 WAN 口的 DHCP/DHCPv6 服务并保存：

![WAN 口 DHCP](/assets/img/OpenWrtAPMesh/WAN口DHCP.png)

![WAN 口 DHCPv6](/assets/img/OpenWrtAPMesh/WAN口DHCPv6.png)

检查 WAN6 口的 DHCP 服务是否为关闭状态：

![WAN6 口 DHCP](/assets/img/OpenWrtAPMesh/WAN6口DHCP.png)

### 4.2 配置 LAN 接口

编辑 LAN 接口：

![编辑 LAN 接口](/assets/img/OpenWrtAPMesh/编辑LAN接口.png)

将 LAN 接口的协议设置为`静态地址`；AP1 的 IPv4 地址设置为 `192.168.1.20`，AP2 的 IPv4 地址设置为 `192.168.1.21`（与[静态 IP 地址配置](#2-配置静态-ip)中的 LAN 网桥 IP 地址相同）；IPv4 网关设置为 `192.168.1.1` （光猫/主路由器的 IP 地址），并保存：

![LAN 接口协议](/assets/img/OpenWrtAPMesh/LAN口协议.png)

在`高级设置`中设置 DNS 服务器为 `192.168.1.1`（光猫/主路由器的 IP 地址），并保存：

![LAN 接口 DNS](/assets/img/OpenWrtAPMesh/LAN口DNS.png)

检查 LAN 接口的 DHCP 服务是否为关闭状态：

![LAN 接口 DHCP](/assets/img/OpenWrtAPMesh/LAN口DHCP.png)

![LAN 接口 DHCPv6](/assets/img/OpenWrtAPMesh/LAN口DHCPv6.png)

### 4.3 应用配置

点击`网络 - 接口`页面右下角的`保存并应用`按钮后，**尽快**将连接在 OpenWrt 设备 WAN 口的网线**改接**至 OpenWrt 设备的 LAN 口。然后通过电脑访问 `192.168.1.20` 或 `192.168.1.21` 即可进入 AP1 或 AP2 的 OpenWrt 后台。（此时 OpenWrt 设备的 LAN 网桥获取到的是之前配置的静态 IP 地址）

![应用接口配置](/assets/img/OpenWrtAPMesh/应用接口配置.png)

至此，通过接口配置，OpenWrt 设备已经被配置为有线中继 AP 模式。

## 5 OpenWrt 无线设置

本节将配置 OpenWrt 设备的无线网络，使其能够通过 Mesh 组网的方式实现 AP 漫游。此处以 5G 无线网络为例。

点击`网络 - 无线`进入无线设置页面：

![无线页面](/assets/img/OpenWrtAPMesh/无线页面.png)

记录 AP1 和 AP2 的 5G 无线 MAC 地址：

|设备|MAC 地址|
|:-:|:-:|
|AP 1|4C:C6:4C:11:11:0C|
|AP 2|4C:C6:4C:22:22:0C|

![无线 MAC 地址](/assets/img/OpenWrtAPMesh/无线MAC地址.png)

点击右侧的`编辑`按钮进入无线网络配置页面：

### 5.1 设备配置

在`常规设置`中启用无线网络，并根据你的无线网络环境选择合适的信道（如果不知道怎么设置就选择`auto`）：

![设备配置-常规设置](/assets/img/OpenWrtAPMesh/设备配置-常规设置.png)

在`高级设置`中选择正确的国家代码：

![设备配置-高级设置](/assets/img/OpenWrtAPMesh/设备配置-高级设置.png)

### 5.2 接口配置

在`常规设置`中设置模式为`接入点AP`，设置 ESSID 为你的无线网络名称：

![接口配置-常规设置](/assets/img/OpenWrtAPMesh/接口配置-常规设置.png)

在`无线安全`中设置加密为 `WPA2-PSK`，算法为 `强制CCMP（AES）`，密钥设置为你的无线网络密码：

![接口配置-无线安全](/assets/img/OpenWrtAPMesh/接口配置-无线安全.png)

在`WLAN 漫游`中各项设置如下：

1. 勾选 `802.11r 快速切换`
2. NAS ID 填写当前设备**不带冒号**的 MAC 地址（ AP1 为 `4CC64C11110C`，AP2 为 `4CC64C22220C`）
3. 移动域保持 OpenWrt 默认 `4f57`
4. 重关联截止时间保持 OpenWrt 默认 `1000`
5. FT 协议选择 `FT over the Air`
6. 本地生成 PMK `勾选`
7. R0 密钥生存期保持 OpenWrt 默认 `10000`
8. R1 密钥持有者填写与 NAS ID 相同的内容（ AP1 为 `4CC64C11110C`，AP2 为 `4CC64C22220C`）
9. PMK R1 推送`勾选`
10. 外部 R0 密钥持有者列表和外部 R1 密钥持有者列表按照如下方法配置：

首先使用以下命令生成 128 位密钥（十六进制字符串）：

```shell
# Windows PowerShell
(Get-Random -Count 16 -InputObject (0..255) | ForEach-Object { $_.ToString("X2") }) -join ''
# Linux Bash
xxd -l 16 -p /dev/random
```

为两个 AP 各生成一个密钥：

|设备|MAC|NAS ID|密钥|
|:-:|:-:|:-:|:-:|
|AP 1|4C:C6:4C:11:11:0C|4CC64C11110C|`55ECD58B1AB9E42F7E921CA9A3A40CFF`|
|AP 2|4C:C6:4C:22:22:0C|4CC64C22220C|`F7DE6F1F57EC3B25867BA039340DD572`|

然后将表中内容以 `MAC,NAS ID,密钥` 的格式填入外部 R0 密钥持有者列表和外部 R1 密钥持有者列表中：

- 外部 R0 密钥持有者列表：
  - `4C:C6:4C:11:11:0C,4CC64C11110C,55ECD58B1AB9E42F7E921CA9A3A40CFF`
  - `4C:C6:4C:22:22:0C,4CC64C22220C,F7DE6F1F57EC3B25867BA039340DD572`
- 外部 R1 密钥持有者列表：
  - `4C:C6:4C:11:11:0C,4CC64C11110C,55ECD58B1AB9E42F7E921CA9A3A40CFF`
  - `4C:C6:4C:22:22:0C,4CC64C22220C,F7DE6F1F57EC3B25867BA039340DD572`

![接口配置-WLAN漫游](/assets/img/OpenWrtAPMesh/接口配置-WLAN漫游.png)

### 5.3 应用配置

上述所有内容配置完成后，点击右下角`保存`按钮。

在无线页面中，点击右下角`保存并应用`按钮。等待一段时间后，无线网络应当可以正常被搜索到。

至此，OpenWrt 有线中继 AP 模式 Mesh 组网教程结束。
