# Validation / Doğrulama

Source syntax and full-history secret scans are performed before publication. The GitHub workflow checks Python syntax and the exclusion of raw asset packs; it cannot build Unreal without an engine installation.

On 2026-11-12, a fresh local clone in an ASCII-only Windows directory passed `SoulCityGameEditor Win64 Development` with UE 5.8.4 and MSVC 14.44. Local, separately acquired Soul: City and Paragon: Murdock packs were linked into the ignored Content directory. `bootstrap_project.py` regenerated the maps/audio/materials and completed with zero errors; it logged ten warnings, including initial missing generated assets, an old Paragon Rig class and a missing Recast mesh during map construction.

The command-line `CityCombat.Gameplay.AuthorityRules` automation test passed (one test). The test checks gameplay authority rules; it does not prove visual quality, packaged-game delivery or Internet multiplayer behavior. Fab acquisition and asset import remain prerequisites for a playable local clone.

The generated combat map also passed the 14-second headless `-CityBattleTest` smoke: assets/sounds, three live bots, all three bot movement checks, ammunition/reload, round progression and safe-position recovery. Every `CITY_BATTLE_*` and `CITY_BOT_MOVEMENT` check reported PASS. This was a local single-instance test.

Temiz klon ASCII klasörde derlendi; harita üretimi ve bir gameplay automation testi geçti. Türkçe karakterli yolda MSVC/UBA geçici PCH dosyasını açamadı. Bu nedenle README ASCII klasör yolu önerir. Görsel kalite ve paketlenmiş oyun bu testten çıkarılmaz.
