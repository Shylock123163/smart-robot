from __future__ import annotations

import argparse
from pathlib import Path

from ultralytics import YOLO


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Train bed clutter classification model")
    parser.add_argument("--data", type=Path, required=True, help="dataset root with train/val folders")
    parser.add_argument("--weights", default="yolo11n-cls.pt", help="base classification model")
    parser.add_argument("--epochs", type=int, default=50, help="training epochs")
    parser.add_argument("--imgsz", type=int, default=224, help="image size")
    parser.add_argument("--batch", type=int, default=16, help="batch size")
    parser.add_argument("--device", default="cpu", help="device such as cpu or 0")
    parser.add_argument("--project", type=Path, default=Path("runs"), help="output project dir")
    parser.add_argument("--name", default="clutter_cls", help="run name")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    args.project.mkdir(parents=True, exist_ok=True)

    model = YOLO(args.weights)
    model.train(
        data=str(args.data),
        epochs=args.epochs,
        imgsz=args.imgsz,
        batch=args.batch,
        device=args.device,
        project=str(args.project),
        name=args.name,
    )


if __name__ == "__main__":
    main()
