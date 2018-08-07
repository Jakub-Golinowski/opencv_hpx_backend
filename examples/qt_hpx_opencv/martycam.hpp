#ifndef HEAD_TRACKER_H
#define HEAD_TRACKER_H

#include <hpx/config.hpp>
#include <hpx/parallel/execution.hpp>

#include <QMainWindow>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
//
#include <opencv2/core/core.hpp>
//
#include "ui_martycam.h"
#include "capturethread.hpp"
#include "processingthread.hpp"

class RenderWidget;
class QDockWidget;
class SettingsWidget;

class MartyCam : public QMainWindow {
Q_OBJECT
public:
    MartyCam(const hpx::threads::executors::pool_executor&, const hpx::threads::executors::pool_executor&);

  void loadSettings();
  void saveSettings();
  void clearGraphs();

public slots:
  void updateGUI();
  void onResolutionSelected(cv::Size newSize);
  void onRotationChanged(int rotation);
  void onCameraIndexChanged(int index, QString URL);
  void onUserTrackChanged(int value);

protected:
  void closeEvent(QCloseEvent*);
  void deleteCaptureThread();
  void createCaptureThread(int FPS, cv::Size &size, int camera,
                           const std::string &cameraname, hpx::threads::executors::pool_executor exec);
  void deleteProcessingThread();
  void createProcessingThread(cv::Size size, ProcessingThread *oldThread, hpx::threads::executors::pool_executor exec,
  ProcessingType processingType);

  void resetChart();
  void initChart();

public:
  static const int IMAGE_BUFF_CAPACITY;
  Ui::MartyCam ui;
  RenderWidget            *renderWidget;
  QDockWidget             *progressToolbar;
  QDockWidget             *settingsDock;
  SettingsWidget          *settingsWidget;
  int                      cameraIndex;
  CaptureThread           *captureThread;
  ProcessingThread        *processingThread;
  ImageBuffer              imageBuffer;
  double                   UserDetectionThreshold;
  int                      EventRecordCounter;
  int                      insideMotionEvent;
  hpx::threads::executors::pool_executor blockingExecutor;
  hpx::threads::executors::pool_executor defaultExecutor;
};

#endif
