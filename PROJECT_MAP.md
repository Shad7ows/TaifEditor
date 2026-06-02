# TaifEditor — Project Map
# طيف — خريطة المشروع

> محرر ألف — محرر ذكي مدمج بالكامل لغة ألف البرمجية
> الإصدار: 4.0 (مخطط)
> آخر تحديث: 2026-06-02

---

## [TECH_STACK]

| المكون | الإصدار | الملاحظات |
|---|---|---|
| Qt | 6.9.2 (MinGW 64-bit) | يتوافق مع 6.8 LTS حتى 6.11 |
| C++ | C++23 | CONFIG += c++23 |
| Build System | qmake | Taif.pro |
| اللغة المدعومة | ألف v5.3.0 | aliflang.org |
| الاتصال الخارجي | لا يوجد | كل شيء محلي (No LSP) |

### المكتبات المستخدمة
- `QtCore` — أساسيات
- `QtGui` — رسم وتنسيق
- `QtWidgets` — واجهة المستخدم
- لا توجد مكتبات خارجية إضافية

---

## [SYSTEM_FLOW]

### تدفق الكتابة الحالي (v3.3)
```
المستخدم يكتب → TEditor::keyPressEvent()
                ├→ handleAutoPairing() — أزواج الأقواس
                ├→ curserIndentation() — إضافة مسافة بادئة
                ├→ performCompletion() — إكمال تلقائي
                └→ document()->contentsChanged()
                    ├→ TSyntaxHighlighter::highlightBlock() — تلوين
                    ├→ TMinimap::updateMinimap() — تحديث الخريطة
                    └→ updateFoldRegions() — تحديث الطي
```

### تدفق الميزات الجديدة (v4.0)
```
المستخدم يكتب → document()->contentsChanged()
                └→ TDocumentAnalyzer (debounce 500ms)
                    ├→ extractSymbols() — استخراج الرموز
                    ├→ checkErrors() — فحص الأخطاء
                    └→ emit analysisCompleted()
                        ├→ THoverManager — تحديث المعلومات
                        ├→ TDiagnosticManager — عرض الأخطاء
                        └→ CompletionStrategies — إكمال بالرموز الجديدة

المستخدم يمرّر الماوس → mouseMoveEvent()
                └→ THoverManager::checkHover()
                    └→ showHover() — عرض tooltip

المستخدم يضغط Ctrl+Click أو F12
                → TGoToDefinition::goToDefinition()
                    ├→ TSymbolTable::resolve() — البحث عن التعريف
                    └→ emit definitionFound() — فتح الملف
```

---

## [ARCHITECTURE]

### البنية الحالية (v3.3)
```
Taif (QMainWindow)
├── TMenuBar — قائمة عربية
├── QSplitter رئيسي
│   ├── QTreeView + QFileSystemModel — شجرة الملفات
│   └── QSplitter أفقي
│       ├── QTabWidget (محرر)
│       │   └── TEditor (per tab)
│       │       ├── TSyntaxHighlighter
│       │       │   └── TLexer (State Machine)
│       │       │       └── LanguageDefinition
│       │       ├── LineNumberArea
│       │       ├── TMinimap
│       │       └── QCompleter + CompletionModel + Strategies
│       └── QTabWidget (console)
│           ├── TConsole (interactive terminal)
│           └── TConsole (alif output)
├── TSettings (نافذة إعدادات)
└── ProcessWorker (تشغيل ألف)
```

### البنية الجديدة (v4.0)
```
Taif (QMainWindow)
├── TMenuBar
├── QSplitter رئيسي
│   ├── QTreeView + QFileSystemModel
│   └── QSplitter أفقي
│       ├── QTabWidget (محرر)
│       │   └── TEditor (per tab)
│       │       ├── TSyntaxHighlighter ← + Semantic Tokens
│       │       │   └── TLexer (State Machine)
│       │       ├── LineNumberArea
│       │       ├── TMinimap
│       │       ├── QCompleter + CompletionModel + Strategies
│       │       │   └── + UserDefinedStrategy [جديد]
│       │       ├── TDocumentAnalyzer [جديد]
│       │       │   ├── TSymbolTable [جديد]
│       │       │   └── Diagnostics [جديد]
│       │       ├── THoverManager [جديد]
│       │       ├── TGoToDefinition [جديد]
│       │       └── TDiagnosticManager [جديد]
│       └── QTabWidget (console)
│           ├── TConsole
│           ├── TConsole
│           └── ProblemsPanel [جديد]
├── TSettings
└── ProcessWorker
```

