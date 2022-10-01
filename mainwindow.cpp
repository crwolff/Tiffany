#include "mainwindow.h"
#include "ui_mainWin.h"
#include "PopupQToolButton.h"
#include <QColorDialog>
#include <QDebug>
#include <QMessageBox>

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
    QObject::connect( ui->blankAct, &QAction::triggered, ui->viewer, &Viewer::blankPage );
    QObject::connect( ui->rotateCWAct, &QAction::triggered, ui->bookmarks, &Bookmarks::rotateCW );
    QObject::connect( ui->rotateCCWAct, &QAction::triggered, ui->bookmarks, &Bookmarks::rotateCCW );
    QObject::connect( ui->rotate180Act, &QAction::triggered, ui->bookmarks, &Bookmarks::rotate180 );

    // View menu
    QObject::connect( ui->zoomInAct, &QAction::triggered, ui->viewer, &Viewer::zoomIn );
    QObject::connect( ui->zoomOutAct, &QAction::triggered, ui->viewer, &Viewer::zoomOut );
    QObject::connect( ui->fitToWindowAct, &QAction::triggered, ui->viewer, &Viewer::fitToWindow );
    QObject::connect( ui->fillWindowAct, &QAction::triggered, ui->viewer, &Viewer::fillWindow );
    QObject::connect( ui->fitWidthAct, &QAction::triggered, ui->viewer, &Viewer::fitWidth );
    QObject::connect( ui->fitHeightAct, &QAction::triggered, ui->viewer, &Viewer::fitHeight );

    // Tools menu
    QObject::connect( ui->pointerAct, &QAction::triggered, ui->viewer, &Viewer::pointerMode );
    QObject::connect( ui->dropperAct, &QAction::triggered, ui->viewer, &Viewer::dropperMode );
    QObject::connect( ui->pencilAct, &QAction::triggered, ui->viewer, &Viewer::pencilMode );

    // Stroke menu
    QObject::connect( ui->pix1Act, &QAction::triggered, ui->viewer, &Viewer::setBrush_1 );
    QObject::connect( ui->pix4Act, &QAction::triggered, ui->viewer, &Viewer::setBrush_4 );
    QObject::connect( ui->pix8Act, &QAction::triggered, ui->viewer, &Viewer::setBrush_8 );
    QObject::connect( ui->pix12Act, &QAction::triggered, ui->viewer, &Viewer::setBrush_12 );

    // Toolbar
    QObject::connect( ui->undoAct, &QAction::triggered, ui->viewer, &Viewer::undoEdit );
    QObject::connect( ui->redoAct, &QAction::triggered, ui->viewer, &Viewer::redoEdit );
    QObject::connect( ui->colorAct, &QAction::triggered, this, &MainWindow::colorMagic );
    QObject::connect( ui->grayscaleAct, &QAction::triggered, ui->bookmarks, &Bookmarks::toGrayscale );
    QObject::connect( ui->binaryAct, &QAction::triggered, ui->bookmarks, &Bookmarks::toBinary );

    // Help menu
    QObject::connect( ui->aboutAct, &QAction::triggered, this, &MainWindow::about );
    QObject::connect( ui->aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);

    // Interconnects
    QObject::connect( ui->bookmarks, &Bookmarks::progressSig, this, &MainWindow::updateProgress );
    QObject::connect( ui->bookmarks, &QListWidget::currentItemChanged, ui->viewer, &Viewer::imageSelected );
    QObject::connect( ui->viewer, &Viewer::zoomSig, this, &MainWindow::updateActions );
    QObject::connect( ui->viewer, &Viewer::imageChangedSig, ui->bookmarks, &Bookmarks::updateIcon );
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
    ui->toolBar->addAction(ui->blankAct);

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
    toolsMenu->addAction(ui->dropperAct);
    toolsMenu->addAction(ui->pencilAct);
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
    colorToolButton.setDefaultAction(ui->colorAct);
    colorToolButton.setIcon(ui->viewer->foregroundColor, ui->viewer->backgroundColor);
    ui->toolBar->addWidget(&colorToolButton);

    // Format conversion button
    QMenu *reFormatMenu = new QMenu();
    reFormatMenu->addAction(ui->binaryAct);
    reFormatMenu->addAction(ui->grayscaleAct);
    PopupQToolButton *reFormatToolButton = new PopupQToolButton();
    reFormatToolButton->setMenu(reFormatMenu);
    reFormatToolButton->setDefaultAction(ui->grayscaleAct);
    ui->toolBar->addWidget(reFormatToolButton);

    // Adjust text for better toolbar layout
    ui->blankAct->setText(QApplication::translate("MainWindow", "&Blank\nPage", nullptr));
    ui->rotateCWAct->setText(QApplication::translate("MainWindow", "&Rotate\nCW", nullptr));
    ui->rotateCCWAct->setText(QApplication::translate("MainWindow", "Rotate\nCCW", nullptr));
    ui->rotate180Act->setText(QApplication::translate("MainWindow", "Rotate\n180", nullptr));
    ui->zoomInAct->setText(QApplication::translate("MainWindow", "Zoom\n&In", nullptr));
    ui->zoomOutAct->setText(QApplication::translate("MainWindow", "Zoom\n&Out", nullptr));
    ui->fitToWindowAct->setText(QApplication::translate("MainWindow", "&Fit to\nWindow", nullptr));
}

//
// Manipulate drawing colors
//
void MainWindow::colorMagic()
{
    if (colorToolButton.mode == "Foreground")
        ui->viewer->foregroundColor = QColorDialog::getColor();
    else if (colorToolButton.mode == "Background")
        ui->viewer->backgroundColor = QColorDialog::getColor();
    else if (colorToolButton.mode == "Swap")
    {
        QColor tmp;
        tmp = ui->viewer->foregroundColor;
        ui->viewer->foregroundColor = ui->viewer->backgroundColor;
        ui->viewer->backgroundColor = tmp;
    }
    else if (colorToolButton.mode == "Reset")
    {
        ui->viewer->foregroundColor = Qt::black;
        ui->viewer->backgroundColor = Qt::white;
    }
    colorToolButton.setIcon(ui->viewer->foregroundColor, ui->viewer->backgroundColor);
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
// Enable zoom icons if scale factor in range
//
void MainWindow::updateActions(float scale)
{
    ui->zoomInAct->setEnabled(scale < 10.0);
    ui->zoomOutAct->setEnabled(scale > 0.1);
}

//
void MainWindow::about()
{
    QMessageBox::about( this, "About Tiffany",
            "<p><b>Tiffany</b> is an image editor tuned for cleaning"
            "up scanned images</p>" );
}

