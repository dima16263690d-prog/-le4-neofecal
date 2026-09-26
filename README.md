# CLEO 4.4.4 Unofficial Patch

## Что это

**CLEO 4.4.4 Unofficial Patch** — независимая разработка на базе исходного CLEO 4.4.4 для GTA San Andreas.

Цель проекта — сохранить старую совместимость CLEO 4 и одновременно привести внутреннюю реализацию к современному окружению разработки. Это не CLEO 5 и не официальный выпуск CLEO Library.

### Основные принципы

1. Сохранять совместимость со старыми `.cs` / `.cs4` скриптами.
2. Не ломать существующие opcode и формат скриптов без веской причины.
3. Исправлять ошибки и проблемы старого ядра постепенно.
4. Использовать современный Visual Studio / MSVC и актуальный plugin-sdk.
5. Добавлять собственные API и диагностические возможности отдельными слоями.
6. Изменения, которые могут повлиять на старое поведение, по возможности делать опциональными.

## Сборка

Основной проект:

`CLEO4.sln`

Конфигурация проекта ориентирована на:

- Visual Studio 2022;
- MSVC v143;
- Win32;
- GTA San Andreas 1.0 US;
- plugin-sdk.

## Внутри каталога

- `source/` — ядро и исходный код;
- `cleo_sdk/` — SDK;
- `demo_plugins/` — примеры плагинов;
- `FileSystemOperations/`, `IniFiles/`, `IntOperations/` — компоненты;
- `third-party/` — сторонние зависимости;
- `CHANGELOG.md` — исторические изменения базы CLEO 4.



## Текущий статус: несколько custom-потоков

В ядре исправлен жизненный цикл дочерних `CCustomScript`, создаваемых из label другого custom-script.

### Что сделано

- Добавлено явное владение буфером кода через `ownedBuffer`.
- Обычный `.cs` владеет только своим выделенным буфером.
- Дочерний script, созданный из label, использует буфер родительского script без попытки освободить его.
- Инициализирована явная связь `parentThread` ↔ `childThreads`.
- При удалении child он отсоединяется от parent.
- При удалении parent дочерние scripts удаляются рекурсивно безопасным способом.
- `RemoveAllCustomScripts()` использует тот же безопасный lifecycle.
- Исправлено дублирование объявления `parentThread` после рефакторинга.

### Параллельное выполнение через 0E6F

Основным механизмом запуска дополнительного custom-потока выбран:

```
0E6F: stream_custom_script_from_label @LABEL
```

Проверено на **GTA San Andreas 1.0.0.0 US**.

Практические тесты подтвердили:

- один parent + один child;
- один parent + два child;
- одновременное выполнение нескольких child;
- разные стартовые label для каждого child;
- повторное создание child после загрузки сохранения;
- корректное удаление нескольких child;
- отсутствие прежнего `RtlFreeHeap` / heap corruption в проверенном сценарии Save/Load.

Пример структуры:

```
CCustomScript
├── MAIN THREAD
├── CHILD #1 → @SECOND_THREAD
└── CHILD #2 → @SECOND_THREAD_1
```

В текущем тестовом окружении регистрация opcode `0E6F` предоставляется `NewOpcodes.cleo`; изменения ядра обеспечивают корректный runtime lifecycle созданных custom-scripts.

### Почему не используется 00D7 для этой задачи

```
00D7: start_new_script @LABEL
```

является штатным GTA SCM-механизмом. Эксперимент с запуском его из custom `.cs` привёл к падению внутри штатного `CRunningScript::ProcessCommands400To499` на GTA SA 1.0 US.

Поэтому `00D7` не переопределяется и не используется как механизм создания дополнительных custom-потоков. Для custom `.cs` используется `0E6F`.

### Проверено в runtime

На **27.09.2026** выполнен полный тест на **GTA San Andreas 1.0.0.0 US**:

