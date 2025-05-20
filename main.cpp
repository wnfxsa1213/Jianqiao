#include "JianqiaoCoreShell.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>

// Global file object for logging - ensure it's accessible and managed correctly.
// For simplicity in a single file main.cpp, a static or global instance can be used,
// but proper RAII / explicit open/close is better in larger apps.
static QFile logFile;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (!logFile.isOpen()) {
        // Attempt to open the log file if it's not already open.
        // This is a fallback, ideally it's opened once in main.
        QString logFilePath = QDir::currentPath() + "/log.txt";
        logFile.setFileName(logFilePath);
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            // Cannot open log file, so just output to console as a last resort
            fprintf(stderr, "Cannot open log file %s. Logging to console: %s\n", qPrintable(logFilePath), qPrintable(msg));
            return;
        }
    }

    QTextStream out(&logFile);
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    out << timestamp << " ";

    switch (type) {
    case QtDebugMsg:       out << "DEBUG:   "; break;
    case QtInfoMsg:        out << "INFO:    "; break;
    case QtWarningMsg:     out << "WARNING: "; break;
    case QtCriticalMsg:    out << "CRITICAL:"; break;
    case QtFatalMsg:       out << "FATAL:   "; break;
    }

    // Include file and line info if available and useful (can be verbose)
    // out << context.file << ":" << context.line << " "; 
    out << msg << "\n";
    out.flush(); // Ensure data is written immediately

    // For fatal messages, abort after logging.
    if (type == QtFatalMsg)
        abort();
}

int main(int argc, char *argv[])
{
    // Open the log file at the beginning of the application run.
    // Overwrite existing log file each time.
    QString logFilePath = QDir::currentPath() + "/log.txt";
    logFile.setFileName(logFilePath);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream(&logFile) << "--- Log started at " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " ---\n";
        logFile.flush();
    } else {
        fprintf(stderr, "Failed to open log file: %s\n", qPrintable(logFilePath));
        // Continue without file logging if it fails to open initially.
    }

    qInstallMessageHandler(myMessageOutput);

    QApplication a(argc, argv);

    qDebug() << "Application started."; // This should now go to log.txt

    JianqiaoCoreShell w;
    w.show();

    int result = a.exec();
    qDebug() << "Application finished with exit code:" << result;

    if (logFile.isOpen()) {
        QTextStream(&logFile) << "--- Log finished at " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " ---\n";
        logFile.close();
    }
    return result;
} 