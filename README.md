# unreal deneme

UE 5.8 ile C++ tabanlı, LAN üzerinden birlikte oynanabilen bir shooter ve parkur prototipi. Dalga sistemi, sunucu denetimli silahlar, şarjör/yedek mühimmat, yeniden doğma ve engel aşma içerir. Bu kişisel bir prototiptir.

An Unreal Engine 5.8 C++ co-op shooter and parkour prototype with LAN play, server-authoritative combat, enemy waves, ammunition, respawning and vaulting.

## Kurulum / Setup

1. Unreal Engine 5.8 ve Visual Studio C++ oyun geliştirme araçlarını kurun. / Install UE 5.8 and Visual Studio's C++ game development tools.
2. `git clone https://github.com/ahmetefesensoy/unreal-deneme.git C:\src\unreal-deneme` ve proje içinde `git lfs pull` çalıştırın. Motorun MSVC/UBA zinciri bu bilgisayarda Türkçe karakterli yolda derlenemedi; ASCII klasör adı kullanın. / Clone into an ASCII-only path: the tested toolchain failed to build in a path containing Turkish characters.
3. [Soul: City](https://www.fab.com/listings/dd77fee6-0ad2-41ce-b32c-09300c24c9f3) ve [Paragon: Murdock](https://www.fab.com/listings/89ae5876-3350-432e-882b-7d617e8c7e6f) paketlerini kendi Fab hesabınızdan edinin; içerikleri `Content/SoulCity` ve `Content/ParagonMurdock` yollarına ekleyin. / Obtain both packs under your own applicable licenses and import them at these exact paths.
4. PowerShell'de proje kökünden derleyin / Build from the repository root:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' SoulCityGameEditor Win64 Development "-Project=$PWD\SoulCityGame.uproject" -WaitMutex -NoHotReloadFromIDE
```

5. `SoulCityGame.uproject` dosyasını açın. Editörde **Python Script** menüsünden `Scripts/bootstrap_project.py` çalıştırın. Bu işlem yerel haritaları ve özgün sesleri üretir; var olan üretilmiş haritaları yeniler. / Open the project and execute `Scripts/bootstrap_project.py` in the editor's Python script menu. It regenerates local maps and original sounds, overwriting earlier generated maps.
6. `/Game/SoulCityGame/Maps/L_CityCombat` haritasını açıp Play'e basın. / Open this map and press Play.

Motorunuz farklı bir dizindeyse ilk komuttaki motor yolunu değiştirin. Hazır binary veya asset paketi bu repoda bulunmaz. / Adjust the engine path for your installation. This repository contains source; licensed packs and generated maps are installed locally.

## Kontroller / Controls

| Tuş / Key | İşlev / Action |
|---|---|
| WASD, fare / mouse | Hareket ve bakış / Move and look |
| Shift, Space | Koşma, zıplama/engel aşma / Sprint, jump/vault |
| E | Silah veya mühimmat al / Pick up weapon or ammunition |
| Sol/sağ fare / LMB/RMB | Ateş / dürbün / Fire / scope |
| R, V | Şarjör / dürbün aç-kapat / Reload / toggle scope |
| F6, F7 | Savaş / parkur haritası / Combat / exploration |
| F5 | LAN sunucusu aç / Start a LAN listen server |
| H | Son güvenli konuma dön / Recover to last safe position |

Diğer istemcide konsoldan `JoinCity <host LAN IP>:7777` kullanın. / Run this command in the other client's console. [Detailed gameplay and test flags](docs/gameplay.en.md).

## Mimari / Architecture

`Source/CityCombat` hareket, silah, bot, HUD ve sunucu denetimini; `Scripts` editör içi harita/asset üretimini; `SourceAssets/Audio` özgün sentezlenmiş sesleri içerir. / The C++ module owns gameplay and replication; editor scripts rebuild maps and generated assets.

## Doğrulama / Validation

Editörde **Session Frontend → Automation → CityCombat.Gameplay.AuthorityRules** testini çalıştırın. / Run this automation test in the editor. Build and validation evidence is recorded in [validation](docs/validation.md).

## Yol haritası / Roadmap

Animation layering, online session discovery and playtesting. Prototipin rekabetçi veya üretim ortamı için doğrulandığı iddia edilmez. / Competitive or production readiness has not been established.

## Lisans ve krediler / License and credits

Özgün kaynak kodu ve sentezlenmiş sesler MIT lisanslıdır. Epic Games motoru, Fab paketleri ve bu paketlerden türeyen içerikler kendi koşullarına tabidir; MIT kapsamına girmez. / Original source and synthesized audio use MIT. Epic's engine, asset packs and derivatives retain their own terms. See [asset provenance](docs/assets.md).