---

## [MILESTONES]

### Milestone 1: محلل المستند + جدول الرموز
**الملفات الجديدة:**
```
source/texteditor/analyzer/
├── TDocumentAnalyzer.h
├── TDocumentAnalyzer.cpp
├── TSymbolTable.h
├── TSymbolTable.cpp
└── TAnalyzerTypes.h
```

**المخرجات:**
- [ ] `TDocumentAnalyzer` يمر على المستند ويستخرج الرموز
- [ ] `TSymbolTable` يخزن الرموز مع النطاقات
- [ ] يكتشف: دوال، صنف، متغيرات، معاملات
- [ ] التعليق فوق التعريف يُستخدم كـ documentation
- [ ] debounce 500ms عند الكتابة
- [ ] لا يؤثر على الأداء

**المدة المقدرة:** 3-4 أيام

---

### Milestone 2: تعزيز الإكمال التلقائي
**الملفات المعدّلة:**
```
source/texteditor/autocomplete/
├── AutoComplete.h    ← + UserDefinedStrategy
└── AutoComplete.cpp
```

**المخرجات:**
- [ ] `UserDefinedStrategy` جديدة تقرأ من `TDocumentAnalyzer`
- [ ] إكمال بالدوال المستخدم定义
- [ ] إكمال بالمتغيرات
- [ ] إكمال بالصنف
- [ ] يظهر النوع والوصف في footer الإكمال

**المدة المقدرة:** 1-2 أيام

---

### Milestone 3: عرض Hover
**الملفات الجديدة:**
```
source/texteditor/hover/
├── THoverManager.h
└── THoverManager.cpp
```

**المخرجات:**
- [ ] Hover فوق الدوال → يعرض التوقيع + الوصف
- [ ] Hover فوق المتغيرات → يعرض النوع
- [ ] Hover فوق الكلمات المفتاحية → يعرض الوثائق
- [ ] تأخير 500ms قبل الإرسال (debounce)
- [ ] تصميم tooltip أنيق

**المدة المقدرة:** 2-3 أيام

---

### Milestone 4: الانتقال إلى التعريف
**الملفات الجديدة:**
```
source/texteditor/goto/
├── TGoToDefinition.h
└── TGoToDefinition.cpp
```

**المخرجات:**
- [ ] Ctrl+Click يعمل على identifiers
- [ ] F12 يعمل
- [ ] Alt+Left يرجع للموقع السابق (navigation stack)
- [ ] يفتح ملفات خارجية إذا لزم
- [ ] في `Taif.h` → `openFileAtPosition()`

**المدة المقدرة:** 2-3 أيام

---

### Milestone 5: اكتشاف الأخطاء
**الملفات الجديدة:**
```
source/texteditor/diagnostics/
├── TDiagnosticManager.h
└── TDiagnosticManager.cpp
```

**الملفات المعدّلة:**
```
taif/Taif.h              ← + ProblemsPanel
taif/Taif.cpp
```

**المخرجات:**
- [ ] خطوط تحتية حمراء للأخطاء (WaveUnderline)
- [ ] خطوط تحتية صفراء للتحذيرات
- [ ] tooltip يعرض رسالة الخطأ
- [ ] Problems Panel في أسفل النافذة
- [ ] أنواع الأخطاء:
  - [ ] متغير غير معرّف
  - [ ] دالة بمعاملات خاطئة
  - [ ] استيراد غير موجود
  - [ ] return خارج دالة

**المدة المقدرة:** 2-3 أيام

---

### Milestone 6: تلوين دلالي (Semantic Highlighting)
**الملفات المعدّلة:**
```
source/texteditor/highlighter/
├── TToken.h           ← + UserFunction, UserClass, Parameter tokens
├── TSyntaxHighlighter.cpp  ← + semantic token coloring
└── TSyntaxThemes.h    ← + function/class colors
```

**المخرجات:**
- [ ] دوال المستخدم بلون مميز
- [ ] صنف المستخدم بلون مميز
- [ ] معاملات بلون مختلف
- [ ] `this` بلون مميز

**المدة المقدرة:** 1-2 أيام

---

### Milestone 7: اختبار وتكامل
**المخرجات:**
- [ ] اختبار كل Milestone
- [ ] اختبار الأداء (لا تأثير على سرعة الكتابة)
- [ ] اختبار عبر الملفات
- [ ] تنظيف الكود وتوثيق

