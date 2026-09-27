# 0AB1 / 0AB2 boundary tests

Тесты для проверки новой границы **32 параметра**.

## Файлы

- `F31TEST.cs` — 31 входной параметр + 1 return
- `F32TEST.cs` — 32 входных параметра + 1 return
- `F32NRET.cs` — 32 входных параметра без return

## Установка

Скопируй файлы `*.cs` из этой папки в:

`GTA SA\cleo\`

Запускай тесты по одному.

Порядок:
1. `F32NRET.cs`
2. `F31TEST.cs`
3. `F32TEST.cs`

## Ожидание

- F32NRET: `0AB1 32 PARAMS NO-RETURN PASS`
- F31TEST: `0AB1/0AB2 31 PARAMS PASS RESULT=31`
- F32TEST: `0AB1/0AB2 32 PARAMS PASS RESULT=32`

Если F32NRET и F31TEST работают, а F32TEST падает, проблема изолируется на границе 32 или на обработке return slot.

## Диагностика

Лог `0AB2` в `source/CCustomOpcodeSystem.cpp` теперь включает имя script. Это позволяет отличить тест от сторонних скриптов, например WeaponWheelUI.
