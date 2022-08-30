#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>

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
    void updateProgress(QString descr, int val);
    void updateActions();
    void about();

private:
    Ui::MainWindow *ui;
    void connectSignalSlots();
    void buildToolBar();
    void colorMagic();

    QLabel *statusLabel;
    QProgressBar *progressBar;
};
#endif // MAINWINDOW_H
