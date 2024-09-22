---
title: 在 OpenWhisk 上部署 YOLO 以实现图片目标检测
date: 2024-09-22 14:38:04 +0800
categories: [软件]
tags: [linux, ubuntu, kubernetes, serverless, yolo]    # TAG names should always be lowercase
---

## 0 前言

本文详细记录了在 OpenWhisk 无服务计算框架上部署 YOLO 的过程。最终实现的效果是：将图片的 URL 作为参数，调用部署在 OpenWhisk 上的函数，函数会返回图片中检测到的目标的坐标、类别和置信度。

在阅读本文之前，你需要学会[在 Kubernetes 集群上部署 OpenWhisk 无服务计算框架](https://liuzhaoze.github.io/posts/%E5%9C%A8Kubernetes%E9%9B%86%E7%BE%A4%E4%B8%8A%E9%83%A8%E7%BD%B2OpenWhisk%E6%97%A0%E6%9C%8D%E5%8A%A1%E8%AE%A1%E7%AE%97%E6%A1%86%E6%9E%B6/)。

## 1 定制带有 YOLO 的 OpenWhisk Python runtime 容器

我们需要使用 `Dockerfile` 在 [openwhisk-runtime-python](https://github.com/apache/openwhisk-runtime-python) 的基础上添加 YOLO 环境。在本文中，以 Python 3.11 为例，构建带有 `YOLOv8n` 模型的容器。（YOLOv8 的模型文件可以在 [Hugging Face](https://huggingface.co/Ultralytics/YOLOv8/tree/main) 上下载）

首先在当前工作目录创建一个名为 `models` 的文件夹，以及一个名为 `Dockerfile` 的空白文件。然后将 `yolov8n.pt` 模型文件下载到 `models` 文件夹中。当前工作目录结构如下：

```text
.
├── Dockerfile
└── models
    └── yolov8n.pt

1 directory, 2 files
```

为了加快构建速度，我们可以提前下载好 [openwhisk/action-python-v3.11](https://hub.docker.com/r/openwhisk/action-python-v3.11/tags) 基础镜像：

```bash
docker pull openwhisk/action-python-v3.11:latest
```

在 `Dockerfile` 中添加如下内容，以 `openwhisk/action-python-v3.11:latest` 为基础镜像，安装 YOLO 所需的 Python 库，并将模型文件复制到容器中：

```Dockerfile
FROM openwhisk/action-python-v3.11:latest
# 安装 OpenCV 图形渲染所需的 OpenGL 库
RUN apt update && apt install -y libgl1-mesa-glx
# 此处安装 CPU 版的 PyTorch 为例
RUN pip install torch torchvision --index-url https://download.pytorch.org/whl/cpu
# 安装 YOLO 库
RUN pip install ultralytics
# 创建模型文件夹
RUN mkdir -p /models
# 将模型文件复制到容器中
COPY ./models/* /models/
```

在 `Dockerfile` 所在目录下执行以下命令构建容器：

```bash
docker build -t openwhisk-yolov8n-runtime:1.0.0 .
```

## 2 测试定制 OpenWhisk Python runtime 容器（可选）

在将容器应用于 OpenWhisk 之前，可以根据[本文](https://github.com/apache/openwhisk-runtime-python/blob/master/tutorials/local_build.md)提供的方法测试单个容器是否能够正常工作。

首先使用以下命令运行容器：

```bash
docker run -d -p 127.0.0.1:80:8080/tcp --name=bloom_whisker --rm -it openwhisk-yolov8n-runtime:1.0.0
```

使用 `docker ps -a` 命令查看容器运行状态。

然后创建一个名为 `python-data-init-params.json` 的文件，用于指定初始化函数的参数：

```json
{
    "value": {
        "name": "yoloTest",
        "main": "main",
        "binary": false,
        "code": "def main(args):\n\timport json\n\tfrom ultralytics import YOLO\n\tsource = args.get('url', None)\n\tmodel = YOLO('/models/yolov8n.pt')\n\tresults = model(source)\n\treturn {'result': str([json.loads(r.to_json()) for r in results])}"
    }
}
```

其中 `code` 关键字所对应的代码实现了调用 YOLO 模型对 URL 指向的图片进行目标检测的功能：

```python
def main(args):
    import json

    from ultralytics import YOLO

    source = args.get("url", None)
    model = YOLO("/models/yolov8n.pt")
    results = model(source)
    return {"result": str([json.loads(r.to_json()) for r in results])}
```

在 `python-data-init-params.json` 所在目录执行 `curl` 命令向容器中的函数初始化 API 发送请求：

```bash
curl -d "@python-data-init-params.json" -H "Content-Type: application/json" http://localhost/init
```

容器会返回初始化结果：

```json
{"ok":true}
```

最后创建一个名为 `python-data-run-params.json` 的文件，用于指定函数调用的参数：

```json
{
    "value": {
        "url": "https://ultralytics.com/images/bus.jpg"
    }
}
```

在 `python-data-run-params.json` 所在目录执行 `curl` 命令向容器中的函数调用 API 发送请求：

```bash
curl -d "@python-data-run-params.json" -H "Content-Type: application/json" http://localhost/run
```

容器会返回函数执行结果：

```json
{"result": "[[{'name': 'bus', 'class': 5, 'confidence': 0.87345, 'box': {'x1': 22.87122, 'y1': 231.27727, 'x2': 805.00256, 'y2': 756.84039}}, {'name': 'person', 'class': 0, 'confidence': 0.86569, 'box': {'x1': 48.55047, 'y1': 398.55219, 'x2': 245.34557, 'y2': 902.7027}}, {'name': 'person', 'class': 0, 'confidence': 0.85284, 'box': {'x1': 669.4729, 'y1': 392.18616, 'x2': 809.72003, 'y2': 877.03546}}, {'name': 'person', 'class': 0, 'confidence': 0.82522, 'box': {'x1': 221.51733, 'y1': 405.79865, 'x2': 344.97067, 'y2': 857.53662}}, {'name': 'person', 'class': 0, 'confidence': 0.26111, 'box': {'x1': 0.0, 'y1': 550.52502, 'x2': 63.00694, 'y2': 873.44293}}, {'name': 'stop sign', 'class': 11, 'confidence': 0.25507, 'box': {'x1': 0.05816, 'y1': 254.4594, 'x2': 32.5574, 'y2': 324.87415}}]]"}
```

对于使用 `wget` 和 `postman` 等工具的测试方法，请参考[该文章](https://github.com/apache/openwhisk-runtime-python/blob/master/tutorials/local_build.md)。

## 3 将定制镜像推送到 Docker Hub

在镜像名中添加 Docker Hub 用户名作为 Tag ：

```bash
docker tag openwhisk-yolov8n-runtime:1.0.0 <DockerHubUser>/openwhisk-yolov8n-runtime:1.0.0

# Example
docker tag openwhisk-yolov8n-runtime:1.0.0 liuzhaoze/openwhisk-yolov8n-runtime:1.0.0
```

将镜像推送到 Docker Hub：

```bash
docker push <DockerHubUser>/openwhisk-yolov8n-runtime:1.0.0

# Example
docker push liuzhaoze/openwhisk-yolov8n-runtime:1.0.0
```

## 4 在 OpenWhisk 上配置定制 Python runtime 容器

管理 OpenWhisk Action 所用镜像的文件有 [runtimes.json](https://github.com/apache/openwhisk-deploy-kube/blob/master/helm/openwhisk/runtimes.json) 和 [runtimes-minimal-travis.json](https://github.com/apache/openwhisk-deploy-kube/blob/master/helm/openwhisk/runtimes-minimal-travis.json) 。

首先将这两个文件进行备份：

```bash
cp runtimes.json runtimes.json.bak
cp runtimes-minimal-travis.json runtimes-minimal-travis.json.bak
```

在 `runtimes-minimal-travis.json` 中，修改 [Python](https://github.com/apache/openwhisk-deploy-kube/blob/master/helm/openwhisk/runtimes-minimal-travis.json#L20) 部分的 [image](https://github.com/apache/openwhisk-deploy-kube/blob/master/helm/openwhisk/runtimes-minimal-travis.json#L24) 为本文中定制的镜像：

```json
"python": [
    {
        "kind": "python:3",
        "default": true,
        "image": {
            "prefix": "liuzhaoze",               // Modify this line
            "name": "openwhisk-yolov8n-runtime", // Modify this line
            "tag": "1.0.0"                       // Modify this line
        },
        "deprecated": false,
        "attached": {
            "attachmentName": "codefile",
            "attachmentType": "text/plain"
        }
    }
],
```

在 `runtimes.json` 中，修改 [Python](https://github.com/apache/openwhisk-deploy-kube/blob/master/helm/openwhisk/runtimes.json#L46) 部分的 [image](https://github.com/apache/openwhisk-deploy-kube/blob/master/helm/openwhisk/runtimes.json#L50) 为本文中定制的镜像。并且可以参照 Node.js 部分的 [stemCells](https://github.com/apache/openwhisk-deploy-kube/blob/master/helm/openwhisk/runtimes.json#L17) 配置定制 Python runtime 容器的 pre-warm 参数，以提高函数的冷启动性能：

```json
"python": [
    {
        "kind": "python:3",
        "default": true,
        "image": {
            "prefix": "liuzhaoze",               // Modify this line
            "name": "openwhisk-yolov8n-runtime", // Modify this line
            "tag": "1.0.0"                       // Modify this line
        },
        "deprecated": false,
        "attached": {
            "attachmentName": "codefile",
            "attachmentType": "text/plain"
        },
        "stemCells": [
            {
                "initialCount": 2,
                "memory": "1024 MB",
                "reactive": {
                    "minCount": 1,
                    "maxCount": 4,
                    "ttl": "2 minutes",
                    "threshold": 1,
                    "increment": 1
                }
            }
        ]
    }
]
```

> 注：因为函数涉及 YOLO 模型的推理，所以应当配置较大的内存。

## 5 使用 Helm 部署 OpenWhisk

详细部署方法参见[该文章](https://liuzhaoze.github.io/posts/%E5%9C%A8Kubernetes%E9%9B%86%E7%BE%A4%E4%B8%8A%E9%83%A8%E7%BD%B2OpenWhisk%E6%97%A0%E6%9C%8D%E5%8A%A1%E8%AE%A1%E7%AE%97%E6%A1%86%E6%9E%B6/#6-%E9%83%A8%E7%BD%B2-openwhisk)。本文为了满足 YOLO 模型的推理需求，需要通过 `mycluster.yaml` 配置文件放宽 OpenWhisk 对容器的内存限制：

```yaml
whisk:
  ingress:
    type: NodePort
    apiHostName: 10.164.210.70
    apiHostPort: 31001
  limits:
    actions:
      time:
        min: "100ms"
        max: "5m"
        std: "2m"
      memory:
        min: "512m"
        max: "2048m"
        std: "768m"
      concurrency:
        min: 1
        max: 4
        std: 2

nginx:
  httpsNodePort: 31001

invoker:
  containerFactory:
    impl: "kubernetes"
```

所有可配置项参见[该文件](https://github.com/apache/openwhisk-deploy-kube/blob/master/helm/openwhisk/values.yaml)。[该文件](https://github.com/apache/openwhisk-deploy-kube/blob/master/helm/openwhisk/values.yaml)中的所有参数都可以通过 `mycluster.yaml` 文件进行覆盖。

> 注：此处对函数调用的时间限制和内存限制都进行了放宽，以适应 YOLO 模型的推理需求。同时也调整了并发数。

配置文件编写完成后，执行以下命令部署 OpenWhisk：

```bash
sudo helm --kubeconfig /etc/rancher/k3s/k3s.yaml install owdev ./helm/openwhisk -n openwhisk --create-namespace -f ~/kubeyaml/mycluster.yaml
```

> 注：`owdev-install-packages` 的初始化可能需要较长时间。

## 6 测试

新建一个名为 `yoloTest` 的文件夹，并在文件夹中创建一个名为 `__main__.py` 的文件，目录结构如下：

```text
.
└── yoloTest
    └── __main__.py

1 directory, 1 file
```

将该文件下的内容打包为 `yoloTest.zip` ：

```bash
zip -j -r yoloTest.zip yoloTest/
```

目录结构如下：

```text
.
├── yoloTest
│   └── __main__.py
└── yoloTest.zip

1 directory, 2 files
```

使用 `wsk` 将函数部署到 OpenWhisk ：

```bash
sudo wsk -i action create yoloTest yoloTest.zip --kind python:3
```

> 上述函数部署方法参见[此处](https://github.com/apache/openwhisk/blob/master/docs/actions-python.md#packaging-python-actions-in-zip-files)。

调用函数进行测试：

```bash
$ sudo wsk -i action invoke --result yoloTest --param url "https://ultralytics.com/images/bus.jpg"
{
    "result": "[[{'name': 'bus', 'class': 5, 'confidence': 0.87345, 'box': {'x1': 22.87122, 'y1': 231.27727, 'x2': 805.00256, 'y2': 756.84039}}, {'name': 'person', 'class': 0, 'confidence': 0.86569, 'box': {'x1': 48.55047, 'y1': 398.55219, 'x2': 245.34557, 'y2': 902.7027}}, {'name': 'person', 'class': 0, 'confidence': 0.85284, 'box': {'x1': 669.4729, 'y1': 392.18616, 'x2': 809.72003, 'y2': 877.03546}}, {'name': 'person', 'class': 0, 'confidence': 0.82522, 'box': {'x1': 221.51733, 'y1': 405.79865, 'x2': 344.97067, 'y2': 857.53662}}, {'name': 'person', 'class': 0, 'confidence': 0.26111, 'box': {'x1': 0.0, 'y1': 550.52502, 'x2': 63.00694, 'y2': 873.44293}}, {'name': 'stop sign', 'class': 11, 'confidence': 0.25507, 'box': {'x1': 0.05816, 'y1': 254.4594, 'x2': 32.5574, 'y2': 324.87415}}]]"
}

$ sudo wsk -i action invoke --result yoloTest --param url "https://pic.nximg.cn/20121105/3915497_114356538176_2.jpg"
{
    "result": "[[{'name': 'car', 'class': 2, 'confidence': 0.85192, 'box': {'x1': 898.85858, 'y1': 427.0462, 'x2': 1023.55389, 'y2': 585.66827}}, {'name': 'person', 'class': 0, 'confidence': 0.8474, 'box': {'x1': 334.18884, 'y1': 427.64355, 'x2': 375.27444, 'y2': 540.27966}}, {'name': 'person', 'class': 0, 'confidence': 0.76925, 'box': {'x1': 402.41656, 'y1': 428.69043, 'x2': 438.98901, 'y2': 544.12128}}, {'name': 'car', 'class': 2, 'confidence': 0.73301, 'box': {'x1': 488.2338, 'y1': 444.8974, 'x2': 531.84363, 'y2': 480.46832}}, {'name': 'car', 'class': 2, 'confidence': 0.69895, 'box': {'x1': 521.92609, 'y1': 437.05988, 'x2': 625.97961, 'y2': 523.72333}}, {'name': 'person', 'class': 0, 'confidence': 0.69167, 'box': {'x1': 269.52325, 'y1': 433.7623, 'x2': 310.35693, 'y2': 561.16443}}, {'name': 'stop sign', 'class': 11, 'confidence': 0.63035, 'box': {'x1': 0.17458, 'y1': 319.77719, 'x2': 32.50227, 'y2': 368.97501}}, {'name': 'truck', 'class': 7, 'confidence': 0.55768, 'box': {'x1': 676.94666, 'y1': 443.49286, 'x2': 760.41406, 'y2': 497.97198}}, {'name': 'car', 'class': 2, 'confidence': 0.48876, 'box': {'x1': 677.42737, 'y1': 443.46967, 'x2': 760.53748, 'y2': 498.47299}}, {'name': 'person', 'class': 0, 'confidence': 0.28551, 'box': {'x1': 272.51321, 'y1': 433.55768, 'x2': 311.13678, 'y2': 528.31403}}]]"
}

$ sudo wsk -i action invoke --result yoloTest --param url "https://p6.itc.cn/q_70/images03/20201219/a833a5d49b49463aab8ccd526cc5a03b.jpeg"
{
    "result": "[[{'name': 'bird', 'class': 14, 'confidence': 0.89198, 'box': {'x1': 9.80772, 'y1': 181.17976, 'x2': 146.61261, 'y2': 306.78497}}, {'name': 'bird', 'class': 14, 'confidence': 0.87933, 'box': {'x1': 332.15588, 'y1': 220.49394, 'x2': 421.29996, 'y2': 318.85773}}, {'name': 'bird', 'class': 14, 'confidence': 0.86041, 'box': {'x1': 390.96136, 'y1': 263.45441, 'x2': 503.65137, 'y2': 542.60962}}, {'name': 'bird', 'class': 14, 'confidence': 0.84437, 'box': {'x1': 86.15939, 'y1': 337.59201, 'x2': 292.3508, 'y2': 546.31122}}, {'name': 'bird', 'class': 14, 'confidence': 0.8341, 'box': {'x1': 505.09445, 'y1': 231.21881, 'x2': 652.68408, 'y2': 307.17935}}, {'name': 'bird', 'class': 14, 'confidence': 0.82347, 'box': {'x1': 196.95297, 'y1': 209.44893, 'x2': 286.32193, 'y2': 340.08099}}, {'name': 'bird', 'class': 14, 'confidence': 0.8195, 'box': {'x1': 283.17896, 'y1': 241.47614, 'x2': 332.95972, 'y2': 309.55136}}, {'name': 'teddy bear', 'class': 77, 'confidence': 0.80825, 'box': {'x1': 633.72913, 'y1': 340.84668, 'x2': 813.52075, 'y2': 546.66455}}, {'name': 'bird', 'class': 14, 'confidence': 0.61646, 'box': {'x1': 107.19971, 'y1': 267.25839, 'x2': 172.39574, 'y2': 352.75092}}, {'name': 'bird', 'class': 14, 'confidence': 0.56528, 'box': {'x1': 227.52942, 'y1': 315.9144, 'x2': 408.51706, 'y2': 547.39673}}, {'name': 'bird', 'class': 14, 'confidence': 0.53396, 'box': {'x1': 733.82507, 'y1': 70.97623, 'x2': 891.36371, 'y2': 182.78658}}, {'name': 'bird', 'class': 14, 'confidence': 0.48765, 'box': {'x1': 0.09951, 'y1': 291.78284, 'x2': 121.53814, 'y2': 547.29529}}, {'name': 'bird', 'class': 14, 'confidence': 0.42327, 'box': {'x1': 505.58591, 'y1': 305.65659, 'x2': 614.04285, 'y2': 426.71091}}, {'name': 'teddy bear', 'class': 77, 'confidence': 0.32751, 'box': {'x1': 502.99014, 'y1': 298.87802, 'x2': 672.08838, 'y2': 547.76776}}, {'name': 'bird', 'class': 14, 'confidence': 0.30638, 'box': {'x1': 0.04798, 'y1': 176.87833, 'x2': 50.1236, 'y2': 218.95305}}, {'name': 'bird', 'class': 14, 'confidence': 0.25292, 'box': {'x1': 504.33881, 'y1': 299.50067, 'x2': 666.71484, 'y2': 548.33795}}]]"
}
```
