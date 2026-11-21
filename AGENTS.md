# miniEC 開発ガイド

miniEC（小石）は、左右それぞれに Pro Micro（ATmega32U4）を使う分割型の静電容量キーボードです。ファームウェアは Vial-QMK でビルドします。

## リポジトリの構成

- [keyboards/miniec/](keyboards/miniec/) にファームウェアがあります。`matrix.c` は ADC 測定、基準値の校正、打鍵判定、閾値調整用の Raw HID 通信を実装しています。`config.h` はピンや閾値の設定、`keyboard.json` はキー配置、`keymaps/` は配列ごとの定義です。
- [keyboards/miniec/tuning/](keyboards/miniec/tuning/) は WebHID を使う静的な閾値調整ツールです。接続・測定・保存の手順は [ファームウェアの README](keyboards/miniec/readme.md) にあります。
- [docs/build_guide.md](docs/build_guide.md) は組み立て・書き込み手順、[docs/maintainers.md](docs/maintainers.md) はコミット前の検査とリリース手順です。`3d_model/` と `plate/` にはケースやプレートの設計データがあります。

## ビルドと検証

- リポジトリ直下で `make build` を実行すると、Docker で全キーマップをビルドし、`build/miniec_<keymap>.hex` を生成します。1種類だけなら `./docker_build.sh miniec:qwerty` のように指定します。Vial-QMK のリビジョンは [Dockerfile](Dockerfile) に固定されています。
- ファームウェアを変更したら全キーマップをビルドし、Pro Micro のフラッシュ容量に収まることを確認します。`test` キーマップでは HID Console で測定値と打鍵イベントを確認できます。実機で確認できた内容と、ビルドやホスト上の確認結果は区別して報告します。
- 調整ツールをローカルで確認する場合は `python3 -m http.server 8000 --bind 127.0.0.1 --directory keyboards/miniec/tuning` を実行し、デスクトップ版 Chrome または Edge で開きます。`keyboard.json` のレイアウトを変更したら、`python3 keyboards/miniec/tuning/generate-layout.py` で配信用の `tuning/layout.json` を再生成します。
- コミット前に必ず `dprint fmt <変更ファイル>` を実行して整形します。整形後の差分を確認し、対象ファイルをステージしてからコミットします。Gitleaks フックの導入手順は [メンテナー向け手順](docs/maintainers.md) を参照してください。`build/` と生成した HEX は Git の管理対象外です。

## 変更時の注意

- マトリクスは片側3行×6列です。測定値や基準値の単位は ADC 生値を4で割った整数（0〜255）です。打鍵判定と、調整ツールや HID Console が表示するフィルタ前の測定値は別々に確認します。
- 閾値調整の Raw HID コマンドや応答形式を変える場合は、ファームウェアと `tuning.js` の組み合わせを確認します。押下・解放オフセットは左右それぞれの EEPROM に保存されるため、保存形式を変える場合は既存設定の引き継ぎも確認します。
