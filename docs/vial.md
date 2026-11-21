# Vial 対応ファームウェア

miniEC の全キーマップは Vial に対応しています。ビルド済みの HEX は
[GitHub Releases](https://github.com/goropikari/miniEC/releases) の Assets から入手できます。
配列名に合った `miniec_<keymap>.hex` を選んでください。

自分でビルドする場合は、リポジトリのルートで `make build` を実行します。
Vial-QMK と Python の依存関係は Docker イメージ内で準備されます。

HEX の書き込み、Vial でのロック解除、ブラウザでの閾値調整は
[キャリブレーション版の手順](../keyboards/miniec/readme.md)を参照してください。
