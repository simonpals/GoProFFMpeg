#ifndef VIDEOEFFECTS_H
#define VIDEOEFFECTS_H

#include <QWidget>

class MainWindow;
namespace Ui {
class VideoEffects;
}

class VideoEffects : public QWidget
{
    Q_OBJECT

public:
    explicit VideoEffects(QWidget *parent = nullptr);
    ~VideoEffects();

private slots:
    void on_openVideoDialog_clicked();
    void on_saveVideoDialog_clicked();
    void on_audioOverlayDialog_clicked();
    void on_imageOverlayDialog_clicked();
    void on_videoOverlayDialog_clicked();
    void on_introVideoDialog_clicked();
    void on_outroVideoDialog_clicked();

private:
    bool checkVideoPathes();
    QString openDirectoryDialog();
    QString openFileDialog();

    Ui::VideoEffects *ui;
    friend class MainWindow;
};

#endif // VIDEOEFFECTS_H
