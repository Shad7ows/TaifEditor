#include "Spectrum.h"

#include <QDockWidget>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QShortcut>
#include <QGuiApplication>
#include <QScreen>
#include <QCoreApplication>
#include <QTextStream>
#include <QApplication>


Spectrum::Spectrum(const QString& filePath, QWidget *parent)
    : QMainWindow(parent) {
    QScreen* screenSize = QGuiApplication::primaryScreen();
    this->setGeometry(screenSize->size().width() / 3, screenSize->size().height() / 7, 600, 700);
    setStyleSheet(R"(
            QMainWindow::separator {
                background-color: #2a2c44;
                width: 4px;
                height: 4px;
            }
            
            QMainWindow::separator:hover {
                background-color: #393c5d;
            })");

    QWidget* center = new QWidget(this);
    QVBoxLayout* vlay = new QVBoxLayout(center);
    vlay->setContentsMargins(0, 0, 0, 0);
    vlay->setSpacing(0);

    editor = new SPEditor(this);
    //terminal = new Terminal(this);
    //folderTree = new FolderTree(editor, this);
    menuBar = new SPMenuBar(this);
    setMenuBar(menuBar);

    updateWindowTitle();

    vlay->addWidget(editor);
    //vlay->addWidget(terminal);


    //addDockWidget(Qt::BottomDockWidgetArea, terminal); // يجب أن تكون بعد vlay->addWidget(terminal)
    //addDockWidget(Qt::RightDockWidgetArea, folderTree);
    this->setCentralWidget(center);

    // لتشغيل ملف ألف بإستخدام محرر طيف عند إختيار المحرر ك برنامج للتشغيل
    if (!filePath.isEmpty()) {
        this->openFile(filePath);
    }

    // Create a shortcut for Ctrl+S
    QShortcut* saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, &Spectrum::saveFile);

    connect(menuBar, &SPMenuBar::newRequested, this, &Spectrum::newFile);
    connect(menuBar, &SPMenuBar::openRequested, this, [this](){this->openFile("");});
    connect(menuBar, &SPMenuBar::saveRequested, this, &Spectrum::saveFile);
    connect(menuBar, &SPMenuBar::saveAsRequested, this, &Spectrum::saveFileAs);
    connect(menuBar, &SPMenuBar::settingsRequest, this, &Spectrum::openSettings);
    connect(menuBar, &SPMenuBar::exitRequested, this, &Spectrum::exitApp);
    connect(menuBar, &SPMenuBar::runRequested, this, &Spectrum::runAlif);
    connect(menuBar, &SPMenuBar::aboutRequested, this, &Spectrum::aboutSpectrum);
    connect(editor, &SPEditor::openRequest, this, [this](QString filePath){this->openFile(filePath);});

    // Connect modification signal so when doc modified it's add "*"
    connect(editor->document(), &QTextDocument::modificationChanged,
            this, &Spectrum::onModificationChanged);
}

Spectrum::~Spectrum() {
    QSettings settings("Alif", "Spectrum");
    settings.setValue("editorFontSize", editor->font().pointSize());

    delete editor;
    delete menuBar;
}

// الكتابة فوق دالة إغلاق البرنامج الرئيسية
void Spectrum::closeEvent(QCloseEvent *event) {
    int isNeedSave = needSave();
    if (!isNeedSave) {
        event->ignore();
        return;
    }
    else if (isNeedSave == 1) {
        this->saveFile();
        event->ignore();
        return;
    }

    QApplication::quit();
    event->accept();
}


/* ----------------------------------- File Menu Button ----------------------------------- */

int Spectrum::needSave() {
    if (editor->document()->isModified()) {
        QMessageBox::StandardButton ret{};
        ret = QMessageBox::warning(nullptr, "ألف",
                                   "تم التعديل على الملف.\n"
                                   "هل تريد حفظ التغييرات؟",
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            return 1;
        }
        else if (ret == QMessageBox::Cancel) {
            return 0;
        }
    }

    return 2;
}

void Spectrum::newFile() {
    int isNeedSave = needSave();
    if (!isNeedSave) {
        return;
    }
    else if (isNeedSave == 1) {
        this->saveFile();
    }

    currentFilePath.clear();
    // editor->clear(); // تجعل الملف الجديد كأنه معدل عليه ولذلك تم إستبدالها بما تحتها الى حين إيجاد الحل
    editor->document()->setPlainText(""); // مؤقت
    editor->document()->setModified(false);
    updateWindowTitle();
}

void Spectrum::openFile(QString filePath) {
    int isNeedSave = needSave();
    if (!isNeedSave) {
        return;
    }
    else if (isNeedSave == 1) {
        this->saveFile();
    }

    if (filePath.isEmpty()) {
        filePath = QFileDialog::getOpenFileName(nullptr, "فتح ملف", "", "ملف ألف (*.alif *.aliflib);;All Files (*)");
    }
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            editor->document()->setPlainText(content);
            file.close();
            currentFilePath = filePath;
            editor->document()->setModified(false);
            updateWindowTitle();

            // حل مؤقت لارجاع المؤشرة الى بداية الملف حيث أنه يجب أن تظهر في البداية بشكل إفتراضي
            QTextCursor cursor = editor->textCursor();
            cursor.setPosition(0);
            editor->setTextCursor(cursor);
        }
        else {
            QMessageBox::warning(nullptr, "خطأ", "لا يمكن فتح الملف");
        }
    }
}

