#include "martycam.hpp"
#include "renderwidget.hpp"
#include "settings.hpp"
//
#include <QTimer>
#include <QToolBar>
#include <QDockWidget>
#include <QSplitter>
#include <QSettings>
//
#include <hpx/lcos/future.hpp>
#include <hpx/include/async.hpp>

//
//----------------------------------------------------------------------------
const int MartyCam::IMAGE_BUFF_CAPACITY = 5;

MartyCam::MartyCam(const hpx::threads::executors::pool_executor& defaultExec,
                   const hpx::threads::executors::pool_executor& blockingExec) : QMainWindow(0)
{
  this->ui.setupUi(this);
  this->processingThread = nullptr;
  this->captureThread = nullptr;
  //
  this->defaultExecutor = defaultExec;
  this->blockingExecutor = blockingExec;
  //
  QString settingsFileName = QCoreApplication::applicationDirPath() + "/MartyCam.ini";
  QSettings settings(settingsFileName, QSettings::IniFormat);
  restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
  //
  this->renderWidget = new RenderWidget(this);
  this->ui.gridLayout->addWidget(this->renderWidget, 0, Qt::AlignHCenter || Qt::AlignTop);

  this->EventRecordCounter      = 0;
  this->insideMotionEvent       = 0;
  this->imageSize               = cv::Size(0,0);
  this->imageBuffer             = ImageBuffer(new ConcurrentCircularBuffer<cv::Mat>(IMAGE_BUFF_CAPACITY));
  this->cameraIndex             = 1;

  //
  // create a dock widget to hold the settings
  //
  settingsDock = new QDockWidget("Settings", this);
  settingsDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
  settingsDock->setObjectName("SettingsDock");
  //
  // create the settings widget itself
  //
  this->settingsWidget = new SettingsWidget(this);
  this->settingsWidget->setRenderWidget(this->renderWidget);
  settingsDock->setWidget(this->settingsWidget);
  settingsDock->setMinimumWidth(300);
  addDockWidget(Qt::RightDockWidgetArea, settingsDock);
  connect(this->settingsWidget, SIGNAL(resolutionSelected(cv::Size)), this, SLOT(onResolutionSelected(cv::Size)));
  connect(this->settingsWidget, SIGNAL(rotationChanged(int)), this, SLOT(onRotationChanged(int)));
  connect(this->settingsWidget, SIGNAL(CameraIndexChanged(int,QString)), this, SLOT(onCameraIndexChanged(int,QString)));
  connect(this->ui.actionQuit, SIGNAL(triggered()), this, SLOT(close()));
  //
  this->loadSettings();
  this->settingsWidget->loadSettings();
  //
  std::string camerastring;
  this->cameraIndex = this->settingsWidget->getCameraIndex(camerastring);
  //
  while (this->imageSize.width==0 && this->cameraIndex>=0) {

    cv::Size res = this->settingsWidget->getSelectedResolution();
    this->createCaptureThread(15, res, this->cameraIndex, camerastring, blockingExecutor);
    this->imageSize = this->captureThread->getImageSize();
    if (this->imageSize.width==0) {
      this->cameraIndex -=1;
      camerastring = "";
      this->deleteCaptureThread();
    }
  }
  if (this->imageSize.width>0) {
    this->renderWidget->setCVSize(this->imageSize);
    this->createProcessingThread(this->imageSize, nullptr, defaultExec,
    this->settingsWidget->getCurentProcessingType());
    //
    this->initChart();
  }
  else {
    //abort if no camera devices connected
  }
  //
  restoreState(settings.value("mainWindowState").toByteArray());
}
//----------------------------------------------------------------------------
void MartyCam::closeEvent(QCloseEvent*) {
  this->saveSettings();
  this->settingsWidget->saveSettings();
  this->deleteCaptureThread();
  this->deleteProcessingThread();
}
//----------------------------------------------------------------------------
void MartyCam::createCaptureThread(int FPS, cv::Size &size, int camera, const std::string &cameraname,
                                   hpx::threads::executors::pool_executor exec)
{
  this->captureThread = new CaptureThread(imageBuffer, size, camera, cameraname, this->blockingExecutor, this->settingsWidget->getRequestedFps());
  this->captureThread->setRotation(this->settingsWidget->getSelectedRotation());
  //TODO for now I need to enforce the resolution size here again but it is more of a hotfix than a good solution -> ultimately I want all this encapsulated in the Capture thread constructor.
  this->captureThread->setResolution(size);
  this->captureThread->startCapture();

  this->settingsWidget->setThreads(this->captureThread, this->processingThread);
}
//----------------------------------------------------------------------------
void MartyCam::deleteCaptureThread()
{
  this->captureThread->stopCapture();
  this->imageBuffer->clear();
  delete captureThread;
  this->captureThread = nullptr;
  // push an empty image to the image buffer to ensure that any waiting processing thread is freed
  this->imageBuffer->send(cv::Mat());
}
//----------------------------------------------------------------------------
void MartyCam::createProcessingThread(cv::Size &size, ProcessingThread *oldThread,
                                      hpx::threads::executors::pool_executor exec,
                                      ProcessingType processingType)
{
  MotionFilterParams mfp = this->settingsWidget->getMotionFilterParams();
  FaceRecogFilterParams frfp = this->settingsWidget->getFaceRecogFilterParams();
    this->processingThread =
            new ProcessingThread(imageBuffer, exec, processingType, mfp, frfp);
  if (oldThread) this->processingThread->CopySettings(oldThread);
  this->processingThread->setRootFilter(renderWidget);
  this->processingThread->startProcessing();

  this->settingsWidget->setThreads(this->captureThread, this->processingThread);
  //
  //TODO undestand what this signals were doing here and write code that does the same in another way
  connect(this->processingThread, SIGNAL(NewData()),
    this, SLOT(updateGUI()),Qt::QueuedConnection);
}
//----------------------------------------------------------------------------
void MartyCam::deleteProcessingThread()
{
  this->processingThread->stopProcessing();
  delete processingThread;
  this->processingThread = nullptr;
}
//----------------------------------------------------------------------------
void MartyCam::onCameraIndexChanged(int index, QString URL)
{
  if (this->cameraIndex==index) {
    return;
  }
  //
  this->cameraIndex = index;
  this->captureThread->connectCamera(index, URL.toStdString());
  this->imageBuffer->clear();
  this->clearGraphs();
}
//----------------------------------------------------------------------------
void MartyCam::onResolutionSelected(cv::Size newSize)
{
  if (newSize==this->imageSize) {
    return;
  }

  this->imageSize = newSize;
  this->captureThread->setResolution(newSize);

  // Update GUI renderwidget size
  this->renderWidget->setCVSize(newSize);
}
//----------------------------------------------------------------------------
void MartyCam::onRotationChanged(int rotation)
{
  this->captureThread->setRotation(rotation);
  //
  if (rotation==0 || rotation==3) {
    this->renderWidget->setCVSize(this->captureThread->getImageSize());
  }
  if (rotation==1 || rotation==2) {
    this->renderWidget->setCVSize(this->captureThread->getRotatedSize());
  }
  this->clearGraphs();
}
//----------------------------------------------------------------------------
void MartyCam::updateGUI() {
  //
  if (!this->processingThread) return;

  statusBar()->showMessage(QString("FPS : %1 | Frame Counter : %2 "
                                   "| Image Buffer Occupancy : %3\% "
                                   "| Sleep in CaptureThread: %4ms "
                                   "| Capture Time: %5ms "
                                   "| Processing Time: %6ms").
    arg(this->captureThread->getFPS(), 5, 'f', 2).
    arg(captureThread->GetFrameCounter(), 5).
    arg(100 * (float)this->imageBuffer->size()/IMAGE_BUFF_CAPACITY, 4).
    arg(captureThread->getSleepTime()).
    arg(captureThread->getCaptureTime()).
    arg(processingThread->getProcessingTime()));
}
//----------------------------------------------------------------------------
void MartyCam::clearGraphs()
{
}
//----------------------------------------------------------------------------
void MartyCam::onUserTrackChanged(int value)
{
  double percent = value;
  this->processingThread->motionFilter->triggerLevel = percent;
}
//----------------------------------------------------------------------------
void MartyCam::resetChart()
{
}
//----------------------------------------------------------------------------
void MartyCam::initChart()
{
}
//----------------------------------------------------------------------------
void MartyCam::saveSettings()
{
  QString settingsFileName = QCoreApplication::applicationDirPath() + "/MartyCam.ini";
  QSettings settings(settingsFileName, QSettings::IniFormat);
  //
  settings.setValue("mainWindowGeometry", saveGeometry());
  settings.setValue("mainWindowState", saveState());
}
//----------------------------------------------------------------------------
void MartyCam::loadSettings()
{
  QString settingsFileName = QCoreApplication::applicationDirPath() + "/MartyCam.ini";
  QSettings settings(settingsFileName, QSettings::IniFormat);
}

