#include "mainwindow.h"
#include "ui_mainWin.h"
#include "Config.h"
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
    Config::LoadConfig();
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
    QObject::connect( ui->blankAct, &QAction::triggered, ui->bookmarks, &Bookmarks::blankPage );
    QObject::connect( ui->fontAct, &QAction::triggered, this, &MainWindow::fontSelect );
    QObject::connect( ui->rotateCWAct, &QAction::triggered, ui->bookmarks, &Bookmarks::rotateCW );
    QObject::connect( ui->rotateCCWAct, &QAction::triggered, ui->bookmarks, &Bookmarks::rotateCCW );
    QObject::connect( ui->rotate180Act, &QAction::triggered, ui->bookmarks, &Bookmarks::rotate180 );
    QObject::connect( ui->mirrorHorizAct, &QAction::triggered, ui->bookmarks, &Bookmarks::mirrorHoriz );
    QObject::connect( ui->mirrorVertAct, &QAction::triggered, ui->bookmarks, &Bookmarks::mirrorVert );

    // View menu
    QObject::connect( ui->zoomInAct, &QAction::triggered, ui->viewer, &Viewer::zoomIn );
    QObject::connect( ui->zoomOutAct, &QAction::triggered, ui->viewer, &Viewer::zoomOut );
    QObject::connect( ui->fitWindowAct, &QAction::triggered, ui->viewer, &Viewer::fitWindow );
    QObject::connect( ui->fillWindowAct, &QAction::triggered, ui->viewer, &Viewer::fillWindow );
    QObject::connect( ui->fitWidthAct, &QAction::triggered, ui->viewer, &Viewer::fitWidth );
    QObject::connect( ui->fitHeightAct, &QAction::triggered, ui->viewer, &Viewer::fitHeight );

    // Undo menu
    QObject::connect( ui->undoAct, &QAction::triggered, ui->bookmarks, &Bookmarks::undoEdit );
    QObject::connect( ui->redoAct, &QAction::triggered, ui->bookmarks, &Bookmarks::redoEdit );

    // Color button
    QObject::connect( ui->colorAct, &QAction::triggered, this, &MainWindow::colorMagic );

    // Pencil menu
    QObject::connect( ui->pointerAct, &QAction::triggered, [this]() { this->ui->viewer->setTool(Viewer::Select); });
    QObject::connect( ui->pencilAct, &QAction::triggered, [this]() { this->ui->viewer->setTool(Viewer::Pencil); });
    QObject::connect( ui->eraserAct, &QAction::triggered, [this]() { this->ui->viewer->setTool(Viewer::Eraser); });

    // Stroke menu
    QObject::connect( ui->pix1Act, &QAction::triggered, [this]() { Config::brushSize = 1; });
    QObject::connect( ui->pix4Act, &QAction::triggered, [this]() { Config::brushSize = 4; });
    QObject::connect( ui->pix8Act, &QAction::triggered, [this]() { Config::brushSize = 8; });
    QObject::connect( ui->pix12Act, &QAction::triggered, [this]() { Config::brushSize = 12; });

    // Dropper menu
    QObject::connect( ui->dropperAct, &QAction::triggered, [this]() { this->ui->viewer->setTool(Viewer::ColorSelect); });
    QObject::connect( ui->dropperAct, &QAction::triggered, [this]() { this->makeDropperVisible(1); });
    QObject::connect( dropperThresholdWidget->spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int val){ Config::dropperThreshold = val; this->ui->viewer->doDropper(); });

    QObject::connect( ui->floodAct, &QAction::triggered, [this]() { this->ui->viewer->setTool(Viewer::FloodFill); });
    QObject::connect( ui->floodAct, &QAction::triggered, [this]() { this->makeDropperVisible(2); });
    QObject::connect( floodThresholdWidget->spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int val){ Config::floodThreshold = val; this->ui->viewer->doFlood(); });

    QObject::connect( ui->removeAct, &QAction::triggered, [this]() { this->ui->viewer->setTool(Viewer::RemoveBG); });
    QObject::connect( ui->removeAct, &QAction::triggered, [this]() { this->makeDropperVisible(4); });
    QObject::connect( ui->removeAct, &QAction::triggered, ui->bookmarks, &Bookmarks::removeBG );
    QObject::connect( bgRemoveThresholdWidget->spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int val){ Config::bgRemoveThreshold = val; this->ui->viewer->doRemoveBG(); });

    // Despeckle menu
    QObject::connect( ui->despeckleAct, &QAction::triggered, [this]() { this->ui->viewer->setTool(Viewer::Despeckle); });
    QObject::connect( ui->despeckleAct, &QAction::triggered, [this]() { this->makeDropperVisible(1); });
    QObject::connect( ui->despeckleAct, &QAction::triggered, ui->bookmarks, &Bookmarks::despeckle );
    QObject::connect( despeckleWidget->spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int val){ Config::despeckleArea = val; this->ui->viewer->doDespeckle(); });

    QObject::connect( ui->devoidAct, &QAction::triggered, [this]() { this->ui->viewer->setTool(Viewer::Devoid); });
    QObject::connect( ui->devoidAct, &QAction::triggered, [this]() { this->makeDropperVisible(2); });
    QObject::connect( ui->devoidAct, &QAction::triggered, ui->bookmarks, &Bookmarks::devoid );
    QObject::connect( devoidWidget->spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int val){ Config::devoidArea = val; this->ui->viewer->doDevoid(); });

    // Format conversion menu
    QObject::connect( ui->grayscaleAct, &QAction::triggered, ui->viewer, &Viewer::toGrayscale );
    QObject::connect( ui->grayscaleAct, &QAction::triggered, ui->bookmarks, &Bookmarks::toGrayscale );

    QObject::connect( ui->binaryAct, &QAction::triggered, ui->viewer, &Viewer::toBinary );
    QObject::connect( ui->binaryAct, &QAction::triggered, ui->bookmarks, &Bookmarks::toBinary );
    QObject::connect( blurWidget->spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int val){ Config::blurRadius = val; this->ui->viewer->toBinary(); });

    QObject::connect( ui->adaptiveBinaryAct, &QAction::triggered, ui->viewer, &Viewer::toAdaptive );
    QObject::connect( ui->adaptiveBinaryAct, &QAction::triggered, ui->bookmarks, &Bookmarks::toAdaptive );
    QObject::connect( blurWidget->spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int val){ Config::blurRadius = val; this->ui->viewer->toAdaptive(); });
    QObject::connect( kernelWidget->spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int val){ Config::kernelSize = val; this->ui->viewer->toAdaptive(); });

    QObject::connect( ui->ditheredBinaryAct, &QAction::triggered, ui->viewer, &Viewer::toDithered );
    QObject::connect( ui->ditheredBinaryAct, &QAction::triggered, ui->bookmarks, &Bookmarks::toDithered );

    // Deskew menu
    QObject::connect( ui->deskewAct, &QAction::triggered, [this]() { this->ui->viewer->setTool(Viewer::Deskew); });
    QObject::connect( ui->deskewAct, &QAction::triggered, ui->bookmarks, &Bookmarks::deskew );
    QObject::connect( deskewWidget->spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double val){ Config::deskewAngle = val; this->ui->viewer->doDeskew(); });

    // Help menu
    QObject::connect( ui->aboutAct, &QAction::triggered, this, &MainWindow::about );
    QObject::connect( ui->aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);

    // Interconnects
    QObject::connect( ui->viewer->blinkTimer, &QTimer::timeout, ui->viewer, &Viewer::blinker );
    QObject::connect( ui->bookmarks, &QListWidget::itemSelectionChanged, ui->bookmarks, &Bookmarks::itemSelectionChanged );
    QObject::connect( ui->bookmarks, &Bookmarks::changePageSig, ui->viewer, &Viewer::changePage );
    QObject::connect( ui->bookmarks, &Bookmarks::updatePageSig, ui->viewer, &Viewer::updatePage );
    QObject::connect( ui->viewer, &Viewer::updateIconSig, ui->bookmarks, &Bookmarks::updateIcon );

    QObject::connect( ui->bookmarks, &Bookmarks::progressSig, this, &MainWindow::updateProgress );
    QObject::connect( ui->viewer, &Viewer::zoomSig, zoomToolButton, &QToolButton::click );
    QObject::connect( ui->viewer, &Viewer::setDeskewSig, deskewWidget->spinBox, &QDoubleSpinBox::setValue);
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
    ui->blankAct->setText(QApplication::translate("MainWindow", "&Blank\nPage", nullptr));

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
    ui->rotateCWAct->setText(QApplication::translate("MainWindow", "&Rotate\nCW", nullptr));
    ui->rotateCCWAct->setText(QApplication::translate("MainWindow", "Rotate\nCCW", nullptr));
    ui->rotate180Act->setText(QApplication::translate("MainWindow", "Rotate\n180", nullptr));
    ui->mirrorHorizAct->setText(QApplication::translate("MainWindow", "Horizontal\nMirror", nullptr));
    ui->mirrorVertAct->setText(QApplication::translate("MainWindow", "Vertical\nMirror", nullptr));

    // Undo button
    QMenu *undoMenu = new QMenu();
    undoMenu->addAction(ui->undoAct);
    undoMenu->addAction(ui->redoAct);
    PopupQToolButton *undoToolButton = new PopupQToolButton();
    undoToolButton->setMenu(undoMenu);
    undoToolButton->setDefaultAction(ui->undoAct);
    ui->toolBar->addWidget(undoToolButton);

    // Zooms actions
    QMenu *zoomMenu = new QMenu();
    zoomMenu->addAction( ui->zoomInAct );
    zoomMenu->addAction( ui->zoomOutAct );
    zoomMenu->addAction( ui->fitWindowAct );
    zoomMenu->addAction( ui->fillWindowAct );
    zoomMenu->addAction( ui->fitWidthAct );
    zoomMenu->addAction( ui->fitHeightAct );
    zoomToolButton = new PopupQToolButton();
    zoomToolButton->setMenu(zoomMenu);
    zoomToolButton->setDefaultAction(ui->fitWindowAct);
    ui->toolBar->addWidget(zoomToolButton);
    ui->zoomInAct->setText(QApplication::translate("MainWindow", "Zoom\n&In", nullptr));
    ui->zoomOutAct->setText(QApplication::translate("MainWindow", "Zoom\n&Out", nullptr));
    ui->fitWindowAct->setText(QApplication::translate("MainWindow", "&Fit\nWindow", nullptr));
    ui->fillWindowAct->setText(QApplication::translate("MainWindow", "&Fill\nWindow", nullptr));
    ui->fitWidthAct->setText(QApplication::translate("MainWindow", "&Fit\nWidth", nullptr));
    ui->fitHeightAct->setText(QApplication::translate("MainWindow", "&Fit\nHeight", nullptr));

    // Color button
    colorToolButton.setDefaultAction(ui->colorAct);
    colorToolButton.setIcon(Config::fgColor, Config::bgColor);
    ui->toolBar->addWidget(&colorToolButton);

    // Tools
    ui->toolBar->addAction(ui->pointerAct);

    // Pencil button
    QMenu *pencilMenu = new QMenu();
    pencilMenu->addAction(ui->pencilAct);
    pencilMenu->addAction(ui->eraserAct);
    PopupQToolButton *pencilToolButton = new PopupQToolButton();
    pencilToolButton->setMenu(pencilMenu);
    pencilToolButton->setDefaultAction(ui->pencilAct);
    ui->toolBar->addWidget(pencilToolButton);

    // Line button
    QMenu *toolSizeMenu = new QMenu();
    toolSizeMenu->addAction(ui->pix1Act);
    toolSizeMenu->addAction(ui->pix4Act);
    toolSizeMenu->addAction(ui->pix8Act);
    toolSizeMenu->addAction(ui->pix12Act);
    PopupQToolButton *toolSizeToolButton = new PopupQToolButton();
    toolSizeToolButton->setMenu(toolSizeMenu);
    if (Config::brushSize == 12)
        toolSizeToolButton->setDefaultAction(ui->pix12Act);
    else if (Config::brushSize == 8)
        toolSizeToolButton->setDefaultAction(ui->pix8Act);
    else if (Config::brushSize == 4)
        toolSizeToolButton->setDefaultAction(ui->pix4Act);
    else
        toolSizeToolButton->setDefaultAction(ui->pix1Act);
    ui->toolBar->addWidget(toolSizeToolButton);

    // Dropper button
    QMenu *dropperMenu = new QMenu();
    dropperMenu->addAction(ui->removeAct);
    dropperMenu->addAction(ui->dropperAct);
    dropperMenu->addAction(ui->floodAct);
    PopupQToolButton *dropperToolButton = new PopupQToolButton();
    dropperToolButton->setMenu(dropperMenu);
    dropperToolButton->setDefaultAction(ui->removeAct);
    ui->toolBar->addWidget(dropperToolButton);

    // Dropper button threshold widgets
    bgRemoveThresholdWidget = new SpinWidget(0, 255, Config::bgRemoveThreshold, 5, "BG Thresh", ui->toolBar);
    bgRemoveThresholdSpin = ui->toolBar->addWidget(bgRemoveThresholdWidget);
    bgRemoveThresholdSpin->setVisible(true);
    dropperThresholdWidget = new SpinWidget(0, 255, Config::dropperThreshold, 5, "Dropper", ui->toolBar);
    dropperThresholdSpin = ui->toolBar->addWidget(dropperThresholdWidget);
    dropperThresholdSpin->setVisible(false);
    floodThresholdWidget = new SpinWidget(0, 255, Config::floodThreshold, 5, "Flood", ui->toolBar);
    floodThresholdSpin = ui->toolBar->addWidget(floodThresholdWidget);
    floodThresholdSpin->setVisible(false);

    // Despeckle button
    QMenu *despeckleMenu = new QMenu();
    despeckleMenu->addAction(ui->despeckleAct);
    despeckleMenu->addAction(ui->devoidAct);
    PopupQToolButton *despeckleToolButton = new PopupQToolButton();
    despeckleToolButton->setMenu(despeckleMenu);
    despeckleToolButton->setDefaultAction(ui->despeckleAct);
    ui->toolBar->addWidget(despeckleToolButton);

    // Despeckle button size widgets
    despeckleWidget = new SpinWidget(1, 100, Config::despeckleArea, 5, "Blob Size", ui->toolBar);
    despeckleSpin = ui->toolBar->addWidget(despeckleWidget);
    despeckleSpin->setVisible(true);
    devoidWidget = new SpinWidget(1, 100, Config::devoidArea, 5, "Void Size", ui->toolBar);
    devoidSpin = ui->toolBar->addWidget(devoidWidget);
    devoidSpin->setVisible(false);

    // Deskew button and widget
    ui->toolBar->addAction(ui->deskewAct);
    deskewWidget = new DoubleSpinWidget(-45.0, 45.0, Config::deskewAngle, 0.05, "Skew", ui->toolBar);
    ui->toolBar->addWidget(deskewWidget);

    // Format conversion button and widgets
    QMenu *reFormatMenu = new QMenu();
    reFormatMenu->addAction(ui->binaryAct);
    reFormatMenu->addAction(ui->adaptiveBinaryAct);
    reFormatMenu->addAction(ui->ditheredBinaryAct);
    reFormatMenu->addAction(ui->grayscaleAct);
    PopupQToolButton *reFormatToolButton = new PopupQToolButton();
    reFormatToolButton->setMenu(reFormatMenu);
    reFormatToolButton->setDefaultAction(ui->binaryAct);
    ui->toolBar->addWidget(reFormatToolButton);
    blurWidget = new OddSpinWidget(1, 31, Config::blurRadius, 2, "Blur Size", ui->toolBar);
    blurSpin = ui->toolBar->addWidget(blurWidget);
    kernelWidget = new OddSpinWidget(3, 149, Config::kernelSize, 2, "Kernel Size", ui->toolBar);
    kernelSpin = ui->toolBar->addWidget(kernelWidget);
    ui->adaptiveBinaryAct->setText(QApplication::translate("MainWindow", "Adaptive\nBinary", nullptr));
    ui->ditheredBinaryAct->setText(QApplication::translate("MainWindow", "Dithered\nBinary", nullptr));

    // Right justify remaining icons
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->toolBar->addWidget(spacer);

    // Help
    QMenu *helpMenu = new QMenu();
    helpMenu->addAction(ui->aboutAct);
    helpMenu->addAction(ui->aboutQtAct);
    PopupQToolButton *helpToolButton = new PopupQToolButton();
    helpToolButton->setMenu(helpMenu);
    helpToolButton->setDefaultAction(ui->aboutAct);
    ui->toolBar->addWidget(helpToolButton);
}

