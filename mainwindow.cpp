#include "mainwindow.h"
#include "ui_mainWin.h"
#include <QCloseEvent>
#include <QColorDialog>
#include <QDebug>
#include <QFontDialog>
#include <QMessageBox>

//
// Constructor
//
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
//    Config::LoadConfig();
    ui->setupUi(this);
    buildToolBar();
    connectSignalSlots();

    // Initialize status bar
    statusLabel = new QLabel("Ready");
    ui->statusbar->addWidget(statusLabel);
    progressBar = new QProgressBar(this);
    progressBar->setMaximumHeight(17);
    progressBar->hide();
    ui->statusbar->addWidget(progressBar);
}

//
// Destructor
//
MainWindow::~MainWindow()
{
//    Config::SaveConfig();
    delete ui;
}

//
// Connect signals to slots
//
void MainWindow::connectSignalSlots()
{
    // File menu
    QObject::connect( ui->openAct, &QAction::triggered, ui->bookmarks, &Bookmarks::openFiles );
    QObject::connect( ui->insertAct, &QAction::triggered, ui->bookmarks, &Bookmarks::insertFiles );
    QObject::connect( ui->replaceAct, &QAction::triggered, ui->bookmarks, &Bookmarks::replaceFiles );
    QObject::connect( ui->saveFilesAct, &QAction::triggered, ui->bookmarks, &Bookmarks::saveFiles );
    QObject::connect( ui->saveToAct, &QAction::triggered, ui->bookmarks, &Bookmarks::saveToDir );
    QObject::connect( ui->exitAct, &QAction::triggered, this, &MainWindow::close );

    // Edit menu
    QObject::connect( ui->selectAllAct, &QAction::triggered, ui->bookmarks, &Bookmarks::selectAll );
    QObject::connect( ui->selectEvenAct, &QAction::triggered, ui->bookmarks, &Bookmarks::selectEven );
    QObject::connect( ui->selectOddAct, &QAction::triggered, ui->bookmarks, &Bookmarks::selectOdd );
    QObject::connect( ui->deleteAct, &QAction::triggered, ui->bookmarks, &Bookmarks::deleteSelection );

    // View menu
    QObject::connect( ui->zoomInAct, &QAction::triggered, ui->viewer, &Viewer::zoomIn );
    QObject::connect( ui->zoomOutAct, &QAction::triggered, ui->viewer, &Viewer::zoomOut );
    QObject::connect( ui->fitToWindowAct, &QAction::triggered, ui->viewer, &Viewer::fitToWindow );
    QObject::connect( ui->fillWindowAct, &QAction::triggered, ui->viewer, &Viewer::fillWindow );
    QObject::connect( ui->fitWidthAct, &QAction::triggered, ui->viewer, &Viewer::fitWidth );
    QObject::connect( ui->fitHeightAct, &QAction::triggered, ui->viewer, &Viewer::fitHeight );

    // Help menu
    QObject::connect( ui->aboutAct, &QAction::triggered, this, &MainWindow::about );
    QObject::connect( ui->aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);

    // Interconnects
    QObject::connect( ui->bookmarks, &QListWidget::currentItemChanged, ui->bookmarks, &Bookmarks::currItemChanged );
    QObject::connect( ui->bookmarks, &Bookmarks::changePage, ui->viewer, &Viewer::changePage );
    QObject::connect( ui->bookmarks, &Bookmarks::progressSig, this, &MainWindow::updateProgress );
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
    saveMenu->addAction(ui->saveToAct);
    PopupQToolButton *saveToolButton = new PopupQToolButton();
    saveToolButton->setMenu(saveMenu);
    saveToolButton->setDefaultAction(ui->saveFilesAct);
    ui->toolBar->addWidget(saveToolButton);

    // Delete actions
    ui->toolBar->addAction(ui->deleteAct);

    // Zooms actions
    ui->toolBar->addAction(ui->zoomOutAct);
    ui->toolBar->addAction(ui->zoomInAct);
    ui->toolBar->addAction(ui->fitToWindowAct);

    // Adjust text for better toolbar layout
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
        progressBar->hide();
        statusLabel->setText("Ready");
        QGuiApplication::restoreOverrideCursor();
    }
    else if (descr != "")
    {
        statusLabel->setText(descr);
        progressBar->setRange(0, val);
        progressBar->setValue(0);
        progressBar->show();
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    }
    else
    {
        progressBar->setValue(val);
    }
}

//
// Set status label to convey small informations
//
void MainWindow::setStatus(QString descr)
{
    if (descr != "")
        statusLabel->setText(descr);
    else
        statusLabel->setText("Ready");
}

//
void MainWindow::about()
{
    QMessageBox::about( this, "About Tiffany",
            "<p><b>Tiffany</b> is an image editor tuned for cleaning"
            "up scanned images</p>" );
}

//
// Check for modified files before exiting
//
void MainWindow::closeEvent (QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::Yes;
    if (ui->bookmarks->anyModified())
    {
        resBtn = QMessageBox::question( this, "Tiffany", tr("Modified files exist, are you sure?\n"),
                QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    }

    if (resBtn != QMessageBox::Yes)
        event->ignore();
    else
        event->accept();
}
