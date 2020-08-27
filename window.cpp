#include "glwidget.h"
#include "window.h"
#include "mainwindow.h"
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QDesktopWidget>
#include <QApplication>
#include <QMessageBox>
#include <QGroupBox>
#include <QSpacerItem>
#include <QLCDNumber>


Window::Window(MainWindow* mw)
    : mainWindow(mw)
{
    glWidget = new GLWidget;

    xSlider = createSlider();
    ySlider = createSlider();
    zSlider = createSlider();

    QGroupBox* groupBoxTools = new QGroupBox(tr("Tools"));
    groupBoxTools->setFixedSize(250, 100);
    startRecordingButton = new QPushButton("Start Recording");
    stopRecordingButton = new QPushButton("Stop Recording");

    QGroupBox* groupBoxDebug = new QGroupBox(tr("Debug"));
    groupBoxDebug->setFixedSize(250, 50);
    debugImageCombo = new QComboBox;
    debugImageCombo->addItem("Debug Off");
    debugImageCombo->addItem("Snow Falling");
    debugImageCombo->addItem("Snow Ground");
    debugImageCombo->addItem("Underside height map");
    debugImageCombo->addItem("Noise image");
    debugImageCombo->addItem("Deformation map 1");
    debugImageCombo->addItem("Deformation map 2");

    connect(startRecordingButton, &QPushButton::pressed, glWidget, &GLWidget::start_recording);
    connect(stopRecordingButton, &QPushButton::pressed, glWidget, &GLWidget::stop_recording);

    connect(debugImageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), glWidget, &GLWidget::change_debug_image);

    connect(xSlider, &QSlider::valueChanged, glWidget, &GLWidget::set_snow_amount);
    connect(ySlider, &QSlider::valueChanged, glWidget, &GLWidget::set_blizzard_amount);
    connect(zSlider, &QSlider::valueChanged, glWidget, &GLWidget::set_turb_amount);
    QGroupBox* groupBoxSnow = new QGroupBox(tr("Falling Snow Controls"));
    groupBoxSnow->setFixedSize(250, 100);
    QGridLayout* mainLayout = new QGridLayout;
    QVBoxLayout* snowFallingContainer = new QVBoxLayout;
    QVBoxLayout* toolsContainer = new QVBoxLayout;
    QHBoxLayout* GLContainer = new QHBoxLayout;
    QVBoxLayout* debugContainer = new QVBoxLayout;
    glWidget->setFixedSize(1000, 800);
    GLContainer->addWidget(glWidget);
    ySlider->setRange(0, 100);
    zSlider->setRange(0, 100);
    snowFallingContainer->addWidget(xSlider);
    snowFallingContainer->addWidget(ySlider);
    snowFallingContainer->addWidget(zSlider);
    QSpacerItem* spacer = new QSpacerItem(1,1);

    toolsContainer->addWidget(startRecordingButton);
    toolsContainer->addWidget(stopRecordingButton);
    groupBoxTools->setLayout(toolsContainer);

    debugContainer->addWidget(debugImageCombo);
    groupBoxDebug->setLayout(debugContainer);

    QVBoxLayout* fpsContainer = new QVBoxLayout;
    QGroupBox* groupBoxFPS = new QGroupBox(tr("FPS"));
    groupBoxFPS->setFixedSize(250, 100);
    QLCDNumber* fps = new QLCDNumber(4);
    fps->setSegmentStyle(QLCDNumber::Flat);
    QObject::connect(glWidget, SIGNAL(fpsChanged(int)), fps, SLOT(display(int)));
    fpsContainer->addWidget(fps);
    groupBoxFPS->setLayout(fpsContainer);
    
    QWidget* w = new QWidget;
    groupBoxSnow->setLayout(snowFallingContainer);
    w->setLayout(GLContainer);
    mainLayout->addWidget(w,0,0,5,1);
    mainLayout->addWidget(groupBoxSnow,0,1);
    mainLayout->addWidget(groupBoxFPS, 1, 1);
    mainLayout->addWidget(groupBoxTools, 2, 1);
    mainLayout->addWidget(groupBoxDebug, 3, 1);
    //TODO add tools
    mainLayout->addItem(spacer, 4, 1);

    setLayout(mainLayout);

    xSlider->setValue(15 * 16);
    ySlider->setValue(345 * 16);
    zSlider->setValue(0 * 16);

    setWindowTitle(tr("Hello GL"));
}

QSlider* Window::createSlider()
{
    QSlider* slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 360 * 16);
    slider->setSingleStep(16);
    slider->setPageStep(15 * 16);
    slider->setTickInterval(15 * 16);
    slider->setTickPosition(QSlider::TicksRight);
    return slider;
}

void Window::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape)
        close();
    else
        QWidget::keyPressEvent(e);
}