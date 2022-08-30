#include "mainwindow.h"
#include "ui_mainWin.h"
#include "PopupQToolButton.h"
#include "ColorQToolButton.h"
#include <QDebug>

//
// Constructor
//
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

//
// Destructor
//
MainWindow::~MainWindow()
{
    delete ui;
}

//
// Connect signals to slots
//
void MainWindow::connectSignalSlots()
{
    // File menu
    QObject::connect( ui->openAct, &QAction::triggered, ui->bookmarks, &Bookmarks::readFiles );
    QObject::connect( ui->insertAct, &QAction::triggered, ui->bookmarks, &Bookmarks::readFiles );
    QObject::connect( ui->replaceAct, &QAction::triggered, ui->bookmarks, &Bookmarks::readFiles );
    QObject::connect( ui->saveFilesAct, &QAction::triggered, ui->bookmarks, &Bookmarks::saveFiles );
    QObject::connect( ui->saveAsAct, &QAction::triggered, ui->bookmarks, &Bookmarks::saveAsFiles );
    QObject::connect( ui->createTIFFAct, &QAction::triggered, ui->bookmarks, &Bookmarks::createTIFF );
    QObject::connect( ui->exitAct, &QAction::triggered, this, &MainWindow::close );

    // Edit menu
    QObject::connect( ui->selectAllAct, &QAction::triggered, ui->bookmarks, &Bookmarks::selectAll );
    QObject::connect( ui->selectEvenAct, &QAction::triggered, ui->bookmarks, &Bookmarks::selectEven );
    QObject::connect( ui->selectOddAct, &QAction::triggered, ui->bookmarks, &Bookmarks::selectOdd );
    QObject::connect( ui->deleteAct, &QAction::triggered, ui->bookmarks, &Bookmarks::deleteSelection );
    QObject::connect( ui->rotateCWAct, &QAction::triggered, ui->bookmarks, &Bookmarks::rotateSelection );
    QObject::connect( ui->rotateCCWAct, &QAction::triggered, ui->bookmarks, &Bookmarks::rotateSelection );
    QObject::connect( ui->rotate180Act, &QAction::triggered, ui->bookmarks, &Bookmarks::rotateSelection );

    // View menu
    QObject::connect( ui->zoomInAct, &QAction::triggered, ui->viewer, &Viewer::zoomIn );
    QObject::connect( ui->zoomOutAct, &QAction::triggered, ui->viewer, &Viewer::zoomOut );
    QObject::connect( ui->fitToWindowAct, &QAction::triggered, ui->viewer, &Viewer::fitToWindow );
    QObject::connect( ui->fillWindowAct, &QAction::triggered, ui->viewer, &Viewer::fillWindow );
    QObject::connect( ui->fitWidthAct, &QAction::triggered, ui->viewer, &Viewer::fitWidth );
    QObject::connect( ui->fitHeightAct, &QAction::triggered, ui->viewer, &Viewer::fitHeight );

    // Interconnects
    QObject::connect( ui->bookmarks, &Bookmarks::progressSig, this, &MainWindow::updateProgress );
    QObject::connect( ui->bookmarks, &QListWidget::currentItemChanged, ui->viewer, &Viewer::imageSelected );
    QObject::connect( ui->viewer, &Viewer::zoomSig, this, &MainWindow::updateActions );
}

// TODO
void MainWindow::colorMagic()
{
}

//
// Populate the toolbar with buttons
//
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
    ui->toolBar->addAction(ui->fitToWindowAct);

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

    // Adjust text for better toolbar layout
    ui->rotateCWAct->setText(QApplication::translate("MainWindow", "&Rotate\nCW", nullptr));
    ui->rotateCCWAct->setText(QApplication::translate("MainWindow", "Rotate\nCCW", nullptr));
    ui->rotate180Act->setText(QApplication::translate("MainWindow", "Rotate\n180", nullptr));
    ui->zoomInAct->setText(QApplication::translate("MainWindow", "Zoom\n&In", nullptr));
    ui->zoomOutAct->setText(QApplication::translate("MainWindow", "Zoom\n&Out", nullptr));
    ui->fitToWindowAct->setText(QApplication::translate("MainWindow", "&Fit to\nWindow", nullptr));
}

//
// Show progress of long operation on status bar
//
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

//
// Enable zoom icons if scale factor in range
//
void MainWindow::updateActions()
{
    ui->zoomInAct->setEnabled(ui->viewer->scaleFactor < 10.0);
    ui->zoomOutAct->setEnabled(ui->viewer->scaleFactor > 0.1);
}

// TODO
void MainWindow::about()
{
}

