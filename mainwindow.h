#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include "ColorQToolButton.h"
#include "SpinWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    ColorQToolButton colorToolButton;

public slots:
    void updateProgress(QString descr, int val);
    void updateActions(float scale);
    void about();

private:
    Ui::MainWindow *ui;
    void connectSignalSlots();
    void buildToolBar();
    void colorMagic();

    QLabel *statusLabel;
    QProgressBar *progressBar;
    SpinWidget *thresholdWidget;
    QAction *threshold;
    SpinWidget *deskewWidget;
    QAction *deskew;
};
#endif // MAINWINDOW_H
