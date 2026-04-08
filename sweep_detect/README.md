# Sweep Detect

这个文件夹专门给鲁班猫放网页实时检测程序。

## 你要放进去的模型

把服务器训练出来的模型放到:

```text
models/best.pt
```

## 目录

```text
sweep_detect/
├─ lubancat_web_detect.py
├─ start_detect.sh
├─ requirements.txt
└─ models/
   └─ .gitkeep
```

## 鲁班猫运行

```bash
cd /home/cat/sweep_detect
python3 -m pip install -r requirements.txt
bash start_detect.sh
```

浏览器打开:

```text
http://鲁班猫IP:5005
```

## 说明

- 推理前会先裁剪 ROI
- ROI 可以在网页里拖动和拉伸
- 也可以直接改 `x1 y1 x2 y2`
- 默认端口是 `5005`
