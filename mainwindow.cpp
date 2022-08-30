#include "mainwindow.h"
#include "ui_mainWin.h"
#include "PopupQToolButton.h"
#include "ColorQToolButton.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connectSignalSlots();
    buildToolBar();

    // Initialize status bar
    statusLabel = new QLabel("Ready");
    ui->statusbar->addWidget(statusLabel);
    progressBar = new QProgressBar();
    progressBar->setMaximumHeight(17);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectSignalSlots()
{
    // File menu
    QObject::connect( ui->openAct,       &QAction::triggered,    ui->bookmarks,  &Bookmarks::readFiles );
    QObject::connect( ui->insertAct,     &QAction::triggered,    ui->bookmarks,  &Bookmarks::readFiles );
    QObject::connect( ui->replaceAct,    &QAction::triggered,    ui->bookmarks,  &Bookmarks::readFiles );
    QObject::connect( ui->saveFilesAct,  &QAction::triggered,    ui->bookmarks,  &Bookmarks::saveFiles );
    QObject::connect( ui->saveAsAct,     &QAction::triggered,    ui->bookmarks,  &Bookmarks::saveAsFiles );
    QObject::connect( ui->createTIFFAct, &QAction::triggered,    ui->bookmarks,  &Bookmarks::createTIFF );
    QObject::connect( ui->exitAct,       &QAction::triggered,    this,           &MainWindow::close );

    // Interconnects
    QObject::connect( ui->bookmarks,     &Bookmarks::progressSig, this,          &MainWindow::updateProgress );
}

void MainWindow::buildToolBar()
{
    // Open button
    QMenu *openMenu = new QMenu();
    openMenu->addAction(ui->openAct);
    openMenu->addAction(ui->insertAct);
    openMenu->addAction(ui->replaceAct);
    PopupQToolButton *openToolButton = new PopupQToolButton();
    openToolButton->setMenu(openMenu);
    openToolButton->setDefaultAction(ui->openAct);
    ui->toolBar->addWidget(openToolButton);

    // Save button
    QMenu *saveMenu = new QMenu();
    saveMenu->addAction(ui->saveFilesAct);
    saveMenu->addAction(ui->saveAsAct);
    saveMenu->addAction(ui->createTIFFAct);
    PopupQToolButton *saveToolButton = new PopupQToolButton();
    saveToolButton->setMenu(saveMenu);
    saveToolButton->setDefaultAction(ui->saveFilesAct);
    ui->toolBar->addWidget(saveToolButton);

    // Delete actions
    ui->toolBar->addAction(ui->deleteAct);

    // Rotate button
    QMenu *rotateMenu = new QMenu();
    rotateMenu->addAction(ui->rotateCWAct);
    rotateMenu->addAction(ui->rotateCCWAct);
    rotateMenu->addAction(ui->rotate180Act);
    PopupQToolButton *rotateToolButton = new PopupQToolButton();
    rotateToolButton->setMenu(rotateMenu);
    rotateToolButton->setDefaultAction(ui->rotateCWAct);
    ui->toolBar->addWidget(rotateToolButton);

    // Edit actions
    ui->toolBar->addAction(ui->undoAct);
    ui->toolBar->addAction(ui->redoAct);
    ui->toolBar->addAction(ui->zoomOutAct);
    ui->toolBar->addAction(ui->zoomInAct);

    // Tool button
    QMenu *toolsMenu = new QMenu();
    toolsMenu->addAction(ui->pointerAct);
    toolsMenu->addAction(ui->pencilAct);
    toolsMenu->addAction(ui->eraserAct);
    toolsMenu->addAction(ui->areaFillAct);
    PopupQToolButton *toolsToolButton = new PopupQToolButton();
    toolsToolButton->setMenu(toolsMenu);
    toolsToolButton->setDefaultAction(ui->pointerAct);
    ui->toolBar->addWidget(toolsToolButton);

    // Line button
    QMenu *toolSizeMenu = new QMenu();
    toolSizeMenu->addAction(ui->pix1Act);
    toolSizeMenu->addAction(ui->pix4Act);
    toolSizeMenu->addAction(ui->pix8Act);
    toolSizeMenu->addAction(ui->pix12Act);
    PopupQToolButton *toolSizeToolButton = new PopupQToolButton();
    toolSizeToolButton->setMenu(toolSizeMenu);
    toolSizeToolButton->setDefaultAction(ui->pix1Act);
    ui->toolBar->addWidget(toolSizeToolButton);

    // Color button
    ColorQToolButton *colorToolButton = new ColorQToolButton();
    colorToolButton->setDefaultAction(ui->colorAct);
    colorToolButton->setIcon(ui->viewer->foregroundColor, ui->viewer->backgroundColor);
    ui->toolBar->addWidget(colorToolButton);
}

void MainWindow::updateProgress(QString descr, int val)
{
    if (val < 0)
    {
        ui->statusbar->removeWidget(progressBar);
        statusLabel->setText("Ready");
    }
    else if (descr != "")
    {
        statusLabel->setText(descr);
        progressBar->setRange(0, val);
        progressBar->setValue(0);
        ui->statusbar->addWidget(progressBar);
    }
    else
    {
        progressBar->setValue(val);
    }
}
