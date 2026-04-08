# 全流程步骤

## 1. 鲁班猫开 SSH

在鲁班猫执行:

```bash
cd /home/cat/new_lable
bash lubancat/setup_lubancat_ssh.sh
```

## 2. 电脑 SSH 连鲁班猫

电脑执行:

```bash
ssh cat@鲁班猫IP
```

## 3. 鲁班猫采图

鲁班猫执行:

```bash
cd /home/cat/new_lable
bash lubancat/start_collect.sh
```

电脑浏览器打开:

```text
http://鲁班猫IP:5004
```

然后点按钮把图保存进:

- `dataset/raw/clean`
- `dataset/raw/clutter`

## 4. 采集数量

- 第一版先做:
- `clean`: `150` 到 `300` 张
- `clutter`: `150` 到 `300` 张

如果后面误判多，再补到:

- `clean`: `800` 到 `1500` 张
- `clutter`: `800` 到 `1500` 张

快速采法:

- 先找 `15` 到 `20` 个不同机位
- 每个机位点一次 `连拍10张`
- 换角度后继续点

## 5. 上传到服务器

上传这些:

- `dataset/raw/`
- `server/`
- `docs/`

## 6. 服务器训练

```bash
conda activate yolov8
cd ~/new_lable
pip install -r server/requirements.txt
python server/scripts/split_dataset.py --raw-root dataset/raw --output-root dataset --val-ratio 0.2 --clean
python server/scripts/check_dataset.py --dataset-root dataset
python server/scripts/train_clutter_cls.py --data dataset --epochs 50 --imgsz 224 --batch 16 --device 0
```

## 7. 服务器网页检测

```bash
python server/web_detect.py --weights runs/clutter_cls/weights/best.pt --camera 0 --port 5003
```
