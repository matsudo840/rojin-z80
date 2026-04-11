# 老人Z80 未解決の技術課題

本ドキュメントでは、現在直面している技術的な課題のみを扱います。
解決済みの課題（GPIOピンアサインの競合など）については、[pin_map.md](pin_map.md) や [implementation_guide.md](implementation_guide.md) の履歴を参照してください。

## 1. ソフトウェア実装の課題 (未着手)

### 1.1 クロック生成の安定性
- **課題**: ESP32から安定したクロックを供給しつつ、Wi-Fi通信を維持できるか。
- **対応予定**: Step 1 のPoCで検証する。

### 1.2 高速バスアクセス
- **課題**: Z80のメモリアクセス要求に対して、ESP32が十分に高速に応答できるか。
- **対応予定**: Step 2, 3 のPoCで検証し、必要であればアセンブリ言語での最適化を行う。

---

## 参考リンク
- [ESP32-S3-DevKitC-1 v1.1 公式ユーザーガイド](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html#hardware-reference)
