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
    QObject::connect( ui->fitToWindowAct, &QAction::triggered, ui->viewer, &Viewer::fitToWindow );
    QObject::connect( ui->fillWindowAct, &QAction::triggered, ui->viewer, &Viewer::fillWindow );
    QObject::connect( ui->fitWidthAct, &QAction::triggered, ui->viewer, &Viewer::fitWidth );
    QObject::connect( ui->fitHeightAct, &QAction::triggered, ui->viewer, &Viewer::fitHeight );

    // Undo menu
    QObject::connect( ui->undoAct, &QAction::triggered, ui->bookmarks, &Bookmarks::undoEdit );
    QObject::connect( ui->redoAct, &QAction::triggered, ui->bookmarks, &Bookmarks::redoEdit );

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
    QObject::connect( ui->removeAct, &QAction::triggered, ui->bookmarks, &Bookmarks::removeBG );
    QObject::connect( ui->dropperAct, &QAction::triggered, [this]() { this->ui->viewer->setTool(Viewer::ColorSelect); });
    QObject::connect( dropperThresholdWidget->spinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            [this]() { Config::dropperThreshold = dropperThresholdWidget->spinBox->value(); });

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
    zoomMenu->addAction( ui->fitToWindowAct );
    zoomMenu->addAction( ui->fillWindowAct );
    zoomMenu->addAction( ui->fitWidthAct );
    zoomMenu->addAction( ui->fitHeightAct );
    zoomToolButton = new PopupQToolButton();
    zoomToolButton->setMenu(zoomMenu);
    zoomToolButton->setDefaultAction(ui->fitToWindowAct);
    ui->toolBar->addWidget(zoomToolButton);

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
    dropperMenu->addAction(ui->dropperAct);
    dropperMenu->addAction(ui->removeAct);
    PopupQToolButton *dropperToolButton = new PopupQToolButton();
    dropperToolButton->setMenu(dropperMenu);
    dropperToolButton->setDefaultAction(ui->dropperAct);
    ui->toolBar->addWidget(dropperToolButton);
    dropperThresholdWidget = new SpinWidget(0, 255, Config::dropperThreshold, 5, "Threshold", ui->toolBar);
    ui->toolBar->addWidget(dropperThresholdWidget);
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
