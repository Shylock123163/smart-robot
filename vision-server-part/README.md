# 鲁班猫与服务器视觉部分

这个目录把机器人视觉链路集中到一起：

- `lubancat-deploy/`
  - 鲁班猫端部署包，包含网页检测、串口下发、自启动脚本
- `sweep_server/`
  - 训练服务器代码、数据集与导出脚本
- `sweep_rknn/`
  - RKNN 导出与鲁班猫端推理脚本
- `sweep_cat/`
  - 杂物分类数据采集
- `wall_line_cat/`
  - 墙边距离分类数据采集

推荐先看：

1. `lubancat-deploy/README.md`
2. `sweep_server/README.md`
3. `sweep_server/docs/STEPS.md`
