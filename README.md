# CLEO 4.4.4 Unofficial Patch

## Что это

**CLEO 4.4.4 Unofficial Patch** — независимая разработка на базе исходного CLEO 4.4.4 для GTA San Andreas.

Цель проекта — сохранить старую совместимость CLEO 4 и одновременно привести внутреннюю реализацию к современному окружению разработки. Проект является независимой неофициальной разработкой.

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

### Stage 4.1 — завершение child через 0A93 (проверено)

На **27.09.2026** выполнен runtime-тест:

- parent `TEST2` создаёт два child через `0E6F`:
  - label `-70`;
  - label `-119`;
- child `-119` выполняет `0A93: end_custom_thread`;
- после `0A93` удаляется только текущий child, второй child и parent продолжают существовать;
- при Save в `cs0.children.sav` сохраняется только оставшийся child:
  `Saved 1 child custom script states ...`;
- после Load читается один child-state;
- восстанавливается только child с label `-70`;
- завершённый через `0A93` child с label `-119` после загрузки не появляется.

Тем самым подтверждена связка:

`0E6F` → создание child → `0A93` → удаление текущего child → Save/Load → восстановление оставшегося child.

### Следующие проверки

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


## Диагностический лог и проверка runtime — 27.09.2026

В проект возвращён диагностический лог без отдельного DiagnosticLog-подпроекта.

### Что сделано

- `CDiagnosticLog.h/.cpp` снова входят непосредственно в основной проект CLEO4.
- `source/cleo.h` подключает `CDiagnosticLog.h`.
- `CLEO4.vcxproj` содержит `CDiagnosticLog.cpp` и `CDiagnosticLog.h`.
- Макрос `DIAG(...)` используется для внутренних диагностических сообщений.
- Лог создаётся в корне игры как `cleo_diagnostic.log`.
- Лог открывается с `std::ios::trunc`, поэтому каждый запуск начинает новый файл.
- Убраны отдельные `Error()` / `DIAG_ERROR()` из текущей реализации.
- Диагностика custom-script lifecycle показывает `LOAD`, `CREATE`, `REGISTER`, `END`, `STOP`, `DELETE`, а также parent/child label.

### Проверка custom-script lifecycle

На **27.09.2026** выполнен runtime-тест на **GTA San Andreas 1.0.0.0 US**.

Лог подтвердил:

- обычные `.cs` из ModLoader успешно загружаются и регистрируются;
- `0E6F` создаёт два дочерних custom-script из одного parent;
- child с label `-170` корректно завершается через `0A93`;
- child с label `-99` продолжает выполняться;
- затем оставшийся child также корректно завершается;
- при завершении игры custom scripts проходят `END` и `DELETE`;
- несколько одновременно существующих custom scripts корректно обрабатываются;
- имена в диагностике соответствуют внутреннему 8-байтному полю `CRunningScript::Name`; изменение отображаемого имени у некоторых ModLoader `.cs` само по себе не считается доказательством повреждения памяти.

Последний тест использовал цепочку:

`0E6F → 0E70 → 0D2E → 0A93`

где:

- `0E6F` создаёт child;
- `0E70` получает указатель на последний созданный custom-script;
- `0D2E` устанавливает значение локальной переменной child;
- `0A93` завершает текущий child.

### Диагностический вывод

По последнему логу явного сбоя lifecycle custom-script не обнаружено. В частности, проверенный сценарий с двумя child корректно проходит создание, выполнение, завершение и удаление.

Имена вроде `manuald`, `driveby`, `voidwea`, `gang_hu` являются именами загруженных `.cs`/ModLoader-скриптов, усечёнными до legacy-буфера `CRunningScript::Name[8]`. Поэтому такие строки в диагностическом логе не рассматриваются как отдельная ошибка CLEO без воспроизводимого повреждения состояния.

### Следующая проверка

Следующим отдельным направлением остаются `0E70`, большее количество child-потоков и вложенные `parent → child → child`. Сохранение и восстановление child-local state уже проверено через полный Save/Load.


## Этап 3 — RAII-защита памяти и несколько PE-секций (28.09.2026)

Добавлен отдельный слой управления защитой памяти для инъекций без изменения legacy execution flow и без изменения старых opcode.

### Что сделано

