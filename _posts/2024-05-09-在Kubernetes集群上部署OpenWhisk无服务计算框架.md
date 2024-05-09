---
title: 在 Kubernetes 集群上部署 OpenWhisk 无服务计算框架
date: 2024-05-09 14:21:13 +0800
categories: [软件]
tags: [linux, ubuntu, kubernetes, serverless]    # TAG names should always be lowercase
---

> 前置知识：[Kubernetes 一小时轻松入门](https://www.bilibili.com/video/BV1Se411r7vY/)

## 1 使用 Multipass 创建虚拟机集群

本文的 Kubernetes 集群中包含四台虚拟机。其中包括一台 master 节点和两台 worker 节点，以及一个用于持久化存储的 NFS 服务器。它们的配置如下：

|虚拟机名称|CPU 核心数|内存容量|硬盘空间|
|:-:|:-:|:-:|:-:|
|master|2|6GB|50GB|
|worker1|2|6GB|20GB|
|worker2|2|6GB|20GB|
|storage|2|4GB|50GB|

OpenWhisk 对节点配置的[最低要求](https://github.com/apache/openwhisk-deploy-kube/tree/master?tab=readme-ov-file#using-a-kubernetes-cluster-from-a-cloud-provider)

### 1.1 安装 [Multipass](https://multipass.run/)

```bash
sudo snap install multipass
```

### 1.2 创建虚拟机

```bash
multipass launch --name master --cpus 2 --memory 6G --disk 50G
multipass launch --name worker1 --cpus 2 --memory 6G --disk 20G
multipass launch --name worker2 --cpus 2 --memory 6G --disk 20G
multipass launch --name storage --cpus 2 --memory 4G --disk 50G
```

查看创建结果：

```bash
multipass list
```

```text
Name                    State             IPv4             Image
master                  Running           10.251.200.141   Ubuntu 24.04 LTS
storage                 Running           10.251.200.186   Ubuntu 24.04 LTS
worker1                 Running           10.251.200.142   Ubuntu 24.04 LTS
worker2                 Running           10.251.200.168   Ubuntu 24.04 LTS
```

#### 其他可能用到的命令

```bash
# 查看虚拟机资源使用情况
multipass info <instance-name>

# 将虚拟机移动至回收站
multipass delete <instance-name>

# 将虚拟机移出回收站
multipass recover <instance-name>

# 销毁虚拟机
multipass delete --purge <instance-name>

# 销毁所有虚拟机
multipass delete --purge --all
```

### 1.3 在 master 节点中安装 K3s

从宿主机连接到 master 节点：

```bash
multipass shell master
```

执行 K3s [安装脚本](https://docs.k3s.io/zh/quick-start#%E5%AE%89%E8%A3%85%E8%84%9A%E6%9C%AC)：

```bash
curl -sfL https://get.k3s.io | sh -

# 使用国内镜像
curl -sfL https://rancher-mirror.rancher.cn/k3s/k3s-install.sh | INSTALL_K3S_MIRROR=cn sh -
```

### 1.4 将 worker 节点加入集群

在宿主机中获取 master 节点的 IP 地址和 K3s 的 token：

```bash
MASTER_IP=$(multipass info master | grep IPv4 | awk '{print $2}')
TOKEN=$(multipass exec master sudo cat /var/lib/rancher/k3s/server/node-token)
```

在所有 worker 节点上执行 K3s [安装脚本](https://docs.k3s.io/zh/quick-start#%E5%AE%89%E8%A3%85%E8%84%9A%E6%9C%AC)：

```bash
for index in 1 2; do
    multipass exec worker${index} -- bash -c "curl -sfL https://get.k3s.io | K3S_URL=https://${MASTER_IP}:6443 K3S_TOKEN=${TOKEN} sh -"
done

# 使用国内镜像
for index in 1 2; do
    multipass exec worker${index} -- bash -c "curl -sfL https://rancher-mirror.rancher.cn/k3s/k3s-install.sh | INSTALL_K3S_MIRROR=cn K3S_URL=https://${MASTER_IP}:6443 K3S_TOKEN=${TOKEN} sh -"
done
```

查看集群节点状态：

```bash
multipass exec master sudo kubectl get nodes
```

```text
NAME      STATUS   ROLES                  AGE     VERSION
master    Ready    control-plane,master   10m     v1.29.4+k3s1
worker1   Ready    <none>                 2m12s   v1.29.4+k3s1
worker2   Ready    <none>                 2m4s    v1.29.4+k3s1
```

### 1.5 搭建 NFS 服务器

> [参考文档](https://github.com/apache/openwhisk-deploy-kube/blob/master/docs/k8s-nfs-dynamic-storage.md#set-up-the-nfs-server)

从宿主机连接到 storage 节点：

```bash
multipass shell storage
```

安装 NFS 服务：

```bash
sudo apt install nfs-kernel-server
```

创建 Kubernetes 集群数据存储目录：

```bash
sudo mkdir /var/nfs/kubedata -p
```

将目录的拥有者设置为 `nobody`：

```bash
sudo chown nobody: /var/nfs/kubedata
```

启用 NFS 服务：

```bash
sudo systemctl enable nfs-server.service
sudo systemctl start nfs-server.service
```

编辑 `/etc/exports` 文件：

```bash
sudo vim /etc/exports
```

在文件末尾添加集群数据存储目录的配置：

```text
/var/nfs/kubedata *(rw,sync,no_subtree_check,no_root_squash,no_all_squash)
```

应用配置文件：

```bash
sudo exportfs -rav
```

```text
exporting *:/var/nfs/kubedata
```

## 2 安装 NFS 客户端

为 Kubernetes 集群中的**所有**节点安装 NFS 客户端：

```bash
multipass exec master -- bash -c "sudo apt install nfs-common -y"
multipass exec worker1 -- bash -c "sudo apt install nfs-common -y"
multipass exec worker2 -- bash -c "sudo apt install nfs-common -y"
```

否则在创建 NFS Dynamic Storage 时，节点无法挂载 NFS 服务器的共享目录。

## 3 安装 Kubernetes 集群的包管理工具 Helm

从宿主机连接到 master 节点：

```bash
multipass shell master
```

使用 `snap` 安装 Helm：

```bash
sudo snap install helm --classic
```

## 4 安装 OpenWhisk Command-line Interface (wsk CLI)

从宿主机连接到 master 节点：

```bash
multipass shell master
```

下载并安装 wsk CLI 二进制文件：

```bash
mkdir ~/downloads && cd ~/downloads
wget https://github.com/apache/openwhisk-cli/releases/download/latest/OpenWhisk_CLI-latest-linux-amd64.tgz
tar -xvf OpenWhisk_CLI-latest-linux-amd64.tgz
sudo mv wsk /usr/local/bin/wsk
```

## 5 为 Kubernetes 集群部署 NFS Dynamic Storage

[参考文档](https://github.com/apache/openwhisk-deploy-kube/blob/master/docs/k8s-nfs-dynamic-storage.md#set-up-nfs-client-provisioner)中 `deployment.yaml` 文件所使用的 `quay.io/external_storage/nfs-client-provisioner` 镜像已停止维护。Pod 运行时会报错 *unexpected error getting claim reference: selfLink was empty, can't make reference* 。`selfLink` 字段在 Kubernetes 1.20 版本中被废弃。

根据[官方文档](https://kubernetes.io/zh-cn/docs/concepts/storage/storage-classes/#nfs)，使用 [NFS subdir](https://github.com/kubernetes-sigs/nfs-subdir-external-provisioner) 为 Kubernetes 集群提供 NFS Dynamic Storage 。

### 使用 Helm Chart 安装 NFS Subdirectory External Provisioner

> [参考文档](https://github.com/kubernetes-sigs/nfs-subdir-external-provisioner/blob/master/charts/nfs-subdir-external-provisioner/README.md)

从宿主机连接到 master 节点：

```bash
multipass shell master
```

添加 Helm Chart 仓库：

```bash
sudo helm repo add nfs-subdir-external-provisioner https://kubernetes-sigs.github.io/nfs-subdir-external-provisioner/
sudo helm repo update
```

以名称 `openwhisk-nfs` 部署 NFS Subdirectory External Provisioner，并指定 NFS 服务器的 [IP 地址](#12-创建虚拟机)和[共享目录](#15-搭建-nfs-服务器)：

```bash
sudo helm --kubeconfig /etc/rancher/k3s/k3s.yaml install openwhisk-nfs nfs-subdir-external-provisioner/nfs-subdir-external-provisioner \
    --set nfs.server=10.251.200.186 \
    --set nfs.path=/var/nfs/kubedata
```

在 K3s 中，需要使用 `--kubeconfig` 参数指定 kubeconfig 文件的路径，使得 Helm 能够[访问集群](https://docs.k3s.io/zh/cluster-access)。

安装成功后显示信息：

```text
NAME: openwhisk-nfs
LAST DEPLOYED: Wed May  8 18:15:14 2024
NAMESPACE: default
STATUS: deployed
REVISION: 1
TEST SUITE: None
```

#### 检查资源创建结果

```bash
sudo kubectl get clusterrole,clusterrolebinding,role,rolebinding | grep nfs
```

```text
clusterrole.rbac.authorization.k8s.io/openwhisk-nfs-nfs-subdir-external-provisioner-runner                   2024-05-08T10:15:14Z
clusterrolebinding.rbac.authorization.k8s.io/run-openwhisk-nfs-nfs-subdir-external-provisioner        ClusterRole/openwhisk-nfs-nfs-subdir-external-provisioner-runner     112s
role.rbac.authorization.k8s.io/leader-locking-openwhisk-nfs-nfs-subdir-external-provisioner   2024-05-08T10:15:14Z
rolebinding.rbac.authorization.k8s.io/leader-locking-openwhisk-nfs-nfs-subdir-external-provisioner   Role/leader-locking-openwhisk-nfs-nf-subdir-external-provisioner   112s
```

```bash
sudo kubectl get storageclass
```

```text
NAME                   PROVISIONER                                                   RECLAIMPOLICY   VOLUMEBINDINGMODE      ALLOWVOLUMEEXPANSION   AGE
local-path (default)   rancher.io/local-path                                         Delete          WaitForFirstConsumer   false                  7h16m
nfs-client             cluster.local/openwhisk-nfs-nfs-subdir-external-provisioner   Delete          Immediate              true                   3m26s
```

```bash
sudo kubectl get pods
```

```text
NAME                                                             READY   STATUS    RESTARTS   AGE
openwhisk-nfs-nfs-subdir-external-provisioner-54846ffcfb-26j8r   1/1     Running   0          5m35s
```

## 6 部署 OpenWhisk

从宿主机连接到 master 节点：

```bash
multipass shell master
```

### 6.1 为集群中的节点添加标签

在部署 OpenWhisk 之前，需要指明哪些节点运行 OpenWhisk 的控制面板（如：controller、kafka、zookeeper 和 CouchDB），哪些节点执行函数调用。

通过给节点添加[标签](https://github.com/apache/openwhisk-deploy-kube?tab=readme-ov-file#multi-worker-node-clusters)来指明节点的角色：

```bash
sudo kubectl label node master openwhisk-role=core
sudo kubectl label node worker1 openwhisk-role=invoker
sudo kubectl label node worker2 openwhisk-role=invoker
```

使用 `sudo kubectl describe nodes <node-name>` 命令查看节点的标签。

### 6.2 编写安装配置文件 `mycluster.yaml`

> [参考文档](https://github.com/apache/openwhisk-deploy-kube/blob/master/docs/configurationChoices.md)

`mycluster.yaml` 配置了 Kubernetes 集群的 Ingress 服务的[地址](#12-创建虚拟机)和端口号（默认 31001 ），用于 wsk CLI 连接 OpenWhisk 服务。

```yaml
whisk:
  ingress:
    type: NodePort
    apiHostName: 10.251.200.141
    apiHostPort: 31001

nginx:
  httpsNodePort: 31001

invoker:
  containerFactory:
    impl: "kubernetes"

# wurstmeister/kafka was removed from Docker Hub
# https://github.com/wurstmeister/kafka-docker/issues/744
kafka:
  imageName: "fatal69100/kafka"
  imageTag: "2.8.1"
```

将上述内容写入 `~/kubeyaml/mycluster.yaml` 文件中。

### 6.3 配置 wsk CLI

配置 wsk CLI 的 `apihost` 为 Ingress 服务的地址和端口号，`auth` 为 OpenWhisk 的 guest 用户的默认 token ：

```bash
sudo wsk property set --apihost 10.251.200.141:31001
sudo wsk property set --auth 23bc46b1-71f6-4ed5-8c54-816aa4f8c502:123zO3xZCLrMN6v2BKK1dXYFpXlPkccOFqm12CdAsMgRU4VrNZ9lyGVCGuMDGIwP
```

### 6.4 使用 Helm 部署 OpenWhisk

> [Helm Repository](https://github.com/apache/openwhisk-deploy-kube?tab=readme-ov-file#deploying-released-charts-from-helm-repository) 中的 OpenWhisk Chart 使用的 npm 镜像版本过低，会导致安装失败。因此使用 [GitHub](https://github.com/apache/openwhisk-deploy-kube?tab=readme-ov-file#deploying-from-git) 上的 OpenWhisk Chart 安装。

```bash
cd ~
git clone https://github.com/apache/openwhisk-deploy-kube.git
cd openwhisk-deploy-kube
sudo helm --kubeconfig /etc/rancher/k3s/k3s.yaml install owdev ./helm/openwhisk -n openwhisk --create-namespace -f ~/kubeyaml/mycluster.yaml
```

在 K3s 中，需要使用 `--kubeconfig` 参数指定 kubeconfig 文件的路径，使得 Helm 能够[访问集群](https://docs.k3s.io/zh/cluster-access)。

安装成功后显示信息：

```text
NAME: owdev
LAST DEPLOYED: Wed May  8 18:26:12 2024
NAMESPACE: openwhisk
STATUS: deployed
REVISION: 1
NOTES:
Apache OpenWhisk
Copyright 2016-2021 The Apache Software Foundation

This product includes software developed at
The Apache Software Foundation (http://www.apache.org/).

To configure your wsk cli to connect to it, set the apihost property
using the command below:

  $ wsk property set --apihost 10.251.200.141:31001

Your release is named owdev.

To learn more about the release, try:

  $ helm status owdev
  $ helm get owdev

Once the 'owdev-install-packages' Pod is in the Completed state, your OpenWhisk deployment is ready to be used.

Once the deployment is ready, you can verify it using:

  $ helm test owdev --cleanup
```

此时可以使用命令 `sudo kubectl get pvc,pv -A` 查看 NFS Dynamic Storage 的创建结果。STATUS 为 `Bound` 时表示创建成功。

使用命令 `sudo kubectl get pods -n openwhisk --watch` 查看集群容器部署的状态。当 `install-packages` Pod 的状态为 `Completed` 时，OpenWhisk 部署完成。

使用命令 `sudo wsk -i package list /whisk.system` 查看安装好的 OpenWhisk 包。`-i` 选项用于跳过 SSL 证书验证。

```text
packages
/whisk.system/messaging                                                shared
/whisk.system/alarms                                                   shared
/whisk.system/websocket                                                shared
/whisk.system/utils                                                    shared
/whisk.system/samples                                                  shared
/whisk.system/slack                                                    shared
/whisk.system/github                                                   shared
```

#### 其他可能用到的命令

```bash
# 查看 OpenWhisk 全部资源状态
sudo kubectl get all -n openwhisk

# 卸载 OpenWhisk
sudo helm --kubeconfig /etc/rancher/k3s/k3s.yaml uninstall owdev -n openwhisk
```

## 7 函数部署示例

> [OpenWhisk 文档](https://github.com/apache/openwhisk/tree/master/docs)

从宿主机连接到 master 节点：

```bash
multipass shell master
```

创建用于存储函数的目录：

```bash
mkdir ~/functions && cd ~/functions
```

### 7.1 通过 wsk CLI 调用函数

创建 Node.js 函数 `hello_cli.js`：

```javascript
function main(params) {
    var name = params.name || 'World';
    return {payload: 'Hello, ' + name + '!'};
}
```

使用 wsk CLI 部署函数：

```bash
sudo wsk -i action create hellocli hello_cli.js
```

调用函数：

```bash
sudo wsk -i action invoke hellocli --result
```

```text
{
    "payload": "Hello, World!"
}
```

传递参数：

```bash
sudo wsk -i action invoke hellocli --result --param name JavaScript
```

```text
{
    "payload": "Hello, JavaScript!"
}
```

### 7.2 通过 web URL 调用函数

> [参考文档](https://github.com/apache/openwhisk/blob/master/docs/webactions.md)

创建 Python 函数 `hello_web.py`：

```python
def main(args):
    return {
        "statusCode": 200,
        "headers": { "Content-Type": "application/json" },
        "body": args
    }
```

使用 wsk CLI 部署函数：

```bash
sudo wsk -i action create helloweb hello_web.py --web true
```

获取函数的 web URL：

```bash
sudo wsk -i action get helloweb --url
```

```text
https://10.251.200.141:31001/api/v1/web/guest/default/helloweb
```

在宿主机中使用 `curl` 命令调用函数，`-k` 选项用于跳过 SSL 证书验证：

```bash
curl -k https://10.251.200.141:31001/api/v1/web/guest/default/helloweb
```

```text
{
  "__ow_headers": {
    "accept": "*/*",
    "host": "controllers",
    "user-agent": "curl/7.81.0"
  },
  "__ow_method": "get",
  "__ow_path": ""
}
```

传递参数：

```bash
curl -k https://10.251.200.141:31001/api/v1/web/guest/default/helloweb?name=Python
```

```text
{
  "__ow_headers": {
    "accept": "*/*",
    "host": "controllers",
    "user-agent": "curl/7.81.0"
  },
  "__ow_method": "get",
  "__ow_path": "",
  "name": "Python"
}
```
