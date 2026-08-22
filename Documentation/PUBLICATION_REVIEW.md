# GitHub公開前確認レポート

作成日：2026年8月22日
対象：Zombie Attack

## 結論

現状のプロジェクトをそのままPublicへ公開することはできません。アセットの再配布条件が一式そろっておらず、ビルド生成物やユーザー名を含むログも存在するためです。

公開する場合は、`Source`、`Config`、プロジェクトファイル、README等に限定した**採用向けコード閲覧リポジトリ**とします。`Content`を含めないため、第三者がリポジトリ単体でゲームを起動することはできません。

## 推奨リポジトリ名

`ZombieAttack-UE5-CPP`

作品名、Unreal Engine、C++作品であることが短く伝わり、採用担当者が内容を判断しやすい名前です。

## 公開予定ファイル

- `.gitignore`
- `.gitattributes`
- `.clang-format`
- `README.md`
- `ZombieAttack.uproject`
- `Config/DefaultEngine.ini`
- `Config/DefaultGame.ini`
- `Config/DefaultInput.ini`
- `Source/`以下のC++、ヘッダー、Build.cs、Target.cs（111ファイル）
- `Documentation/CONTROLS.md`
- `Documentation/PUBLICATION_REVIEW.md`

## 除外する主なファイルと理由

- `Content/`：約9.32GB。Marketplace、Fab、Mixamo由来と思われる素材を含み、完全な再配布許諾を確認できないため
- `Binaries/`：約1.09GB。実行ファイル、DLL、PDB等の生成物
- `DerivedDataCache/`：約39.7MB。Unreal Engineのキャッシュ
- `Intermediate/`：約4.61GB。コンパイル中間生成物
- `Saved/`：約4.94GB。Cook済みデータ、ログ、Webキャッシュ、ユーザー環境情報
- `Build/`：約1.56MB。FileOpenOrderログにローカルの絶対パスとユーザー名が含まれるため
- `VideoCaptureBuild/`：約1.78GB。パッケージ済み実行データ
- `実行ファイル.zip`、`実行ファイル (2).zip`：各約1.68GB。大容量の配布用バイナリ
- `output/`：約7.9MB。文書生成・変換時の出力
- `.codex-temp/`：録画補助スクリプトにローカル絶対パス、設定ファイルを含むため
- `SourceAssets/`：元音源を含むため。CC0記録はあるがコード閲覧には不要
- `ThirdPartySource/`：第三者由来データの可能性があり、公開の必要性と権利を確認できないため
- `Documentation`内のDOCX、PDF、画像、ZIP、生成スクリプト：採用コード閲覧に不要なバイナリ・素材・作業ファイルのため

## 容量

- 元プロジェクト：23.43GiB、9,070ファイル
- 公開候補：121ファイル、813,690bytes（約0.776MiB、Git管理情報を除く）
- 最大ファイル：公開候補はGitHubの通常制限に十分収まる見込み

## 個人情報・機密情報の検査結果

- 公開候補の`Source`、`Config`、`uproject`から、APIキー、アクセストークン、秘密鍵、パスワードは検出されませんでした。
- `.codex-temp/Record-ZombieGameplay.ps1`にWindowsユーザー名を含む絶対パスが存在します。公開対象から除外します。
- `Build/Windows/FileOpenOrder/`のログにWindowsユーザー名と絶対パスが大量に存在します。`Build/`全体を除外します。
- `Saved/`にはログ、Webキャッシュ、Cook済みデータが含まれるため、内容にかかわらず全除外します。
- `.codex-temp/obs-websocket-config.json`には空の`server_password`項目があります。値は空ですが、ローカル設定のため除外します。

## 著作権・ライセンス上の注意点

以下のContentフォルダは、名前や構成から外部アセットを含むと判断できますが、プロジェクト内に完全なライセンス証跡がありません。

- `ProceduralNtr_vol2`
- `Rain_Forest`
- `PN_interactiveSpruceForest`
- `ModularBuildingSet`
- `Free_Spells`
- `CrosshairFreePack`
- `Fab`
- `Assets`内のキャラクター、モデル、アニメーション

音声については、`Content/Audio/CC0/README.md`と`SourceAssets/Audio/Music/LICENSE_The_Hunt.txt`にOpenGameArt、Kenney、CC0の出典記録があります。ただし、Content全体の権利確認が完了していないため、音声だけを例外公開することはしません。

Mixamoのモデル・アニメーション、Unreal Marketplace/Fabアセットは、取得条件と再配布条件を各サービスの購入・取得履歴で確認する必要があります。原則として、アセットのソース形式をPublicリポジトリへ含めない方針が安全です。

## READMEの状態

READMEには、作品概要、担当範囲、プレイヤー制御、敵AI、ゲームシステム、技術的工夫、問題と解決、主要コード5件、操作方法、確認方法、素材ライセンス、生成AI使用範囲を記載しました。

次の情報を制作者へ確認し、READMEへ反映しました。

- 制作期間：約1.5か月（2025年9月22日〜2025年11月7日）
- プレイ動画：https://drive.google.com/file/d/1ezazuAM-6t2Oc3je9asIbnAP9bNGCkFo/view
- `Source`以下に学校、チームメンバー、第三者の非公開コードは含まれていない
- 生成AI使用範囲の記載は制作者確認済み

## 採用担当者から見た改善点

1. プレイ動画URLをREADMEの冒頭へ追加する
2. 制作期間を明記する
3. 主要機能を30〜90秒で確認できる短い動画またはGIFを用意する
4. Content非公開でも設計が伝わるクラス図をPNGまたはMermaidでREADMEへ追加する
5. コード内に説明が曖昧なコメントが残っているため、主要5ファイルを中心にコメントを簡潔に見直す
6. GitHub ActionsでのビルドはContentとUnreal Engine環境が必要なため難しいが、少なくとも公開前にローカルのEditor Build成功日時をREADMEへ記載する
7. ゲーム本体を配布する場合は、GitHub Releasesではなく、権利確認済みのShippingビルドを別の配布先へ置く
8. `ZombieAttack.cpp`、`ZombieAttack.h`、`ZombieAttack.Build.cs`にEpic Gamesの著作権表記が残っているため、テンプレート由来か、本人のコードへ付ける表記として適切かを公開前に確認する

## Public公開可能と判断する条件

以下の条件を満たす場合、コード閲覧用Publicリポジトリとして公開可能と判断します。

- `.gitignore`が適用され、公開予定一覧以外がステージされていない
- `Content`、ビルド生成物、ログ、絶対パスを含むファイルがcommitされていない
- 制作期間、動画URL、生成AI使用範囲を制作者が確認した
- チームメンバーのコードや学校提供の非公開コードが`Source`へ含まれていないことを制作者が確認した
- READMEに「Content非公開のコード閲覧用」と明記されている
- 公開直前の秘密情報スキャン結果が0件である

## 公開前に制作者へ確認する事項

確認済み：

1. 制作期間
2. プレイ動画URL
3. `Source`以下のコードの公開権限
4. 生成AI使用範囲

未確認：

1. ソースコードへMIT等の再利用ライセンスを付けず、閲覧目的のPublic公開としてよいか
2. 最終内容を確認し、GitHubへ「公開してください」と明確に許可するか
