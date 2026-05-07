# SNWriter Deb 打包说明

## 构建 DEB 包

运行打包脚本：

```bash
./package.sh
```

这将生成 `SNWriter_1.0.0_amd64.deb` 包。

## 安装 DEB 包

在目标机器上：

```bash
sudo dpkg -i SNWriter_1.0.0_amd64.deb
```

## 运行程序

安装后，可以直接运行：

```bash
SNWriter
```

或者：

```bash
/opt/SNWriter/SNWriter.sh
```

## 依赖说明

- DEB 包已包含所有 GTK/GTKMM 相关的动态库
- 唯一的外部依赖是 `adb`，需要单独安装：
  ```bash
  sudo apt install android-tools-adb
  ```

## 包结构

```
/
├── opt/
│   └── SNWriter/
│       ├── SNWriter          # 主程序
│       ├── SNWriter.sh       # 启动脚本（设置 LD_LIBRARY_PATH）
│       └── lib/              # 依赖的动态库
└── usr/
    └── local/
        └── bin/
            └── SNWriter -> /opt/SNWriter/SNWriter.sh
```
