#include "settings.hpp"
#include <QDebug>
#include <QFileDialog>
#include <QSettings>
#include <QClipboard>
#include <QMessageBox>
//
#include "renderwidget.hpp"
//
#ifdef WIN32
 #include "videoInput.h"
#endif
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/highgui/highgui_c.h>
//
// we need these to get access to videoInput
// Caution: including cpp file here as routines are not exported from openCV
//#include "../../highgui/src/precomp.hpp"
//#include "../../highgui/src/cap_dshow.cpp"

//----------------------------------------------------------------------------
SettingsWidget::SettingsWidget(QWidget* parent) : QWidget(parent)
{
  this->processingthread = NULL;
  this->capturethread    = NULL;
  this->SnapshotId       = 0;
  //
  ui.setupUi(this);
  setMinimumWidth(150);
  //
  connect(ui.threshold, SIGNAL(valueChanged(int)), this, SLOT(onThresholdChanged(int)));
  connect(ui.average, SIGNAL(valueChanged(int)), this, SLOT(onAverageChanged(int)));
  connect(ui.erode, SIGNAL(valueChanged(int)), this, SLOT(onErodeChanged(int)));
  connect(ui.dilate, SIGNAL(valueChanged(int)), this, SLOT(onDilateChanged(int)));
  connect(ui.browse, SIGNAL(clicked()), this, SLOT(onBrowseClicked()));
  connect(ui.add_camera, SIGNAL(clicked()), this, SLOT(onAddCameraClicked()));

  connect(ui.WriteMotionAVI, SIGNAL(toggled(bool)), this, SLOT(onWriteMotionAVIToggled(bool)));
  connect(&this->clock, SIGNAL(timeout()), this, SLOT(onTimer()));
  connect(ui.blendRatio, SIGNAL(valueChanged(int)), this, SLOT(onBlendChanged(int)));
  //
  connect(ui.snapButton, SIGNAL(clicked()), this, SLOT(onSnapClicked()));

  ResolutionButtonGroup.addButton(ui.res1600,4);
  ResolutionButtonGroup.addButton(ui.res1280,3);
  ResolutionButtonGroup.addButton(ui.res720,2);
  ResolutionButtonGroup.addButton(ui.res640,1);
  ResolutionButtonGroup.addButton(ui.res320,0);
  connect(&ResolutionButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(onResolutionSelection(int)));

  ImageButtonGroup.addButton(ui.cameraImage,0);
  ImageButtonGroup.addButton(ui.movingAverage,1);
  ImageButtonGroup.addButton(ui.differenceImage,2);
  ImageButtonGroup.addButton(ui.blendedImage,3);
  ImageButtonGroup.addButton(ui.maskImage,4);
  connect(&ImageButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(onImageSelection(int)));

  RotateButtonGroup.addButton(ui.rotate0,0);
  RotateButtonGroup.addButton(ui.rotate90,1);
  RotateButtonGroup.addButton(ui.rotate90m,2);
  RotateButtonGroup.addButton(ui.rotate180,3);
  connect(&RotateButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(onRotateSelection(int)));
  //
  this->setupCameraList();
}
//----------------------------------------------------------------------------
void SettingsWidget::setThreads(CaptureThread *capthread, ProcessingThread *procthread)
{
  this->capturethread = capthread;
  this->processingthread = procthread;
}
//----------------------------------------------------------------------------
void SettingsWidget::onThresholdChanged(int value)
{
  this->ui.t_label->setText(QString("%1").arg(value, 2));
  this->processingthread->setThreshold(value);
}
//----------------------------------------------------------------------------
void SettingsWidget::onAverageChanged(int value)
{
  double newval = value/100.0;
  QString label = QString("%1").arg(newval,4 , 'f', 2);
  this->ui.a_label->setText(label);
  this->processingthread->setAveraging(value/100.0);
}
//----------------------------------------------------------------------------
void SettingsWidget::onErodeChanged(int value)
{
  this->ui.e_label->setText(QString("%1").arg(value, 2));
  this->processingthread->setErodeIterations(value);
}
//----------------------------------------------------------------------------
void SettingsWidget::onDilateChanged(int value)
{
  this->ui.d_label->setText(QString("%1").arg(value, 2));
  this->processingthread->setDilateIterations(value);
}
//----------------------------------------------------------------------------
void SettingsWidget::onBrowseClicked()
{
  QFileDialog dialog;
  dialog.setViewMode(QFileDialog::Detail);
  dialog.setFileMode(QFileDialog::DirectoryOnly);
  if(dialog.exec()) {
    QString fileName = dialog.selectedFiles().at(0);
    this->ui.avi_directory->setText(fileName);
    this->capturethread->setWriteMotionAVIDir(fileName.toStdString().c_str());
  }
}
//----------------------------------------------------------------------------
void SettingsWidget::onAddCameraClicked()
{
    //TODO implement this or delete the GUI element
    QMessageBox::warning(
        this,
        tr("Not implemented warning."),
        tr("Switching camera from the default is not currently available.") );
}
//----------------------------------------------------------------------------
void SettingsWidget::setupCameraList()
{
  int index = this->ui.cameraSelect->currentIndex();
  this->ui.cameraSelect->blockSignals(true);
  this->ui.cameraSelect->clear();
  //
#ifdef _WIN32
  // use videoInput object to enumerate devices
  this->NumDevices = videoInput::listDevices(true);
  for (int i=0; i<this->NumDevices; i++) {
    this->ui.cameraSelect->addItem(QString(videoInput::getDeviceName(i)));
  }
#else
  this->NumDevices = 1;
  this->ui.cameraSelect->addItem("standard camera");
#endif
}
//----------------------------------------------------------------------------
void SettingsWidget::onWriteMotionAVIToggled(bool state)
{
  this->RecordMotionAVI(state);
}
//----------------------------------------------------------------------------
void SettingsWidget::onTimer()
{
  QTime now = QTime::currentTime().addMSecs(500);
  int el = this->AVI_StartTime.elapsed();
  int h  = el/3600000>0?el/3600000:0;
  int m  = el-h*3600000>0?(el-h*3600000)/60000:0;
  int s  = el-h*3600000-m*60000>0?(el-(h*3600000+m*60000))/1000:0;
  int ms = el-(h*3600000+m*60000+s*1000)>0?el-(h*3600000+m*60000+s*1000):el;
  if (ms>500) s+=1;
  this->ui.elapsed->setText(QTime(h,m,s,0).toString("hh:mm:ss"));
  //
  int re = now.secsTo(this->AVI_EndTime);
  if (re>0) {
    h = re/3600>0 ? re/3600 : 0;
    m = re-h*3600>0 ? (re-h*3600)/60 : 0;
    s = re-h*3600-m*60>0 ? (re-(h*3600+m*60)) : 0;
    this->ui.remaining->setText(QTime(h,m,s,0).toString("hh:mm:ss"));
  }
  else {
    this->ui.WriteMotionAVI->setChecked(false);
  }
}
//----------------------------------------------------------------------------
int SettingsWidget::getCameraIndex(std::string &text)
{
  QString val = this->ui.cameraSelect->currentText();
  int index = this->ui.cameraSelect->currentIndex();
  // if this is not an autodetected webcam/internal camera, return the user name
  if (index>NumDevices) {
    text = val.toStdString();
  }
  else {
    text = "";
  }
  return index;
}
//----------------------------------------------------------------------------
void SettingsWidget::SetupAVIStrings()
{
  QString filePath = this->ui.avi_directory->text();
  QString fileName = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
  this->capturethread->setWriteMotionAVIName(fileName.toLatin1().constData());
  this->capturethread->setWriteMotionAVIDir(filePath.toLatin1().constData());
}
//----------------------------------------------------------------------------
void SettingsWidget::RecordMotionAVI(bool state)
{
  this->ui.WriteMotionAVI->setChecked(state);
  //
  QString filePath = this->ui.avi_directory->text();
  QString fileName = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
  this->capturethread->setWriteMotionAVIName(fileName.toLatin1().constData());
  this->capturethread->setWriteMotionAVIDir(filePath.toLatin1().constData());

  //
  // add duration to end time even if already running
  //
  QTime duration = this->ui.AVI_Duration->time();
  int secs = duration.second();
  int mins = duration.minute();
  int hrs  = duration.hour();
  this->AVI_EndTime = QTime::currentTime().addSecs(secs+mins*60+hrs*60*60).addMSecs(500);
  //
  if (!state) {
    this->capturethread->setWriteMotionAVI(state);
    this->ui.WriteMotionAVI->setText("Write AVI");
    this->capturethread->closeAVI();
    this->clock.stop();
  }
  else if (!this->capturethread->getWriteMotionAVI()) {
    this->capturethread->setWriteMotionAVI(state);
    this->AVI_StartTime = QTime::currentTime();
    this->ui.WriteMotionAVI->setText("Stop AVI");
    this->ui.elapsed->setText("00:00:00");
    this->ui.remaining->setText(duration.toString("hh:mm:ss"));
    this->clock.setInterval(1000);
    this->clock.start();
    this->onTimer();
  }
}
//----------------------------------------------------------------------------
void SettingsWidget::onImageSelection(int btn)
{
  this->processingthread->setDisplayImage(btn);
}
//----------------------------------------------------------------------------
cv::Size SettingsWidget::getSelectedResolution()
{
  switch (this->ResolutionButtonGroup.checkedId()) {
    case 4:
      return cv::Size(1600,1200);
      break;
    case 3:
      return cv::Size(1280,720);
      break;
    case 2:
      return cv::Size(720,576);
      break;
    case 1:
      return cv::Size(640,480);
      break;
    case 0:
      return cv::Size(320,240);
      break;
  }
  return cv::Size(320,240);
}
//----------------------------------------------------------------------------
void SettingsWidget::onResolutionSelection(int btn)
{
  emit(resolutionSelected(getSelectedResolution()));
}
//----------------------------------------------------------------------------
int SettingsWidget::getSelectedRotation()
{
  return this->RotateButtonGroup.checkedId();
}
//----------------------------------------------------------------------------
void SettingsWidget::onRotateSelection(int btn)
{
  emit(rotationChanged(btn));
}
//----------------------------------------------------------------------------
void SettingsWidget::onBlendChanged(int value)
{
  this->processingthread->setBlendRatios(this->ui.blendRatio->value()/100.0);
}
//----------------------------------------------------------------------------
void SettingsWidget::saveSettings()
{
  QString settingsFileName = QCoreApplication::applicationDirPath() + "/MartyCam.ini";
  QSettings settings(settingsFileName, QSettings::IniFormat);
  //
  settings.beginGroup("MotionDetection");
  settings.setValue("threshold",this->ui.threshold->value());
  settings.setValue("average",this->ui.average->value());
  settings.setValue("erode",this->ui.erode->value());
  settings.setValue("dilate",this->ui.dilate->value());
  settings.endGroup();

  settings.beginGroup("UserSettings");
  settings.setValue("resolution",this->ResolutionButtonGroup.checkedId());
  settings.setValue("rotation",this->RotateButtonGroup.checkedId());
  settings.setValue("display",this->ImageButtonGroup.checkedId());
  settings.setValue("cameraIndex",this->ui.cameraSelect->currentIndex());
  settings.setValue("aviDirectory",this->ui.avi_directory->text());
  settings.setValue("blendImage",this->ui.blendRatio->value());
  settings.endGroup();

  settings.beginGroup("MotionAVI");
  settings.setValue("snapshot",this->SnapshotId);
  settings.setValue("aviDuration", this->ui.AVI_Duration->time());
  settings.endGroup();
}
//----------------------------------------------------------------------------
void SettingsWidget::loadSettings()
{
  QString settingsFileName = QCoreApplication::applicationDirPath() + "/MartyCam.ini";
  QSettings settings(settingsFileName, QSettings::IniFormat);
  //
  settings.beginGroup("MotionDetection");
  SilentCall(this->ui.threshold)->setValue(settings.value("threshold",3).toInt());
  SilentCall(this->ui.average)->setValue(settings.value("average",10).toInt());
  SilentCall(this->ui.erode)->setValue(settings.value("erode",1).toInt());
  SilentCall(this->ui.dilate)->setValue(settings.value("dilate",1).toInt());
  settings.endGroup();

  settings.beginGroup("UserSettings");
  SilentCall(&this->ResolutionButtonGroup)->button(settings.value("resolution",0).toInt())->click();
  SilentCall(&this->RotateButtonGroup)->button(settings.value("rotation",0).toInt())->click();
  SilentCall(&this->ImageButtonGroup)->button(settings.value("display",0).toInt())->click();
  SilentCall(this->ui.cameraSelect)->setCurrentIndex(settings.value("cameraIndex",0).toInt());
  SilentCall(this->ui.avi_directory)->setText(settings.value("aviDirectory","$HOME/wildlife").toString());
  //
  SilentCall(this->ui.blendRatio)->setValue(settings.value("blendImage",0.5).toInt());
  settings.endGroup();

  settings.beginGroup("MotionAVI");
  this->SnapshotId = settings.value("snapshot",0).toInt();
  SilentCall(this->ui.AVI_Duration)->setTime(settings.value("aviDuration", QTime(0,0,10)).toTime());
  settings.endGroup();
}
//----------------------------------------------------------------------------
void SettingsWidget::onSnapClicked()
{
  QPixmap p = this->renderWidget->grab();
  QString filename = QString("%1/MartySnap-%2").arg(this->ui.avi_directory->text()).arg(SnapshotId++, 3, 10, QChar('0')) + QString(".png");
  p.save(filename);
  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setPixmap(p);
}

