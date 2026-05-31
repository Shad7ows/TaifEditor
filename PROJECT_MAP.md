# PROJECT_MAP.md - TaifEditor LSP Integration

> **التاريخ:** 2026-05-31
> **الإصدار الحالي:** 3.3.0
> **الحالة:** Planning → Approved

---

## [TECH_STACK]

| المكون | الإصدار | المبرر |
|--------|---------|--------|
| **C++** | 23 (`CONFIG += c++23`) | موجود بالمشروع |
| **Qt Widgets** | 6.x | موجود بالمشروع |
| **Qt Core** | 6.x | QJsonDocument, QProcess, QThread |
| **Build System** | qmake | موجود بالمشروع |
| **LSP Protocol** | 3.17.x | المعيار العالمي |
| **Target Platforms** | Windows, Linux, macOS | عبر Qt |

### المكتبات المستخدمة

| المكتبة | المصدر | الاستخدام |
|---------|--------|-----------|
| QJsonDocument | Qt bundled | تسلسل/فك تسلسل JSON لبروتوكول LSP |
| QProcess | Qt bundled | تشغيل alif-lsp.exe والاتصال عبر stdio |
| QThread | Qt bundled | معالجة LSP بشكل غير حاظر |
| QRegularExpression | Qt bundled | تحليل النص للـ tokenization |

**لا توجد external dependencies إضافية** - نبقى على Qt فقط.

---

## [SYSTEM_FLOW]

### تدفق الإكمال التلقائي (Completion Flow)

```
المستخدم يكتب
       │
       ▼
┌──────────────┐    didChange     ┌──────────────┐    stdin (JSON)    ┌──────────────┐
│   TEditor    │ ───────────────▶ │  TLspClient  │ ────────────────▶ │ alif-lsp.exe │
│              │                  │              │                   │              │
│  textCursor  │ ◀─────────────  │  parseJSON   │ ◀──────────────── │  Completion  │
│  .position() │   CompletionList│              │    stdout (JSON)  │  Provider    │
└──────────────┘                  └──────────────┘                   └──────────────┘
       │
       ▼
┌──────────────┐
│AutoCompleteUI│  ← نفس الواجهة الحالية
│  (m3tCompletionPopup) │
└──────────────┘
```

### تدفق التشخيص (Diagnostics Flow)

```
المستخدم يحفظ/يعدل الملف
       │
       ▼
┌──────────────┐    didSave/didChange   ┌──────────────┐    stdin    ┌──────────────┐
│   TEditor    │ ─────────────────────▶ │  TLspClient  │ ─────────▶ │ alif-lsp.exe │
│              │                        │              │            │              │
│  Diagnostic  │ ◀───────────────────── │  publishDiag │ ◀───────── │  Diagnostic  │
│  Markers     │   publishDiagnostics   │  nostics     │  stdout    │  Provider    │
└──────────────┘                        └──────────────┘            └──────────────┘
       │
       ▼
┌──────────────┐
│  Red underlines │  ← عرض الأخطاء inline
│  + squiggly     │
└──────────────┘
```

### تدفق Hover

```
المستخدم يوقف المؤشر فوق كلمة
       │
       ▼
┌──────────────┐    textDocument/hover   ┌──────────────┐    stdin    ┌──────────────┐
│   TEditor    │ ──────────────────────▶ │  TLspClient  │ ─────────▶ │ alif-lsp.exe │
│              │                         │              │            │              │
│  QToolTip    │ ◀────────────────────── │  HoverResult │ ◀───────── │  Hover       │
│  .showText() │      HoverContent      │              │  stdout    │  Provider    │
└──────────────┘                         └──────────────┘            └──────────────┘
```

### تدفق Definition (Go to Definition)

```
المستخدم يضغط Ctrl+Click على متغير
       │
       ▼
┌──────────────┐    textDocument/definition   ┌──────────────┐    stdin    ┌──────────────┐
│   TEditor    │ ───────────────────────────▶ │  TLspClient  │ ─────────▶ │ alif-lsp.exe │
│              │                              │              │            │              │
│  QTextCursor │ ◀─────────────────────────── │  Location    │ ◀───────── │  Definition  │
│  .setPosition│      {file, line, col}       │              │  stdout    │  Provider    │
└──────────────┘                              └──────────────┘            └──────────────┘
```

