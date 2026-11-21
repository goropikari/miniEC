# メンテナー向け手順

## コミット前のシークレット検査

[pre-commit](https://pre-commit.com/#install) をインストールし、クローンごとにリポジトリ直下で `pre-commit install` を実行してください。以降は `git commit` のたびに Gitleaks がステージ済みの変更を検査し、シークレットを検出するとコミットを中止します。

## ファームウェアのリリース

[リリースワークフロー](../.github/workflows/release-firmware.yml) を含むコミットに `v1.2.3` のようなタグを付けて push すると、GitHub Actions が次の処理を実行します。

1. リポジトリ直下の `Dockerfile` から Vial のビルド用イメージを作成し、GitHub Container Registry (GHCR) の `ghcr.io/goropikari/miniec-builder` に実行ごとのタグで保存します。
2. GHCR からそのイメージをダイジェスト指定で取得し、全キーマップをビルドします。
3. タグの Release に各配列の `miniec_<keymap>.hex` と、使用したイメージのダイジェストとソースのコミットを記録した `miniec-build-info.txt` を添付します。

過去のリリースと同じビルド環境を使えるよう、リリースで使ったイメージは GHCR に残してください。GHCR が非公開の場合、ローカルから取得するにはパッケージの読み取り権限が必要です。
