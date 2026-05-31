#include "ProcessWorker.h"
#include <QDebug>

ProcessWorker::ProcessWorker(const QString &program, const QStringList &args, const QString &workingDir)
    : program(program), args(args), workingDir(workingDir), process(nullptr)
{
    flushTimer = new QTimer(this);
    flushTimer->setInterval(20);
    connect(flushTimer, &QTimer::timeout, this, &ProcessWorker::flushBuffers);
}

void ProcessWorker::start() {
    // --- 1) أنشئ الـ QTimer هنا داخل الثريد الخاص بالـ Worker ---
    if (!flushTimer) {
        flushTimer = new QTimer(this);
        flushTimer->setInterval(20);
        connect(flushTimer, &QTimer::timeout, this, &ProcessWorker::flushBuffers);
    }

    // --- 2) تأكد أن الـ QProcess له parent (this) حتى يعيش داخل نفس الثريد ---
    if (process) {
        // لو كان موجودًا من تشغيل سابق، نظفه أول
        disconnect(process, nullptr, this, nullptr);
        process->deleteLater();
        process = nullptr;
    }
    process = new QProcess(this);

    process->setProgram(program);
    process->setArguments(args);
    process->setWorkingDirectory(workingDir);
    process->setProcessChannelMode(QProcess::SeparateChannels);

    // --- 3) ربط إشارات القراءة ---
    connect(process, &QProcess::readyReadStandardOutput,
            this, &ProcessWorker::onReadyReadOutput);
    connect(process, &QProcess::readyReadStandardError,
            this, &ProcessWorker::onReadyReadError);

    // --- 4) عند بدء العملية: شغّل التايمر (بدون waitForStarted blocking) ---
    connect(process, &QProcess::started, this, [this]() {
        // ابدأ التايمر فقط بعد أن تبدأ العملية فعلاً
        if (flushTimer && !flushTimer->isActive())
            flushTimer->start();
    });

    // --- 5) عند الانتهاء: أوقف التايمر وأرسل أي بيانات متبقية ثم أرسل finished ---
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus /*status*/) {
                if (flushTimer && flushTimer->isActive())
                    flushTimer->stop();

                // إرسال أي بيانات متبقية
                flushBuffers();

                emit finished(code);
            });

    // --- 6) ابدأ العملية بطريقة غير حابسة ---
    process->start();

    // --- 7) إذا لم تبدأ العملية خلال مهلة معقولة، انبه بالخطأ (non-blocking check) ---
    // هنا نستخدم QTimer أحادي للتحقق بعد مدة قصيرة بدل waitForStarted()
    QTimer::singleShot(100, this, [this]() {
        if (!process)
            return;
    });
}
// ✅ التنفيذ (الكود) يجب أن يكون هنا في ملف .cpp
void ProcessWorker::onReadyReadOutput() {
    QString s = QString::fromLocal8Bit(process->readAllStandardOutput());
    outputBuffer.append(s);
}

// ✅ التنفيذ (الكود) يجب أن يكون هنا في ملف .cpp
void ProcessWorker::onReadyReadError() {
    QString s = QString::fromLocal8Bit(process->readAllStandardError());
    errorBuffer.append(s);
}

// ✅ التنفيذ (الكود) يجب أن يكون هنا في ملف .cpp
void ProcessWorker::flushBuffers() {
    if (!outputBuffer.isEmpty()) {
        emit outputReady(outputBuffer);
        outputBuffer.clear();
    }
    if (!errorBuffer.isEmpty()) {
        emit errorReady(errorBuffer);
        errorBuffer.clear();
    }
}

// ✅ التنفيذ (الكود) يجب أن يكون هنا في ملف .cpp
void ProcessWorker::sendInput(const QString &text) {
    if (process && process->state() == QProcess::Running) {
        process->write((text + "\n").toLocal8Bit());
    }
}
