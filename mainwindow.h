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
    SpinWidget *dropperThresholdWidget;
    QAction *dropperThresholdSpin;
    SpinWidget *floodThresholdWidget;
    QAction *floodThresholdSpin;
    DoubleSpinWidget *deskewWidget;
    QAction *deskewSpin;
    SpinWidget *despeckleWidget;
    QAction *despeckleSpin;
    SpinWidget *blurWidget;
    QAction *blurSpin;
};
#endif // MAINWINDOW_H
