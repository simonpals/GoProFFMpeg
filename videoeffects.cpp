#include <QDir>
#include <QFile>
#include <QFileDialog>
#include "videoeffects.h"
#include "ui_videoeffects.h"

VideoEffects::VideoEffects(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VideoEffects)
{
    ui->setupUi(this);
    setWindowTitle("Видеоэффекты");
}

VideoEffects::~VideoEffects()
{
    delete ui;
}

QString VideoEffects::openDirectoryDialog()
{
    QString dir = QFileDialog::getExistingDirectory(nullptr, ("Выбрать выходную папку"), QDir::currentPath());
    return dir;
}

QString VideoEffects::openFileDialog()
{
//    QString filter = "File Description (*.extention)";
    // For example: "Mpeg Layer 3 music files (*.mp3)"
    QString file = QFileDialog::getOpenFileName(this, "Выберите файл..."/*, QDir::homePath(), filter*/);
    return file;
}

void VideoEffects::on_openVideoDialog_clicked()
{
    ui->originalVideoPath->setText(openDirectoryDialog());
}

void VideoEffects::on_saveVideoDialog_clicked()
{
    ui->savingVideoPath->setText(openDirectoryDialog());
}

bool VideoEffects::checkVideoPathes()
{
    return QFile::exists(ui->originalVideoPath->text()) &&
            QFile::exists(ui->savingVideoPath->text());
}

void VideoEffects::on_audioOverlayDialog_clicked()
{
    ui->audioOverlayPath->setText(openFileDialog());
}

void VideoEffects::on_imageOverlayDialog_clicked()
{
    ui->imageOverlayPath->setText(openFileDialog());
}

void VideoEffects::on_videoOverlayDialog_clicked()
{
    ui->videoOverlayPath->setText(openFileDialog());
}

void VideoEffects::on_introVideoDialog_clicked()
{
    ui->introVideoPath->setText(openFileDialog());
}

void VideoEffects::on_outroVideoDialog_clicked()
{
    ui->outroVideoPath->setText(openFileDialog());
}
