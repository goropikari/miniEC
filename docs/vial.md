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
QMK_VERSION=vial-qmk-20231210153557 ./docker_build.sh miniec:vial:flash
```

## QMK Toolbox を使用する場合
[build/miniec_vial.hex](../build/miniec_vial.hex) を [qmk toolbox](https://github.com/qmk/qmk_toolbox) 使用して flash することも出来ます。
