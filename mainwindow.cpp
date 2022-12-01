#include "mainwindow.h"
#include "ui_mainWin.h"
#include "PopupQToolButton.h"
#include "DoubleSpinWidget.h"
#include "SpinWidget.h"
#include <QCloseEvent>
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
    QObject::connect( ui->mirrorHorizAct, &QAction::triggered, ui->bookmarks, &Bookmarks::mirrorHoriz );
    QObject::connect( ui->mirrorVertAct, &QAction::triggered, ui->bookmarks, &Bookmarks::mirrorVert );

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
    QObject::connect( ui->eraserAct, &QAction::triggered, ui->viewer, &Viewer::eraserMode );
    QObject::connect( ui->deskewAct, &QAction::triggered, ui->viewer, &Viewer::deskewMode );
    QObject::connect( ui->despeckleAct, &QAction::triggered, ui->viewer, &Viewer::despeckleMode );

    QObject::connect( ui->dropperAct, &QAction::triggered, [this]() { this->makeVisible(1); });
    QObject::connect( ui->deskewAct, &QAction::triggered, [this]() { this->makeVisible(2); });
    QObject::connect( ui->despeckleAct, &QAction::triggered, [this]() { this->makeVisible(4); });

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
    QObject::connect( ui->adaptiveBinaryAct, &QAction::triggered, ui->bookmarks, &Bookmarks::toAdaptive );
    QObject::connect( ui->binaryAct, &QAction::triggered, [this]() { this->makeVisible(8); });
    QObject::connect( ui->adaptiveBinaryAct, &QAction::triggered, [this]() { this->makeVisible(8); });

    // Help menu
    QObject::connect( ui->aboutAct, &QAction::triggered, this, &MainWindow::about );
    QObject::connect( ui->aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);

    // Interconnects
    QObject::connect( ui->bookmarks, &Bookmarks::progressSig, this, &MainWindow::updateProgress );
    QObject::connect( ui->bookmarks, &QListWidget::currentItemChanged, ui->viewer, &Viewer::imageSelected );
    QObject::connect( ui->bookmarks, &Bookmarks::updateViewerSig, ui->viewer, &Viewer::updateViewer );
    QObject::connect( ui->viewer, &Viewer::zoomSig, this, &MainWindow::updateActions );
    QObject::connect( ui->viewer, &Viewer::imageChangedSig, ui->bookmarks, &Bookmarks::updateIcon );
    QObject::connect( ui->viewer->blinkTimer, &QTimer::timeout, ui->viewer, &Viewer::blinker );
    QObject::connect( dropperThresholdWidget->spinBox, QOverload<int>::of(&QSpinBox::valueChanged), ui->viewer, &Viewer::setDropperThreshold);
    QObject::connect( deskewWidget->spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), ui->viewer, &Viewer::setDeskew);
    QObject::connect( despeckleWidget->spinBox, QOverload<int>::of(&QSpinBox::valueChanged), ui->viewer, &Viewer::setDespeckle);
    QObject::connect( blurWidget->spinBox, QOverload<int>::of(&QSpinBox::valueChanged), ui->bookmarks, &Bookmarks::setBlurRadius);
}

// Toggle visibility on the spin boxes
void MainWindow::makeVisible(int mask)
{
    dropperThresholdSpin->setVisible((mask & 1) != 0);
    deskewSpin->setVisible((mask & 2) != 0);
    despeckleSpin->setVisible((mask & 4) != 0);
    blurSpin->setVisible((mask & 8) != 0);
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
    rotateMenu->addAction(ui->mirrorHorizAct);
    rotateMenu->addAction(ui->mirrorVertAct);
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

    // Tools
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->pointerAct);

    // Pencil button
    QMenu *pencilMenu = new QMenu();
    pencilMenu->addAction(ui->pencilAct);
    pencilMenu->addAction(ui->eraserAct);
    PopupQToolButton *pencilToolButton = new PopupQToolButton();
    pencilToolButton->setMenu(pencilMenu);
    pencilToolButton->setDefaultAction(ui->pencilAct);
    ui->toolBar->addWidget(pencilToolButton);

    // Remaining tools
    ui->toolBar->addAction(ui->deskewAct);
    deskewWidget = new DoubleSpinWidget(-45.0, 45.0, ui->viewer->deskewAngle, 0.05, "Skew\nAngle", ui->toolBar);
    deskewSpin = ui->toolBar->addWidget(deskewWidget);
    deskewSpin->setVisible(false);
    ui->toolBar->addAction(ui->dropperAct);
    dropperThresholdWidget = new SpinWidget(0, 255, ui->viewer->dropperThreshold, 5, "Dropper\nThreshold", ui->toolBar);
    dropperThresholdSpin = ui->toolBar->addWidget(dropperThresholdWidget);
    dropperThresholdSpin->setVisible(false);
    ui->toolBar->addAction(ui->despeckleAct);
    despeckleWidget = new SpinWidget(1, 100, ui->viewer->despeckleArea, 1, "Maximum\nBlob Size", ui->toolBar);
    despeckleSpin = ui->toolBar->addWidget(despeckleWidget);
    despeckleSpin->setVisible(false);

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
    reFormatMenu->addAction(ui->adaptiveBinaryAct);
    reFormatMenu->addAction(ui->grayscaleAct);
    PopupQToolButton *reFormatToolButton = new PopupQToolButton();
    reFormatToolButton->setMenu(reFormatMenu);
    reFormatToolButton->setDefaultAction(ui->grayscaleAct);
    ui->toolBar->addWidget(reFormatToolButton);
    blurWidget = new SpinWidget(1, 9, ui->bookmarks->blurRadius, 2, "Blur Size", ui->toolBar);
    blurSpin = ui->toolBar->addWidget(blurWidget);
    blurSpin->setVisible(false);

    // Adjust text for better toolbar layout
    ui->blankAct->setText(QApplication::translate("MainWindow", "&Blank\nPage", nullptr));
    ui->mirrorHorizAct->setText(QApplication::translate("MainWindow", "Horizontal\nMirror", nullptr));
    ui->mirrorVertAct->setText(QApplication::translate("MainWindow", "Vertical\nMirror", nullptr));
    ui->rotateCWAct->setText(QApplication::translate("MainWindow", "&Rotate\nCW", nullptr));
    ui->rotateCCWAct->setText(QApplication::translate("MainWindow", "Rotate\nCCW", nullptr));
    ui->rotate180Act->setText(QApplication::translate("MainWindow", "Rotate\n180", nullptr));
    ui->zoomInAct->setText(QApplication::translate("MainWindow", "Zoom\n&In", nullptr));
    ui->zoomOutAct->setText(QApplication::translate("MainWindow", "Zoom\n&Out", nullptr));
    ui->fitToWindowAct->setText(QApplication::translate("MainWindow", "&Fit to\nWindow", nullptr));
    ui->adaptiveBinaryAct->setText(QApplication::translate("MainWindow", "Adaptive\nBinary", nullptr));
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