**المدة المقدرة:** 2-3 أيام

---

## [ARCHITECTURE_DECISIONS]

### 1. لماذا بدون LSP؟
- الميزات الحالية (تلوين + إكمال) مبنية محلياً وتعمل بكفاءة
- Alif-LSP في بدايته ولا يملك parser
- بناء محلي = تحكم كامل + أداء أفضل
- لا إضافات خارجية = لا مشاكل توافق

### 2. لماذا نستخدم TLexer الحالي؟
- مُختبر ومُحسّن
- يدعم cross-line state (F-strings, triple-quoted)
- لا حاجة لبناء lexer جديد

### 3. لماذا debounce 500ms؟
- يمنع التحليل عند كل ضغطة
- يكفي للمستخدم (لا يشعر بالتأخير)
- يحافظ على الأداء

### 4. لماذا نستخدم Strategy Pattern للإكمال؟
- متوافق مع البنية الحالية
- يسمح بإضافة مصادر إكمال جديدة بسهولة
- كل strategy مستقلة وقابلة للاختبار

---

## [DEPENDENCIES]

### داخلية (داخل المشروع)
- `TEditor` ← `TDocumentAnalyzer` (يُمرّر في constructor)
- `TDocumentAnalyzer` ← `TSymbolTable` (مُملّك من محلل)
- `THoverManager` ← `TDocumentAnalyzer` (للاستعلام)
- `TGoToDefinition` ← `TDocumentAnalyzer` (للبحث)
- `TDiagnosticManager` ← `TDocumentAnalyzer` (للأخطاء)
- `Taif` ← كل المكونات (يُنشئها ويُديرها)

### خارجية
- لا توجد مكتبات خارجية جديدة

---

## [ORPHANS_AND_PENDING]

### غير مُنجز
- [ ] Problems Panel UI
- [ ] Navigation Stack (Alt+Left)
- [ ] Unit tests
- [ ] توثيق الكود

### مُعلّق
- [ ] دعم ملفات متعددة (cross-file analysis)
- [ ] Find References (Shift+F12)
- [ ] Rename Refactoring (Ctrl+Rename)
- [ ] Code Actions (quick fixes)

### مُلغي
- ~~LSP Client~~ — تم الإلغاء (بناء محلي أفضل)
- ~~LSP Server~~ — تم الإلغاء

---

## [FILE_STRUCTURE]

```
source/texteditor/
├── TEditor.h / .cpp                  — المحرر الرئيسي
├── analyzer/                          — [جديد] محلل المستند
│   ├── TAnalyzerTypes.h
│   ├── TDocumentAnalyzer.h / .cpp
│   └── TSymbolTable.h / .cpp
├── autocomplete/                      — الإكمال التلقائي
│   ├── AutoComplete.h / .cpp
│   └── AutoCompleteUI.h / .cpp
├── highlighter/                       — التلوين
│   ├── TLexer.h / .cpp
│   ├── TSyntaxHighlighter.h / .cpp
│   ├── TSyntaxDefinition.h / .cpp
│   ├── TSyntaxThemes.h
│   └── TToken.h
├── hover/                             — [جديد] عرض المعلومات
│   └── THoverManager.h / .cpp
├── goto/                              — [جديد] الانتقال
│   └── TGoToDefinition.h / .cpp
└── diagnostics/                       — [جديد] عرض الأخطاء
    └── TDiagnosticManager.h / .cpp
```

---

## [VERIFICATION_CRITERIA]

### Milestone 1
- [ ] المحلل يكتشف 5+ دوال في ملف اختبار
- [ ] النطاقات صحيحة (function scope داخل global scope)
- [ ] لا crash عند حذف كل النص

### Milestone 2
- [ ] الإكمال يعرض "دالة_جديدة" إذا تم تعريفها
- [ ] لا تكرار في الإكمال

### Milestone 3
- [ ] Hover يظهر خلال < 500ms
- [ ] لا tooltip زائف

### Milestone 4
- [ ] Ctrl+Click ينتقل للتعريف
- [ ] Alt+Left يرجع

### Milestone 5
- [ ] خطأ "متغير غير معرّف" يظهر تحت التسطير
- [ ] Problems Panel يعرض الأخطاء

### Milestone 6
- [ ] دوال المستخدم بلون مختلف عن المدمجة