---

## [ARCHITECTURE]

### النموذج العام

```
┌─────────────────────────────────────────────────────────────────────┐
│                        TaifEditor (taif.exe)                        │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                        UI Layer                              │   │
│  │  ┌──────────┐  ┌──────────────┐  ┌─────────────────────┐   │   │
│  │  │ TEditor  │  │AutoCompleteUI│  │  DiagnosticOverlay  │   │   │
│  │  │(QPlainTe │  │(CompletionPo │  │  (inline errors)    │   │   │
│  │  │ xtEdit)  │  │   pup)       │  │                     │   │   │
│  │  └────┬─────┘  └──────┬───────┘  └──────────┬──────────┘   │   │
│  │       │               │                     │               │   │
│  └───────┼───────────────┼─────────────────────┼───────────────┘   │
│          │               │                     │                    │
│  ┌───────▼───────────────▼─────────────────────▼───────────────┐   │
│  │                     LSP Client Layer                         │   │
│  │  ┌─────────────────────────────────────────────────────┐    │   │
│  │  │                    TLspClient                        │    │   │
│  │  │  - QProcess (alif-lsp.exe)                          │    │   │
│  │  │  - JSON encode/decode                               │    │   │
│  │  │  - Request/Response correlation                     │    │   │
│  │  │  - Notification handling                            │    │   │
│  │  └─────────────────────────────────────────────────────┘    │   │
│  │                                                             │   │
│  │  ┌──────────────────────┐  ┌────────────────────────────┐  │   │
│  │  │  TLspTypes.h          │  │  TLspCompletionBridge      │  │   │
│  │  │  (LSP data structs)  │  │  (LSP ↔ AutoCompleteUI)    │  │   │
│  │  └──────────────────────┘  └────────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────┘   │
│          │ stdio (stdin/stdout)                                     │
│          │                                                          │
│  ┌───────▼─────────────────────────────────────────────────────┐   │
│  │              Existing Components (kept as-is)                │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │   │
│  │  │TSyntaxHighligh│  │AutoComplete  │  │  TMinimap    │      │   │
│  │  │  ter (Lexer)  │  │  (fallback)  │  │              │      │   │
│  │  └──────────────┘  └──────────────┘  └──────────────┘      │   │
│  └─────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              │ stdio
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    alif-lsp.exe (سيرفر مستقل)                       │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    AlifLspServer                             │   │
│  │  - stdio transport (read/write JSON-RPC)                   │   │
│  │  - Request routing                                         │   │
│  │  - Lifecycle management                                    │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    AlifParser                                │   │
│  │  - Tokenizer (from Alif grammar)                           │   │
│  │  - Symbol extraction                                       │   │
│  │  - Syntax validation                                       │   │
│  │  - Scope analysis                                          │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                                                                     │
│  ┌─────────────────────┐ ┌──────────────────┐ ┌────────────────┐  │
│  │ CompletionProvider  │ │ DiagnosticsProv. │ │ HoverProvider  │  │
│  │ - keywords          │ │ - syntax errors  │ │ - docstrings   │  │
│  │ - builtins          │ │ - type errors    │ │ - type info    │  │
│  │ - symbols           │ │ - scope errors   │ │ - signatures   │  │
│  │ - member access     │ │ - warnings       │ │ - examples     │  │
│  └─────────────────────┘ └──────────────────┘ └────────────────┘  │
│                                                                     │
│  ┌─────────────────────┐ ┌──────────────────┐ ┌────────────────┐  │
│  │ DefinitionProvider  │ │ DocumentSymbol   │ │ FormattingProv.│  │
│  │ - go to function    │ │ - outline tree   │ │ (future)       │  │
│  │ - go to class       │ │ - symbols list   │ │                │  │
│  │ - go to variable    │ │                  │ │                │  │
│  └─────────────────────┘ └──────────────────┘ └────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

### هيكل الملفات الجديدة

```
TaifEditor/
├── source/
│   └── lsp/                          # عميل LSP المدمج
│       ├── TLspClient.h              # إدارة الاتصال بـ alif-lsp.exe
│       ├── TLspClient.cpp            # encode/decode JSON + QProcess
│       ├── TLspTypes.h               # أنواع LSP (CompletionItem, Diagnostic, إلخ)
│       └── TLspCompletionBridge.h    # جسر بين LSP والـ AutoCompleteUI
│
├── alif-lsp/                         # سيرفر LSP المستقل
│   ├── alif-lsp.pro                  # ملف qmake
│   ├── main.cpp                      # نقطة الدخول
│   ├── AlifLspServer.h/.cpp          # سيرفر LSP الرئيسي
│   ├── AlifTokenizer.h/.cpp          # محلل tokens للـ Alif
│   ├── AlifParser.h/.cpp             # محلل syntax مبسط
│   ├── AlifSymbolTable.h/.cpp        # جدول الرموز
│   ├── AlifCompletionProvider.h/.cpp  # مزود الإكمال
│   ├── AlifDiagnosticsProvider.h/.cpp # مزود التشخيص
│   ├── AlifHoverProvider.h/.cpp       # مزود Hover
│   ├── AlifDefinitionProvider.h/.cpp  # مزود Definition
│   └── AlifDocDatabase.h/.cpp        # قاعدة بيانات التوثيق
│
├── taif/
│   └── Taif.pro                      # مُحدَّث: إضافة INCLUDEPATH + SOURCES
│
└── PROJECT_MAP.md                    # هذا الملف
```

---

## [MILESTONES]

### Milestone 1: البنية التحتية (Infrastructure)
**هدف قابل للتحقق:** `alif-lsp.exe` يقبل `initialize` request ويرد بـ `capabilities`

| # | المهمة | الملفات | الحالة |
|---|--------|---------|--------|
| 1.1 | إنشاء مجلد `source/lsp/` | - | pending |
| 1.2 | إنشاء `TLspTypes.h` مع أنواع LSP | `source/lsp/TLspTypes.h` | pending |
| 1.3 | إنشاء `TLspClient.h/.cpp` | `source/lsp/TLspClient.h`, `.cpp` | pending |
| 1.4 | إنشاء مشروع `alif-lsp/` | `alif-lsp/alif-lsp.pro`, `main.cpp` | pending |
| 1.5 | تنفيذ initialize/initialized/shutdown | `alif-lsp/AlifLspServer.*` | pending |
| 1.6 | اختبار الاتصال عبر stdio | manual test | pending |

**متطلبات النجاح:**
- [ ] `alif-lsp.exe` يبدأ وينتظر JSON على stdin
- [ ] إرسال `initialize` → استقبال `InitializeResult` مع capabilities
- [ ] إرسال `initialized` → لا خطأ
- [ ] إرسال `shutdown` → استقبال response
- [ ] إرسال `exit` → إنهاء العملية

---

### Milestone 2: الإكمال التلقائي المتقدم (Advanced Completion)
**هدف قابل للتحقق:** إكمال تلقائي يقترح keywords + builtins + functions + variables من الكود

| # | المهمة | الملفات | الحالة |
|---|--------|---------|--------|
| 2.1 | بناء AlifTokenizer | `alif-lsp/AlifTokenizer.*` | pending |
| 2.2 | بناء AlifSymbolTable | `alif-lsp/AlifSymbolTable.*` | pending |
| 2.3 | بناء AlifParser | `alif-lsp/AlifParser.*` | pending |
| 2.4 | بناء AlifCompletionProvider | `alif-lsp/AlifCompletionProvider.*` | pending |
| 2.5 | ربط TLspClient بالمحرر | `TEditor.cpp`, `Taif.cpp` | pending |
| 2.6 | إنشاء TLspCompletionBridge | `source/lsp/TLspCompletionBridge.h` | pending |
| 2.7 | الحفاظ على النظام القديم كـ fallback | `AutoComplete.h` | pending |

**CompletionItem kinds المطلوبة:**
| Kind | المثال | الوصف |
|------|--------|-------|
| `Keyword` (14) | `دالة`, `اذا`, `بينما` | كلمات مفتاحية |
| `Function` (3) | `اطبع()`, `مدى()` | دوال مدمجة |
| `Variable` (6) | متغيرات معرفة في الكود | متغيرات محلية |
| `Class` (7) | `صنف اسم` | تعريفات الأصناف |
| `Method` (2) | دوال داخل صنف | طرق الصنف |
| `Snippet` (15) | `دالة`, `اذا`, `لكل` | قوالب كود |
| `Module` (9) | `الوقت`, `الرياضيات` | وحدات مستوردة |

**Trigger characters:** `.` (member access), `(` (function call), ` ` (space after keyword)

**متطلبات النجاح:**
- [ ] Ctrl+Space يعرض اقتراحات
- [ ] الكتابة التلقائية تعرض اقتراحات م relevance
- [ ] اختيار builtin يدخل `functionName()` مع cursor داخل الأقواس
- [ ]اختيار snippet يدخلقالب الكود مع placeholders
- [ ] member access (`object.`) يعرض خصائص الصنف
- [ ] النظام القديم يعمل إذا فشل LSP

---

### Milestone 3: التشخيص والأخطاء (Diagnostics)
**هدف قابل للتحقق:** عرض أخطاء النسق inline في المحرر

| # | المهمة | الملفات | الحالة |
|---|--------|---------|--------|
| 3.1 | بناء AlifDiagnosticsProvider | `alif-lsp/AlifDiagnosticsProvider.*` | pending |
| 3.2 | تنفيذ publishDiagnostics notification | `AlifLspServer.cpp` | pending |
| 3.3 | عرض الأخطاء inline في المحرر | `TEditor.cpp` (paintEvent) | pending |
| 3.4 | تلوين DiagnosticSeverity | `TLspTypes.h` | pending |

**DiagnosticSeverity levels:**
| Severity | Value | اللون |
|----------|-------|-------|
| Error | 1 | أحمر `#f44747` |
| Warning | 2 | أصفر `#cca700` |
| Information | 3 | أزرق `#10a8f4` |
| Hint | 4 | رمادي `#666666`**

**متطلبات النجاح:**
- [ ] خطأ نسقي يظهر كـ red underline
- [ ] تحذير يظهر كـ yellow underline
- [ ] Mensaje de error يظهر عند التمرير على الخطأ
- [ ] لا يوجد false positives في الكود الصحيح

---

### Milestone 4: Hover و Definition
**هدف قابل للتحقق:** عرض معلومات عند التمرير + الانتقال لتعريف المتغير

| # | المهمة | الملفات | الحالة |
|---|--------|---------|--------|
| 4.1 | بناء AlifHoverProvider | `alif-lsp/AlifHoverProvider.*` | pending |
| 4.2 | بناء AlifDefinitionProvider | `alif-lsp/AlifDefinitionProvider.*` | pending |
| 4.3 | بناء AlifDocDatabase | `alif-lsp/AlifDocDatabase.*` | pending |
| 4.4 | بناء AlifDocumentSymbolProvider | `alif-lsp/AlifDocumentSymbolProvider.*` | pending |

**Documentation DB المطلوبة:**
| الدالة | الوصف | Parameters |
|--------|-------|------------|
| `اطبع()` | طباعة نص إلى المخرجات | `*objects, separator=' ', end='\n'` |
| `ادخل()` | قراءة مدخل من المستخدم | `prompt=''` |
| `مدى()` | إنشاء سلسلة أرقام | `stop, start=0, step=1` |
| `طول()` | إرجاع طول الكائن | `object` |
| `صحيح()` | تحويل إلى عدد صحيح | `x, base=10` |
| `عشري()` | تحويل إلى عدد عشري | `x` |
| `مصفوفة()` | إنشاء مصفوفة | `iterable` |
| `مترابطة()` | إنشاء مترابطة | `*elements` |
| `فهرس()` | إنشاء فهرس | `mapping` |
| `مميزة()` | إنشاء مميزة | `iterable` |
| `افتح()` | فتح ملف | `file, mode='r', encoding=None` |
| `اقرأ()` | قراءة ملف | `file` |
| `اكتب()` | كتابة في ملف | `file, content` |
| `اغلق()` | إغلاق ملف | `file` |
| `اقصى()` | أقصى قيمة | `*args` |
| `ادنى()` | أدنى قيمة | `*args` |
| `اجمع()` | مجموع العناصر | `iterable, start=0` |
| `تحقق_اي()` | هل أي عنصر صحيحاً | `iterable` |
| `هل_نوع()` | فحص نوع الكائن | `object, type` |
| `مقرون()` | ربط عناصر بنص | `separator, iterable` |

**متطلبات النجاح:**
- [ ] التمرير فوق `اطبع` يعرض tooltip بالوصف والمعاملات
- [ ] Ctrl+Click على متغير ينتقل لسطر تعريفه
- [ ] Symbol outline يعرض هيكل الملف
- [ ] Documentation تظهر بتنسيق markdown

---

### Milestone 5: التكامل النهائي
**هدف قابل للتحقق:** النظام يعمل كاملاً مع fallback تلقائي

| # | المهمة | الملفات | الحالة |
|---|--------|---------|--------|
| 5.1 | إضافة toggle في Settings | `TSettings.cpp` | pending |
| 5.2 | نظام fallback تلقائي | `TLspClient.cpp` | pending |
| 5.3 | Optimistic updates | `TLspClient.cpp` | pending |
| 5.4 | Logging system | `source/lsp/TLspLogger.h/.cpp` | pending |
| 5.5 | تحديث Taif.pro | `taif/Taif.pro` | pending |
| 5.6 | اختبار شامل | - | pending |
| 5.7 | تحديث deployment scripts | `deployment.md` | pending |

**Settings toggle:**
```cpp
// في TSettings
QCheckBox *lspEnabledCheckBox;
// QSettings key: "lspEnabled" (default: true)
```

**Fallback logic:**
```
if (lspEnabled && alifLspProcess->state() == QProcess::Running) {
    // استخدام LSP
    requestLspCompletion(...);
} else {
    // النظام القديم
    completionModel->updateCompletions(prefix, text);
}
```

**متطلبات النجاح:**
- [ ] Toggle في Settings يعمل (تفعيل/تعطيل LSP)
- [ ] إذا لم يُعثر على alif-lsp.exe → fallback تلقائي
- [ ] إذا crash السيرفر → fallback تلقائي + إعادة تشغيل
- [ ] Optimistic completion (النظام القديم يعمل حتى يرد LSP)
- [ ] Logging يسجل الأخطاء في ملف
- [ ] alif-lsp.exe مُضمن في حزمة التثبيت

---

## [DECISIONS]

| # | القرار | البديل المرفوض | المبرر |
|---|--------|----------------|--------|
| D1 | سيرفر LSP منفصل (alif-lsp.exe) | LSP مدمج في Taif.exe | عزل الأخطاء، سهولة الصيانة، المعيار العالمي |
| D2 | beralih ke qmake (بما أن المشروع يستخدمه) | CMake/FetchContent | الاتساق مع المشروع الحالي |
| D3 | QJsonDocument لبروتوكول LSP | nlohmann/json | لا external dependencies |
| D4 | Parser مبسط (لا AST كامل) | استخدام parser Alif الأصلي | parser الأصلي مرتبط بالـ interpreter |
| D5 | Conservative logging (QFile + QTextStream) | spdlog, glog | لا external dependencies |
| D6 | الحفاظ على النظام القديم كـ fallback | استبدال كامل | الأمان عند فشل LSP |

---

## [LOGGING]

### نظام التسجيل الآسيوي (Asynchronous Logging)

```
┌──────────────┐     enqueue     ┌──────────────┐    flush (50ms)    ┌──────────────┐
│  LSP Client  │ ──────────────▶ │  Log Buffer  │ ────────────────▶ │  alif-lsp    │
│  (any thread)│                  │  (QMutex)    │                   │  .log file   │
└──────────────┘                  └──────────────┘                   └──────────────┘
```

**المستويات:**
| Level | الاستخدام |
|-------|-----------|
| `ERROR` | فشل تشغيل السيرفر، crash، خطأ في protocol |
| `WARN` | timeout في الرد، fallback للنظام القديم |
| `INFO` | تشغيل السيرفر، إكمال ناجح، تشخيص |

**الملف:** `%APPDATA%/Alif/Taif/lsp.log` (Windows) أو `~/.config/Alif/Taif/lsp.log` (Linux/Mac)

---

## [ORPHANS & PENDING]

| العنصر | النوع | الحالة | الأولوية | ملاحظات |
|--------|-------|--------|----------|---------|
| LSP Protocol Implementation | بناء | pending | high | بسيط عبر QJsonDocument |
| Alif Tokenizer for LSP | بناء | pending | high | مبسط من Grammar الرسمي |
| Alif Parser for LSP | بناء | pending | high | syntax validation + symbol extraction |
| Symbol Table | بناء | pending | high | يتبع functions, classes, variables |
| Completion Provider | بناء | pending | high | keywords + builtins + symbols |
| Diagnostics Provider | بناء | pending | high | syntax + semantic errors |
| Hover Provider | بناء | pending | medium | documentation for builtins |
| Definition Provider | بناء | pending | medium | go to definition |
| Document Symbol Provider | بناء | pending | medium | outline/symbols |
| Documentation Database | بناء | pending | medium | 20+ built-in functions |
| Settings Toggle | تعديل | pending | high | LSP on/off |
| Fallback System | بناء | pending | high | auto-switch to old system |
| Optimistic Updates | بناء | pending | medium | old system works while LSP responds |
| Logging System | بناء | pending | low | async file logging |
| Taif.pro Update | تعديل | pending | high | add LSP sources |
| Deployment Scripts | تعديل | pending | medium | include alif-lsp.exe |
| Testing | اختبار | pending | high | unit + integration tests |

---

## [DEPENDENCIES]

```
TaifEditor (taif.exe)
    │
    ├── source/lsp/ (مدمج)
    │   ├── TLspClient
    │   ├── TLspTypes
    │   └── TLspCompletionBridge
    │
    └── alif-lsp.exe (مستقل)
        ├── AlifLspServer
        ├── AlifTokenizer
        ├── AlifParser
        ├── AlifSymbolTable
        ├── AlifCompletionProvider
        ├── AlifDiagnosticsProvider
        ├── AlifHoverProvider
        ├── AlifDefinitionProvider
        └── AlifDocDatabase
```

---

## [TEST_PLAN]

### Unit Tests
| # | الاختبار | المكون |
|---|----------|--------|
| T1 | JSON encode/decode لـ LSP messages | TLspClient |
| T2 | Tokenizer يقرأ keywords صحيحة | AlifTokenizer |
| T3 | Parser يكتشف syntax errors | AlifParser |
| T4 | SymbolTable يتبع المتغيرات | AlifSymbolTable |
| T5 | CompletionProvider يقترح بالـ prefix الصحيح | AlifCompletionProvider |
| T6 | DiagnosticsProvider يكتشف أخطاء م_known | AlifDiagnosticsProvider |

### Integration Tests
| # | الاختبار | النتيجة المتوقعة |
|---|----------|-------------------|
| IT1 | فتح ملف Alif → LSP يبدأ | initialize response |
| IT2 | كتابة `اطب` → completion | اقتراح `اطبع()` |
| IT3 | كتابة كود خاطئ → diagnostics | error underline |
| IT4 | Ctrl+Click على متغير → definition | الانتقال للسطر |
| IT5 | Hover فوق `اطبع` → documentation | tooltip |
| IT6 | crash LSP → fallback | النظام القديم يعمل |
| IT7 | تعطيل LSP من Settings | النظام القديم فقط |

### Manual Testing Checklist
- [ ] تشغيل alif-lsp.exe بشكل مستقل
- [ ] الاتصال عبر stdio
- [ ] completion في ملف فارغ
- [ ] completion في ملف به أصناف ودوال
- [ ] diagnostics في كود خاطئ
- [ ] hover على كل built-in functions
- [ ] definition لمتغير معرف محلياً
- [ ] definition لدالة معرفة في نفس الملف
- [ ] document symbols في ملف كبير
- [ ] performance (< 100ms latency)

---

## [PERFORMANCE_TARGETS]

| المقياس | الهدف | ملاحظات |
|---------|-------|---------|
| Completion latency | < 100ms | من الإدخال إلى عرض الاقتراحات |
| Diagnostics latency | < 500ms | من الحفظ إلى عرض الأخطاء |
| Hover latency | < 200ms | من التوقف إلى عرض tooltip |
| LSP startup | < 2s | من تشغيل alif-lsp.exe إلى initialize response |
| Memory overhead | < 50MB | alif-lsp.exe usage |
| CPU idle | < 1% | عند عدم استخدام LSP features |

---

> **ملاحظة:** هذا الملف يُحدَّث تلقائياً مع تقدم المشروع.
> آخر تحديث: 2026-05-31
