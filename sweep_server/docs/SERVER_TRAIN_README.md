# 服务器训练说明

## 训练环境

服务器直接使用:

```bash
conda activate yolov8
```

已确认环境包含:

- Python `3.12.9`
- torch `2.6.0+cu124`
- CUDA 可用
- ultralytics `8.3.89`

## 需要上传到服务器的内容

至少上传:

- `dataset/raw/`
- `server/`
- `docs/`

## 训练步骤

```bash
conda activate yolov8
cd ~/new_lable
pip install -r server/requirements.txt
python server/scripts/split_dataset.py --raw-root dataset/raw --output-root dataset --val-ratio 0.2 --clean
python server/scripts/check_dataset.py --dataset-root dataset
python server/scripts/train_clutter_cls.py --data dataset --epochs 50 --imgsz 224 --batch 16 --device 0
```

## 训练结果

输出目录:

```text
runs/clutter_cls/
```

模型文件:

```text
runs/clutter_cls/weights/best.pt
```

## 网页实时检测

```bash
python server/web_detect.py --weights runs/clutter_cls/weights/best.pt --camera 0 --port 5003
```

浏览器打开:

```text
http://服务器IP:5003
```
