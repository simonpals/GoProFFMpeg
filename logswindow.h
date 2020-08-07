#ifndef LOGSWINDOW_H
#define LOGSWINDOW_H

#include <QDialog>

class QTextEdit;
namespace Ui {
class LogsWindow;
}
class MainWindow;

class LogsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LogsWindow(QWidget *parent = nullptr);
    ~LogsWindow();

    QTextEdit *getLogsList();
    void setMainWindow(MainWindow *mv) { m_mainWindow = mv; }
    void setPerformedPath(const QString &path);
    void setPerformedCommand(const QString &command);

private slots:
    void on_pushButton_clicked();

private:
    Ui::LogsWindow *ui;
    MainWindow *m_mainWindow;
};

#endif // LOGSWINDOW_H
