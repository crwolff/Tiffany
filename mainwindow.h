#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFont>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include "Widgets/ColorQToolButton.h"
#include "Widgets/DoubleSpinWidget.h"
#include "Widgets/OddSpinWidget.h"
#include "Widgets/PopupQToolButton.h"
#include "Widgets/SpinWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void colorMagic();
    void fontSelect();
    void updateProgress(QString descr, int val);
    void setStatus(QString descr);
    void about();
    void closeEvent (QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    void connectSignalSlots();
    void buildToolBar();
    void makeDropperVisible(int mask);
    void makeDespeckleVisible(int mask);
    QLabel *statusLabel;
    QProgressBar *progressBar;

    PopupQToolButton *zoomToolButton;
    SpinWidget *bgRemoveThresholdWidget;
    QAction *bgRemoveThresholdSpin;
    SpinWidget *dropperThresholdWidget;
    QAction *dropperThresholdSpin;
    SpinWidget *floodThresholdWidget;
    QAction *floodThresholdSpin;
    SpinWidget *despeckleWidget;
    QAction *despeckleSpin;
    SpinWidget *devoidWidget;
    QAction *devoidSpin;
    DoubleSpinWidget *deskewWidget;
    QAction *deskewSpin;
    OddSpinWidget *blurWidget;
    QAction *blurSpin;
    OddSpinWidget *kernelWidget;
    QAction *kernelSpin;

    ColorQToolButton colorToolButton;
};
#endif // MAINWINDOW_H
