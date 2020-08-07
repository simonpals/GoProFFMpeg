#include "logswindow.h"
#include "ui_logswindow.h"
#include "MainWindow.h"

LogsWindow::LogsWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LogsWindow),
    m_mainWindow(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("Логи обработки видео");
//    setWindowModality(Qt::WindowModal);
    setModal(true);
}

LogsWindow::~LogsWindow()
{
    delete ui;
}

QTextEdit *LogsWindow::getLogsList()
{
    return ui->logsListView;
}

void LogsWindow::setPerformedPath(const QString &path)
{
    ui->performedFilePath->setText(path);
}

void LogsWindow::setPerformedCommand(const QString &command)
{
    ui->performedCommand->setText(command);
}

void LogsWindow::on_pushButton_clicked()
{
    m_mainWindow->on_stopConvert_clicked();
    close();
}
