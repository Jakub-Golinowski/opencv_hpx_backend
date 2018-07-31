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
    this->createProcessingThread(this->imageSize, nullptr, defaultExec);
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
  this->captureThread = new CaptureThread(imageBuffer, size, camera, cameraname, this->blockingExecutor);
  this->captureThread->setRotation(this->settingsWidget->getSelectedRotation());
  this->captureThread->startCapture();
  //TODO for now I need to enforce the resolution size here again but it is more of a hotfix than a good solution -> ultimately I want all this encapsulated in the Capture thread constructor.
  this->captureThread->setResolution(size);

  this->settingsWidget->setThreads(this->captureThread, this->processingThread);
}
//----------------------------------------------------------------------------
void MartyCam::deleteCaptureThread()
{
  this->captureThread->setAbort(true);
  this->captureThreadFinished.wait();
  this->imageBuffer->clear();
  delete captureThread;
  this->captureThread = nullptr;
  // push an empty image to the image buffer to ensure that any waiting processing thread is freed
  this->imageBuffer->send(cv::Mat());
}
//----------------------------------------------------------------------------
void MartyCam::createProcessingThread(cv::Size &size, ProcessingThread *oldThread,
                                      hpx::threads::executors::pool_executor exec)
{
  this->processingThread = new ProcessingThread(imageBuffer, this->imageSize);
  if (oldThread) this->processingThread->CopySettings(oldThread);
  this->processingThread->setRootFilter(renderWidget);
  this->settingsWidget->setThreads(this->captureThread, this->processingThread);
  //
  //TODO undestand what this signals were doing here and write code that does the same in another way
//  connect(temp, SIGNAL(NewData()),
//    this, SLOT(updateGUI()),Qt::QueuedConnection);
  //
  this->processingThreadFinished =
          hpx::async(exec, &ProcessingThread::run, this->processingThread);
}
//----------------------------------------------------------------------------
void MartyCam::deleteProcessingThread()
{
  this->processingThread->setAbort(true);
  this->processingThreadFinished.wait();
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

  bool wasActive = this->captureThread->stopCapture();

  this->imageBuffer->clear();

  //Change the settings
  //todo why is the image size kept twice? Look into it.
  this->imageSize = newSize;
  this->captureThread->imageSize = newSize;
  this->captureThread->rotatedSize = cv::Size(this->imageSize.height, this->imageSize.width);
  this->captureThread->setResolution(this->imageSize);

  // Update GUI renderwidget size
  this->renderWidget->setCVSize(newSize);

  this->captureThread->startCapture();
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

  statusBar()->showMessage(QString("FPS : %1, Frame Counter : %2, Image Buffer Occupancy : %3\%").
    arg(this->captureThread->getFPS(), 5, 'f', 2).
    arg(captureThread->GetFrameCounter(), 5).
    arg(100 * (float)this->imageBuffer->size()/IMAGE_BUFF_CAPACITY, 4));

  QDateTime now = QDateTime::currentDateTime();
  QDateTime start = this->settingsWidget->TimeLapseStart();
  QDateTime stop = this->settingsWidget->TimeLapseEnd();
  QTime interval = this->settingsWidget->TimeLapseInterval();
  int secs = (interval.hour()*60 + interval.minute())*60 + interval.second();
  QDateTime next = this->lastTimeLapse.addSecs(secs);

  if (!this->captureThread->TimeLapseAVI_Writing) {
    if (this->settingsWidget->TimeLapseEnabled()) {
      if (now>start && now<stop) {
        this->settingsWidget->SetupAVIStrings();
        this->captureThread->startTimeLapse(this->settingsWidget->TimeLapseFPS());
        this->captureThread->updateTimeLapse();
        this->lastTimeLapse = now;
      }
    }
  }
  else if ((now>next && now<stop) && this->settingsWidget->TimeLapseEnabled()) {
    this->captureThread->updateTimeLapse();
    this->lastTimeLapse = now;
  }
  else if (now>stop || !this->settingsWidget->TimeLapseEnabled()) {
    this->captureThread->stopTimeLapse();
  }
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

