#include "mainwindow.h"
#include "ui_mainWin.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connectSignalSlots();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectSignalSlots()
{
    // File menu
    QObject::connect(ui->openAct,       &QAction::triggered, ui->bookmarks, &Bookmarks::readFiles);
    QObject::connect(ui->insertAct,     &QAction::triggered, ui->bookmarks, &Bookmarks::readFiles);
    QObject::connect(ui->replaceAct,    &QAction::triggered, ui->bookmarks, &Bookmarks::readFiles);
    QObject::connect(ui->saveFilesAct,  &QAction::triggered, ui->bookmarks, &Bookmarks::saveFiles);
    QObject::connect(ui->saveAsAct,     &QAction::triggered, ui->bookmarks, &Bookmarks::saveAsFiles);
    QObject::connect(ui->createTIFFAct, &QAction::triggered, ui->bookmarks, &Bookmarks::createTIFF);
    QObject::connect(ui->exitAct,       &QAction::triggered, this,          &MainWindow::close);

    // Interconnects
    QObject::connect(ui->bookmarks,     &Bookmarks::progressSig, this,      &MainWindow::updateProgress);
}

void MainWindow::updateProgress(QString descr, int val)
{
    qInfo() << descr << val;
}
