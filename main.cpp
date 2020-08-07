#include "MainWindow.h"
#include <QApplication>

#include <QFile>
#include <QFontDatabase>
#include <QStyleFactory>

void setStyleSheet() {
    QFile styleF;
    styleF.setFileName(":/style.css");
    styleF.open(QFile::ReadOnly);
    QString qssStr = styleF.readAll();
    styleF.close();

    qApp->setStyleSheet(qssStr);
}

void loadFonts() {
    QFontDatabase::addApplicationFont(":/fonts/open_sans_semibold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/open_sans_bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/open_sans.ttf");
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;

     // Customize the color palette for the interface elements
     darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
     darkPalette.setColor(QPalette::WindowText, Qt::white);
     darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
     darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
     darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
     darkPalette.setColor(QPalette::ToolTipText, Qt::white);
     darkPalette.setColor(QPalette::Text, Qt::white);
     darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
     darkPalette.setColor(QPalette::ButtonText, Qt::white);
     darkPalette.setColor(QPalette::BrightText, Qt::red);
     darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
     darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
     darkPalette.setColor(QPalette::HighlightedText, Qt::black);

     // Install this palette
     qApp->setPalette(darkPalette);
//    setStyleSheet();
    loadFonts();

    MainWindow w;
    w.show();

    return a.exec();
}