//
// Set one of the dropper spinboxes visible
//
void MainWindow::makeDropperVisible(int mask)
{
    dropperThresholdSpin->setVisible((mask & 1) != 0);
    floodThresholdSpin->setVisible((mask & 2) != 0);
    bgRemoveThresholdSpin->setVisible((mask & 4) != 0);
}

//
// Set one of the despeckle spinboxes visible
//
void MainWindow::makeDespeckleVisible(int mask)
{
    despeckleSpin->setVisible((mask & 1) != 0);
    devoidSpin->setVisible((mask & 2) != 0);
}

//
// Manipulate drawing colors
//
void MainWindow::colorMagic()
{
    if (colorToolButton.mode == "Foreground")
    {
        QColor tmp = QColorDialog::getColor(Config::fgColor, nullptr, "Foreground");
        if (tmp.isValid())
            Config::fgColor = tmp;
    }
    else if (colorToolButton.mode == "Background")
    {
        QColor tmp = QColorDialog::getColor(Config::bgColor, nullptr, "Background");
        if (tmp.isValid())
            Config::bgColor = tmp;
    }
    else if (colorToolButton.mode == "Swap")
    {
        QColor tmp;
        tmp = Config::fgColor;
        Config::fgColor = Config::bgColor;
        Config::bgColor = tmp;
    }
    else if (colorToolButton.mode == "Reset")
    {
        Config::fgColor = Qt::black;
        Config::bgColor = Qt::white;
    }
    colorToolButton.setIcon(Config::fgColor, Config::bgColor);
}

//
// Set font for text insertion
//
void MainWindow::fontSelect()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, Config::textFont, this);
    if (ok)
        Config::textFont = font;
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
