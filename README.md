# 小石 (miniEC)

![miniec](docs/img/miniec.png)

小石(miniEC) is a electrocapacitive split keyboard inspired by Manta40([KamoNanban](https://github.com/KamoNanban)), CorneEC([sekigon-gonnoc](https://github.com/sekigon-gonnoc)) and EC Helix([ginjake](https://github.com/ginjake)).

ビルドガイド

- [prototype, ~v1](https://github.com/goropikari/miniEC/blob/prototype/README.md)
- [v1~](docs/build_guide.md)

[販売ページ](https://pikarikbd.booth.pm/items/4799710)

## 閾値調整

デスクトップ版 Chrome または Edge で [miniEC 閾値調整（GitHub Pages）](https://goropikari.github.io/miniEC/) を開くと、USB で接続した miniEC のキーごとの押下・解放閾値を調整できます。対応ファームウェアの準備、接続、ローカルでの起動方法は[閾値調整の手順](keyboards/miniec/readme.md#ブラウザで閾値を調整する)を参照してください。

## キャリブレーション版

`make build` で QMK 公式の [`qmkfm/base_container`](https://hub.docker.com/r/qmkfm/base_container) をベースに、Vial 対応ファームウェアをコンテナ内でビルドできます。

## ファームウェアの入手

ビルド済みの HEX は [GitHub Releases](https://github.com/goropikari/miniEC/releases) の Assets から入手できます。

License

- keyboards/miniec/, build/, docker_build.sh: GNU General Public License v3.0
- 3d_model/, plate/: [Creative Commons Attribution 4.0 International License](https://creativecommons.org/licenses/by/4.0/)
