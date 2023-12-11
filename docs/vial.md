vial 対応は試験的です。今後もサポートするかは未定です。
[vial 0.7.1](https://github.com/vial-kb/vial-gui/releases/tag/v0.7.1) で動作を確認しています。


## ローカルで flash する場合
```bash
git clone --depth 1 --recurse-submodules --shallow-submodules -b v1 https://github.com/goropikari/miniEC
cd miniEC
python3 -m pip install -r requirements-vial.txt
ln -s $(pwd)/keyboards/miniec vial-qmk/keyboards/miniec
cd vial-qmk
make miniec:vial:flash
```

## Docker を使用する場合
```bash
git clone --depth 1 -b v1 https://github.com/goropikari/miniEC
cd miniEC
QMK_VERSION=vial-qmk-20231210153557 ./docker_build.sh miniec:vial:flash
```

## QMK Toolbox を使用する場合
[build/miniec_vial.hex](../build/miniec_vial.hex) を [qmk toolbox](https://github.com/qmk/qmk_toolbox) 使用して flash することも出来ます。