1. Тестовый `.cs` создаёт два child custom-stream через `0E6F`:
   - `@SECOND_THREAD` (label `-70`);
   - `@1SECOND_THREAD` (label `-119`).
2. При сохранении создаётся обычный legacy-файл:
   `./cleo/cleo_saves/csN.sav`.
3. Состояния child-потоков сохраняются отдельно:
   `./cleo/cleo_saves/csN.children.sav`.
4. Проверено фактическое сохранение двух child-состояний:
   `Saved 2 child custom script states ...`.
5. После загрузки читаются оба файла:
   `Loaded 2 child custom script states ...`.
6. Parent восстанавливается из обычного safe-list, после чего ядро автоматически создаёт оба child обратно и применяет сохранённое состояние:
   - `Restored custom child script ... label=-70 ordinal=0`;
   - `Restored custom child script ... label=-119 ordinal=0`.
7. Проверено, что parent + два child существуют одновременно после Load и корректно удаляются при завершении.
8. Старый формат `.cs/.cs3` и layout `CRunningScript` для этой функции не изменялись.

### Следующие проверки

- проверить `0A93` для явного завершения child-script;
- отдельно проверить `0E70` и значение последнего созданного custom-script;
- протестировать большее количество child-потоков и вложенные parent/child;
- исследовать отдельные случаи повреждённых имён scripts в диагностическом логе;
- при необходимости перенести регистрацию/реализацию `0E6F` непосредственно в собственную `CustomOpcodeSystem`.

## Неофициальный статус

Проект не является официальным релизом CLEO Library и не должен выдаваться за него. Оригинальные авторы и официальный проект сохраняют своё авторство. См. [UPSTREAM.md](./UPSTREAM.md) и [AUTHORS.md](./AUTHORS.md).

## AI-assisted development

В проекте используется помощь **ChatGPT (OpenAI)** для анализа исходного кода, рефакторинга, документации и проектирования новых компонентов. AI-помощник не заявляет авторские права на код и работает по указаниям владельца проекта.


## Этап 1 — безопасность памяти (завершено в исходниках)

Первый этап выполняется без изменения формата старых .cs/.cs3 и без изменения структуры CRunningScript/legacy SDK.

Исправлены следующие места:

- 0AB1: количество копируемых параметров ограничено 32 локальными переменными; временный массив аргументов больше не является общим static-буфером.
- 0E6F/child-script path: создание дочернего CCustomScript теперь разрешено только от другого custom-script.
- 0AED: убран неограниченный sprintf; запись результата ограничена 16-байтным legacy string storage.
- 0AE6 и 0AE7: generic filename output больше не использует неограниченный strcpy; запись ограничена legacy 16-байтным string storage.
- CLEO_WriteStringOpcodeParam: запись в строковую переменную выполняется с ограничением размера и безопасным завершением строки.

Все старые opcode остаются на своих номерах и сохраняют существующую схему вызова. На этом этапе не изменяются формат .cs/.cs3, layout CRunningScript, Save/Load и архитектура двух потоков через 0E6F.

### Проверка перед runtime-тестом

Этап 1 протестирован вместе с Stage 2/3 на реальной сборке **Release / GTASA / Win32**. Старые `.cs`-скрипты продолжают загружаться, а тестовый parent + два child через `0E6F` проходят цикл создания, сохранения, загрузки и восстановления.


### Stage 2/3 — parallel custom streams and child save state

- Custom children created from a label inherit the parent CLEO compatibility version.
- `0ABA` prefers terminating the current custom thread when parent and child share the same 8-byte script name; legacy name lookup remains the fallback.
- Child streams are never written into the legacy hash-only saved-thread list.
- Child state is stored in a separate `csN.children.sav` sidecar. The legacy `csN.sav` header and record layout remain unchanged.
- The sidecar stores parent/child node IDs, label, sibling ordinal, locals, timers, IP offset, condition/logical state and name, allowing nested child streams to be restored when their parent recreates them.