- Добавлен модуль `CMemory.h/.cpp` с классом `CMemoryProtection`.
- `CMemoryProtection` временно меняет защиту памяти через `VirtualProtect` и автоматически восстанавливает сохранённое состояние при выходе из области действия.
- Класс поддерживает move-семантику и не допускает копирования состояния защиты.
- `CCodeInjector` теперь использует отдельные RAII-состояния для всех найденных writable-секций.
- Важное исправление: GTA SA 1.0 US в текущем окружении содержит два отдельных региона `.text`. Поэтому один объект `m_textProtection` оказался недостаточным и приводил к преждевременному восстановлению защиты первого региона.
- Хранение изменено на `std::vector<CMemoryProtection>`, поэтому каждый найденный `.text` и `.rdata` регион получает собственный объект защиты.
- `OpenReadWriteAccess()` сохраняет состояние всех обработанных регионов.
- `CloseReadWriteAccess()` восстанавливает каждый сохранённый регион отдельно.
- Существующий контракт CLEO сохранён: во время инициализации plugins/injection запись в нужные области остаётся доступной.
- Legacy `CCodeInjector` hooks, `CScriptEngine`, `CCustomScript` и `ScmFunction` по логике не переписывались.

### Runtime-проверка

На **28.09.2026** выполнен полный запуск на **GTA San Andreas 1.0 US** после исправления поддержки нескольких `.text` регионов.

В логе подтверждены:

- оба региона `.text` и регион `.rdata` успешно открываются для записи;
- `MenuStatusNotifier`, `DmaFix`, `TextManager`, `SoundSystem`, `CustomOpcodeSystem` и `ScriptEngine` успешно инъектируются;
- старые CLEO/ModLoader `.cs` снова загружаются и регистрируются;
- `0A92` успешно создаёт custom script;
- `0AB1/0AB2` продолжают работать;
- тест `test2` передаёт `123` и возвращает `123`;
- завершение scripts и игры проходит без зафиксированного crash/access violation.

Зафиксированный runtime-фрагмент:

```text
[0A92] Starting new custom script WEAPONWHEELUI.CS from thread named vwwmain
[0AB1] vwwmain args=1 1513 0
[0AB2] vwwmain ret=1 0

[0AB1] test2 args=1 123 0
[0AB2] test2 ret=1 123
```

### Что исправлено после первого теста

Первая версия RAII-хранила только:

```cpp
CMemoryProtection m_textProtection;
CMemoryProtection m_rdataProtection;
```

При обработке второго `.text` старое состояние первого региона заменялось. В результате часть памяти возвращалась к исходной защите ещё до установки следующих хуков, и старые CLEO переставали работать.

После перехода на:

```cpp
std::vector<CMemoryProtection> m_memoryProtections;
```

каждая секция хранит собственное состояние защиты. Повторный runtime-тест подтвердил восстановление работы старых CLEO-скриптов и всех проверенных custom-script механизмов.

### Ограничение текущей проверки

Runtime подтверждает корректную установку и удержание защиты во время инъекций. Отдельный тест вызова `CloseReadWriteAccess()` с проверкой фактических исходных `PAGE_*` прав ещё является самостоятельной проверкой и не считается завершённым на этом этапе.

## Этап 2 — безопасность и совместимость 0AB1/0AB2 (27.09.2026)

Исходная реализация CLEO 4.4.4 могла выйти за границу фиксированного набора из 32 `SCRIPT_VAR` при работе с `0AB1/0AB2`. Для патча сохранена архитектура CLEO 4, а количество аргументов и результатов явно ограничено 32 слотами.

### Что изменено

- `0AB1`: допустимо максимум 32 входных параметра.
- `0AB1`: параметры декодируются непосредственно из потока скрипта и сохраняются только в фиксированные 32 локальных слота.
- `0AB1`: при количестве >32 вызов отклоняется до создания нового `ScmFunction`.
- `0AB2`: допустимо максимум 32 возвращаемых параметра.
- `0AB2`: при количестве >32 вызов отклоняется до чтения возвращаемых значений и записи в `opcodeParams`.
- `0AB2`: после возврата значения записываются по одному через `SetScriptParams(thread, 1)`, сохраняя штатную семантику переменных GTA.
- В обоих opcode не выполняется массовый вызов GTA с количеством параметров больше 32.
- Добавлена явная обработка неизвестного типа параметра в `0AB1`.

### Что не изменялось

- `CRunningScript` и его legacy layout;
- `LocalVar[32]`;
- `GetScriptParams` / `SetScriptParams` внутри GTA;
- архитектура `ScmFunction`;
- архитектура parent → child;
- `0E6F` и lifecycle custom-stream;
- Save/Load `csN.sav` и `csN.children.sav`;
- совместимость старых `.cs`, `.cs3`, `.cs4`.