void Spectrum::saveFile() {
    QString content = editor->document()->toPlainText();
    if (currentFilePath.isEmpty()) {
        saveFileAs();
    }
    else {
        QFile file(currentFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << content;
            file.close();
            editor->document()->setModified(false);
            updateWindowTitle();
        }
        else {
            QMessageBox::warning(nullptr, "خطأ", "لا يمكن حفظ الملف");
        }
    }
}


void Spectrum::saveFileAs() {
    QString content = editor->document()->toPlainText();
    QString fileName = QFileDialog::getSaveFileName(nullptr, "حفظ الملف", "ملف جديد", "ملف ألف (*.alif);;مكتبة ألف(*.aliflib);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << content;
            file.close();
            currentFilePath = fileName;
            editor->document()->setModified(false);
            updateWindowTitle();
        }
        else {
            QMessageBox::warning(nullptr, "خطأ", "لا يمكن حفظ الملف");
        }
    }
}


void Spectrum::openSettings() {
    // Prevent multiple settings windows
    if (settings and settings->isVisible()) return;

    settings = new SPSettings(this);
    connect(settings, &SPSettings::fontSizeChanged, editor, &SPEditor::updateFontSize);

    settings->show();
}


void Spectrum::exitApp() {
    int isNeedSave = needSave();
    if (!isNeedSave) {
        return;
    }
    else if (isNeedSave == 1) {
        this->saveFile();
        return;
    }

    QApplication::quit();
}


/* ----------------------------------- Run Menu Button ----------------------------------- */

void Spectrum::runAlif() {
    QString program{};
    QStringList args{};
    QString command{};
    QStringList arguments{currentFilePath};
    QString workingDirectory = QCoreApplication::applicationDirPath();

    if (currentFilePath.isEmpty() or editor->document()->isModified()) {
        QMessageBox::warning(nullptr, "تنبيه", "قم بحفظ الملف لتشغيله");
        return;
    }

#if defined(Q_OS_WINDOWS)
    // Windows: Start cmd.exe with /K to keep the window open
    program = "cmd.exe";
    command = "alif\\alif.exe";
    args << "/C" << "start" << program << "/K" << command << arguments;
#elif defined(Q_OS_LINUX)
    // Linux: Use x-terminal-emulator with -e to execute the command
    program = "x-terminal-emulator";
    command = "./alif/alif";
    if (!arguments.isEmpty()) {
        command += " " + arguments.join(" ");
    }
    command += "; exec bash";
    args << "-e" << "bash" << "-c" << command;
#elif defined(Q_OS_MACOS)
    // macOS: Use AppleScript to run the command in Terminal.app
    program = "osascript";
    command = "./alif/alif";

    // Escape each part for shell execution
    QStringList allParts = QStringList() << command << arguments;
    QStringList escapedShellParts;
    for (const QString &part : allParts) {
        QString escaped = part;
        escaped.replace("'", "'\"'\"'"); // Escape single quotes for AppleScript
        escapedShellParts << "'" + escaped + "'";
    }
    QString shellCommand = escapedShellParts.join(" ");

    // Escape double quotes for AppleScript
    QString escapedAppleScriptCommand = shellCommand.replace("\"", "\\\"");

    // Construct AppleScript
    QString script = QString(
                         "tell application \"Terminal\"\n"
                         "    activate\n"
                         "    do script \"cd '%1' && %2\"\n"
                         "end tell"
                         ).arg(workingDirectory, escapedAppleScriptCommand);

    args << "-e" << script;
#endif

    QProcess* process = new QProcess(this);
    process->setWorkingDirectory(workingDirectory);

    process->start(program, args);
}







/* ----------------------------------- Help Menu Button ----------------------------------- */


void Spectrum::aboutSpectrum() {
    QMessageBox::information(nullptr,
                             "عن المحرر"
                             ,R"(
محرر طيف (نــ1.0) 1445-1446

© الحقوق محفوظة لصالح
برمجيات ألف - عبدالرحمن ومحمد الخطيب

محرر نصي خاص بلغة ألف نـ5
يعمل على جميع المنصات "ويندوز - لينكس - ماك"
ـــــــــــــــــــــــــــــــــــــــــــــــــــــ
المحرر لا يزال تحت التطوير وقد يحتوي بعض الاخطاء
نرجو تبليغ مجتمع ألف في حال وجود أي خطأ
https://t.me/aliflang
ـــــــــــــــــــــــــــــــــــــــــــــــــــــ
فريق التطوير لا يمتلك أي ضمانات وغير مسؤول
عن أي خطأ او خلل قد يحدث بسبب المحرر.

المحرر يخضع لرخصة برمجيات ألف
يجب قراءة الرخصة جيداً قبل البدأ بإستخدام المحرر
                            )");
}





/* ----------------------------------- Other Functions ----------------------------------- */

void Spectrum::updateWindowTitle() {
    QString title{};
    if (currentFilePath.isEmpty()) {
        title = "غير معنون[*]";
    } else {
        title = QFileInfo(currentFilePath).fileName() + "[*]";
    }
    title += " - طيف";
    setWindowTitle(title);
}

void Spectrum::onModificationChanged(bool modified) {
    this->setWindowModified(modified);
}











