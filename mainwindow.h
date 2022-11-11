#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include "ColorQToolButton.h"
#include "DoubleSpinWidget.h"
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
    void closeEvent (QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    void connectSignalSlots();
    void makeVisible(int mask);
    void buildToolBar();
    void colorMagic();

    QLabel *statusLabel;
    QProgressBar *progressBar;
    SpinWidget *thresholdWidget;
    QAction *threshold;
    DoubleSpinWidget *deskewWidget;
    QAction *deskew;
    SpinWidget *despeckleWidget;
    QAction *despeckle;
};
#endif // MAINWINDOW_H