### Источник модели

Для `0AB1/0AB2` закреплено единое правило: функция не может получать или возвращать более 32 параметров. Ограничение проверяется до создания `ScmFunction` и до записи возвращаемых значений.

### Текущий статус

Изменение внесено непосредственно в `source/CCustomOpcodeSystem.cpp` и зафиксировано в Git. Новые runtime-тестовые файлы в основной репозиторий не добавлялись.

**Сборка подтверждена 27.09.2026:**

- `Release / GTASA / Win32`;
- проект `CLEO4` — успешно;
- `output\\Release\\CLEO.asi` создан;
- `IntOperations` — успешно;
- `IniFiles` — успешно;
- `FileSystemOperations` — успешно;
- итог: **4 успешно, 0 ошибок, 0 пропущено**.

Компиляция подтверждает корректность C++-изменений `0AB1/0AB2` на уровне сборки. Runtime-проверка поведения opcode в GTA SA выполняется отдельно.

## Следующий этап

1. Собрать `Release / GTASA / Win32`.
2. Проверить загрузку обычных `.cs/.cs3/.cs4`.
3. Отдельно выполнить runtime-проверку 0AB1/0AB2 на количестве параметров больше 32 без добавления тестовых файлов в основной репозиторий.
4. Проверить сохранение и восстановление child-local state через полный Save/Load.
5. Продолжить аудит custom-script lifecycle и вложенных parent/child.
## Runtime verification — 0AB1/0AB2, ScmFunction и 0E6F (27.09.2026)

Этот раздел фиксирует фактически выполненную runtime-проверку на **GTA San Andreas 1.0.0.0 US**. Он предназначен как постоянная техническая отметка для дальнейшей разработки и повторной проверки архитектуры.

### 0AB1/0AB2 — пройденные тесты

Использован синтаксис и opcode definition из **Sanny Builder 3.8.5** (`opcodes SannyBuilder-v3.8.5(1).txt`).

Проверены:

1. `0AB1` без параметров → `0AB2` без возврата.
2. `0AB1` с одним параметром → `0AB2` с одним результатом: `10 → 15`.
3. `0AB1` с двумя параметрами → `0AB2` с двумя результатами: `111, 222 → 121, 242`.
4. Передача `ConditionResult` из функции, возвращающей `TRUE`.
5. Передача `ConditionResult` из функции, возвращающей `FALSE`.
6. Проверка изменения результата после `0AB1` через `004D: jump_if_false`.
7. Проверка сохранения `TRUE` результата после `0AB1`.
8. Вложенные функции: `MAIN → A → B → C → 0AB2`, результаты: `777 → 797 → 807`.
9. `GOSUB` внутри `0AB1`-функции: `40 → 42`.

Диагностический лог подтвердил:

```
[0AB1] testab12 args=0 0 0
[0AB2] testab12 ret=0 0

[0AB1] testab12 args=1 10 0
[0AB2] testab12 ret=1 15

[0AB1] testab12 args=2 111 222
[0AB2] testab12 ret=2 121

[0AB2] testab12 ret=1 777
[0AB2] testab12 ret=1 797
[0AB2] testab12 ret=1 807

[0AB1] testab12 args=1 40 0
[0AB2] testab12 ret=1 42
```

`0AB2` в текущем диагностическом сообщении печатает количество возвращаемых параметров и только **первое** значение; поэтому второе значение `242` должно подтверждаться экранным результатом тестового скрипта, а не этой одной строкой лога.

### Почему ConditionResult-тест написан через 004D

В используемом файле opcode definition Sanny Builder 3.8.5 строка:

```
0AB1: cleo_call @LABEL ...
```

не объявлена как `IF and SET`. Поэтому конструкция вида:

```
if
    0AB1: cleo_call ...
then
```

не является корректным синтаксисом для данного opcode definition.

Для runtime-проверки используется:

```
0AB1: cleo_call @FUNCTION 0
004D: jump_if_false @LABEL
```

Это позволяет проверять фактический `ConditionResult`, не изменяя opcode definition Sanny Builder.

### ScmFunction — текущая архитектура

В патче `ScmFunction` реализован как отдельный execution-scope поверх существующего `CCustomScript`:

```
ScmFunction
├── prevScmFunctionId / thisScmFunctionId
├── callArgCount / callIP / retnAddress
├── savedBaseIP / savedCodeSize
├── savedStack[8] / savedSP
├── savedTls[32]
├── savedCondResult
├── savedLogicalOp
├── savedNotFlag
├── savedScriptFileDir / savedScriptFileName
└── stringParams
```

При входе в функцию сохраняется контекст caller и создаётся чистый execution-scope. При `0AB2` восстанавливается caller, включая locals, GOSUB stack и состояние условного выражения.

Вложенный тест `A → B → C` подтвердил работу цепочки `ScmFunction`, а тест с `GOSUB` подтвердил работу отдельного сохранённого GOSUB stack.

### 0E6F — архитектура не заменяется

**Важно для дальнейшей разработки: `0E6F` сохраняет текущую custom-stream архитектуру.**

Наша custom-stream схема остаётся:

```
0E6F
  ↓
CCustomScript
  ├── parentThread
  ├── childThreads
  ├── ownedBuffer
  └── CodeSize / Save state
```

`ScmFunction` является отдельным механизмом функций и не заменяет `CCustomScript`, `parentThread`, `childThreads` или lifecycle `0E6F`.

### Runtime Save/Load — 0E6F child states

27.09.2026 выполнен Save/Load с тестовым `test 2potoka.cs`.

При сохранении лог подтвердил:

```
Done. Saved 1024 cleo variables, 2 saved threads, 0 stopped threads
[Cleo][Save][Custom] saved child states=3 file=./cleo/cleo_saves/cs0.children.sav
```

При загрузке:

```
Finished. Loaded 1024 cleo variables, 2 saved threads info, 0 stopped threads info
[Cleo][LOAD][Custom] loaded child states=3 file=./cleo/cleo_saves/cs0.children.sav
```

После загрузки `0AB1/0AB2` снова выполняются без ошибки.

Это подтверждает, что загрузка `csN.sav` + `csN.children.sav` не ломает последующее выполнение проверенного `0AB1/0AB2` runtime.

### Что НЕ делать при следующих изменениях

- Не менять legacy layout `CRunningScript` без отдельного обоснования.
- Не менять `.cs/.cs3/.cs4` формат ради новой архитектуры.
- Не использовать строковые предположения о `SCRIPT_VAR` без проверки точного типа и `GetScriptParamPointer()`.
- Не считать диагностическую строку `ret=2 121` доказательством значения второго return — текущий лог печатает только первый result.
- Для тестов Sanny Builder 3.8.5 использовать фактические определения из `opcodes SannyBuilder-v3.8.5(1).txt`, а не придумывать дополнительные `IF`-свойства opcode.

### Следующий технический этап

После завершённых базовых тестов следующий отдельный тест должен соединить две уже проверенные системы:

```
0E6F
  ↓
child
  ↓
0AB1
  ↓
nested 0AB1
  ↓
0AB2
  ↓
Save/Load
  ↓
продолжение child
```

Цель — проверить именно взаимодействие `CCustomScript/0E6F` и `ScmFunction`, не изменяя архитектуру `0E6F`.
## Этап 4 — HookSystem v1: CALL/JUMP/POINTER hooks (28.09.2026)

Добавлен первый отдельный менеджер legacy-инъекций поверх существующей 5-байтовой модели `CALL/JMP`.

### Что сделано

- Добавлен `source/CHookSystem.h`.
- Добавлен `source/CHookSystem.cpp`.
- Обновлены `CLEO4.vcxproj` и `CLEO4.vcxproj.filters`.
- `CCleoInstance` получил экземпляр `CHookSystem`.
- HookSystem хранит адрес патча, адрес replacement, имя hook, тип `CALL/JUMP` и исходные 5 байт.
- Для `CALL` дополнительно сохраняется вычисленный адрес оригинальной функции.
- Повторная установка hook на тот же адрес отклоняется.
- Реализованы `Remove()`, `RemoveAll()` и `IsInstalled()`.
- Legacy `MemCall()` / `MemJump()` пока не удаляются и не переписываются глобально.
- Остальные существующие hooks пока продолжают работать через старый `CCodeInjector`, чтобы миграция выполнялась по одному и была проверяема.

### Runtime-проверка

На **28.09.2026** выполнен runtime-тест на **GTA San Andreas 1.0 US**.

Первым hook через HookSystem был переведён:

```text
UpdateGameLogics
```

Лог подтвердил:

```text
[HookSystem] Installed CALL 'UpdateGameLogics' at 0x0053E981 -> replacement original=0x0053BEE0
```

Вторым hook через HookSystem был переведён:

```text
CreateMainWindow
```

Лог подтвердил:

```text
[HookSystem] Installed CALL 'CreateMainWindow' at 0x007487A8 -> replacement original=0x00745560
```

После обоих hook продолжили работать:

- создание главного окна;
- SoundSystem;
- загрузка обычных CLEO/ModLoader `.cs`;
- `0A92`;
- `0AB1/0AB2`;
- тест `123 → 123`;
- завершение и удаление custom scripts;
- восстановление защиты памяти при завершении CLEO.

Crash/access violation в проверенном сценарии не зафиксирован.

### PointerHook — runtime-проверка (28.09.2026)

Добавлен и проверен отдельный тип pointer/data hook для `MA_DEF_WINDOW_PROC_PTR`.

Логика сохраняет двойную косвенность существующего патча:

- HookSystem сохраняет исходное 32-битное значение слота `MA_DEF_WINDOW_PROC_PTR`.
- После установки pointer hook исходное значение используется для второго чтения и получения реального адреса `DefWindowProc`.
- `CSoundSystem::imp_DefWindowProc` получает настоящий исходный адрес, как в старой реализации.
- При этом запись и восстановление самого pointer-слота выполняет `CHookSystem`.

Runtime-проверка на **GTA San Andreas 1.0 US** подтверждена:

```text
[HookSystem] Installed POINTER 'DefWindowProc' at 0x00748454 -> replacement original=0x008582A4
Creating main window...
SoundSystem initialized
[0A92] Starting new custom script WEAPONWHEELUI.CS
[0AB1] ...
[0AB2] ...
...
Log finished.
```

После исправления двойной косвенности приложение запускается штатно. В проверенном сценарии продолжают работать SoundSystem, обычные CLEO/ModLoader `.cs`, `0A92`, `0AB1/0AB2`, завершение scripts и восстановление memory protection.
### DrawMenuBackground — runtime-проверка (28.09.2026)

Третий обычный `CALL` hook переведён на `CHookSystem`:

```text
DrawMenuBackground
```

Runtime-лог на GTA San Andreas 1.0 US подтвердил:

```text
[HookSystem] Installed CALL 'DrawMenuBackground' at 0x0057B9FD -> replacement original=0x00728350
Creating main window...
SoundSystem initialized
[0A92] Starting new custom script WEAPONWHEELUI.CS
[0AB1] ...
[0AB2] ...
[0AB1] test2 args=1 123 0
[0AB2] test2 ret=1 123
Log finished.
```

После перевода `DrawMenuBackground` продолжили работать меню, SoundSystem, обычные CLEO/ModLoader `.cs`, custom-script lifecycle и проверенные `0A92/0AB1/0AB2`. При завершении снова восстановлены все сохранённые `.text/.rdata` защиты.
### CTextLocate — JUMP runtime-проверка (28.09.2026)

Первый реальный **JUMP** hook через `CHookSystem` переведён с legacy `InjectFunction()` на:

```text
CTextLocate
```

Использованный патч:

```cpp
CText__Get = gvm.TranslateMemoryAddress(MA_CALL_CTEXT_LOCATE);
GetInstance().HookSystem.InstallJump(
    inj,
    "CTextLocate",
    CText__Get,
    (size_t)CText__locate
);
```

Runtime-лог на **GTA San Andreas 1.0 US** подтвердил установку:

```text
[HookSystem] Installed JUMP 'CTextLocate' at 0x006A0050 -> 0x6AD55321 original=0x00000000
Creating main window...
SoundSystem initialized
Scripts exclusively initialized
```

После установки `CTextLocate` успешно продолжили работу:

- создание главного окна;
- SoundSystem;
- загрузка обычных CLEO/ModLoader `.cs`;
- `0A92` и создание `WEAPONWHEELUI.CS`;
- `0AB1/0AB2` внутри custom-script;
- существующий parent/child custom-stream lifecycle;
- завершение scripts и игры без зафиксированного crash/access violation;
- восстановление `.text/.rdata` memory protection при завершении.

Важное ограничение текущей диагностики: для отдельного `test2` в этом запуске в логе присутствует `0AB1`, но строка `0AB2` перед завершением не зафиксирована. Поэтому этот конкретный запуск не используется как доказательство полного цикла `test2 0AB1 → 0AB2`; ранее полный цикл `123 → 123` уже был отдельно подтверждён.

### Связь HookSystem с custom-stream архитектурой 0E6F

Перевод `CTextLocate` на `JUMP` **не изменяет архитектуру `0E6F`**.

Проверенная схема custom-stream остаётся:

```text
0E6F
  ↓
CCustomScript
  ├── parentThread
  ├── childThreads
  ├── ownedBuffer
  └── Save/Load child state
```

HookSystem отвечает только за установку и откат native memory hooks. `0E6F`, `CCustomScript`, `parentThread/childThreads` и sidecar `csN.children.sav` остаются отдельным execution/lifecycle слоем.

Предыдущие runtime-проверки уже подтвердили:

- создание нескольких child через `0E6F`;
- завершение child через `0A93`;
- сохранение child state;
- восстановление child state после Load;
- продолжение работы `0AB1/0AB2` после восстановления.

Таким образом, текущая миграция HookSystem не подменяет и не объединяет `JUMP`-hooks с механизмом `0E6F`.

### Что пока не переводилось

- hooks внутри `CScriptEngine::Inject()`;
- остальные старые `ReplaceFunction()` / `InjectFunction()`.

Pointer/data hook `MA_DEF_WINDOW_PROC_PTR` уже переведён и отдельно подтверждён в runtime, включая сохранение двойной косвенности исходной реализации.

### Следующий технический этап

Продолжать миграцию оставшихся legacy hooks по одному:

1. выбрать конкретный `ReplaceFunction()` / `InjectFunction()`;
2. перевести его на соответствующий тип `CHookSystem`;
3. собрать Win32;
4. выполнить runtime-проверку на **GTA San Andreas 1.0 US**;
5. только после успешной проверки фиксировать следующий hook.

Execution Engine, legacy `CRunningScript`, формат старых `.cs/.cs3/.cs4` и архитектура `0E6F` при этом не изменяются.
## Структурный рефакторинг — 27.09.2026

Выполнено безопасное разделение внутреннего кода без изменения legacy execution flow.

### Новая структура

```text
CScriptEngine.cpp/.h
└── управление custom-script, очередями и Save/Load

CCustomScript.cpp/.h
└── состояние и жизненный цикл одного custom .cs

ScmFunction.cpp/.h
└── execution-scope для 0AB1/0AB2

CCustomOpcodeSystem.cpp/.h
└── регистрация и обработчики opcode
```

### Что сделано

- `CCustomScript` вынесен из `CScriptEngine.cpp/.h` в отдельный модуль.
- `ScmFunction` вынесен из `CCustomOpcodeSystem.cpp` в отдельный модуль.
- `0AB1` и `0AB2` не переписывались по логике; изменено только размещение связанного кода.
- `Execution Engine` не переделывался.
- Save/Load оставлен в `CScriptEngine`, поскольку он управляет общим состоянием движка, очередями и восстановлением custom scripts.
- Обновлены `CLEO4.vcxproj` и `CLEO4.vcxproj.filters`.
- Сохранены существующие `.cs/.cs3/.cs4`, legacy `CRunningScript` и номера старых opcode.

### Результат сборки

27.09.2026 успешно собраны обе конфигурации:

- `Debug / GTASA / Win32` — **4 успешно, 0 ошибок**;
- `Release / GTASA / Win32` — **4 успешно, 0 ошибок**.

Release-файлы:

```text
output/Release/CLEO.asi
output/IniFiles.cleo
output/IntOperations.cleo
output/FileSystemOperations.cleo
```

### Runtime-проверка Release

На GTA San Andreas 1.0 US проверено:

- загрузка CLEO;
- загрузка обычных `.cs` из CLEO и ModLoader;
- создание custom script через `0A92`;
- выполнение `0AB1`;
- возврат через `0AB2`;
- корректная регистрация и удаление custom scripts;
- корректное завершение игры без ошибки.

Зафиксированный пример:

```text
[0A92] Starting new custom script WEAPONWHEELUI.CS from thread named vwwmain
[0AB1] vwwmain args=1 1513 0
[0AB2] vwwmain ret=1 0
```

Debug runtime дополнительно подтвердил Save/Load custom-script state и sidecar child-state.

### Текущее состояние

Структурный рефакторинг завершён и проверен в `Debug` и `Release`. Следующие изменения должны выполняться отдельными этапами с обязательной сборкой и runtime-проверкой после каждого крупного изменения.
