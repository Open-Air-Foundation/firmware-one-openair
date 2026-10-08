#ifndef _AG_OLED_DISPLAY_H_
#define _AG_OLED_DISPLAY_H_

#include "AgConfigure.h"
#include "AgValue.h"
#include "AirGradient.h"
#include "Main/PrintLog.h"
#include <Arduino.h>
#ifdef ESP32
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#endif

class OledDisplay : public PrintLog {
private:
  Configuration &config;
  AirGradient *ag;
  bool isBegin = false;
  void *u8g2 = NULL;
  Measurements &value;
  bool isDisplayOff = false;
#ifdef ESP32
  /**
   * Serializes every access to the display. The display is driven from more
   * than one FreeRTOS task: loop() redraws the dashboard every
   * DISP_UPDATE_INTERVAL, while configuration updates (cloud config sync in
   * NetworkingTask, PUT /config in the webserver task) call setBrightness()
   * from a higher-priority task. Without this lock a brightness change can
   * preempt a dashboard redraw between its isDisplayOff check and its
   * sendBuffer(), so the display is cleared and then immediately redrawn and
   * stays lit while isDisplayOff == true (no later redraw or clear happens
   * until the brightness changes again).
   */
  SemaphoreHandle_t mutex = NULL;
#endif
  void lock(void);
  void unlock(void);
  friend class OledDisplayLock;

  typedef struct {
    int width;
    int height;
    unsigned char *icon;
  } xbm_icon_t;

  void showTempHum(bool hasStatus);
  void setCentralText(int y, String text);
  void setCentralText(int y, const char *text);
  void showIcon(int x, int y, xbm_icon_t *icon);

public:
  OledDisplay(Configuration &config, Measurements &value, Stream &log);
  ~OledDisplay();

  enum DashboardStatus {
    DashBoardStatusNone,
    DashBoardStatusWiFiIssue,
    DashBoardStatusServerIssue,
    DashBoardStatusAddToDashboard,
    DashBoardStatusDeviceId,
    DashBoardStatusOfflineMode,
  };

  void setAirGradient(AirGradient *ag);
  bool begin(void);
  void end(void);
  void setText(String &line1, String &line2, String &line3);
  void setText(const char *line1, const char *line2, const char *line3);
  void setText(String &line1, String &line2, String &line3, String &line4);
  void setText(const char *line1, const char *line2, const char *line3,
               const char *line4);
  void showWiFiProvisioning(bool firstRun, int countdown);
  void showDashboard(void);
  void showDashboard(DashboardStatus status);
  void setBrightness(int percent);
#ifdef ESP32
  void showFirmwareUpdateVersion(String version);
  void showFirmwareUpdateProgress(int percent);
  void showFirmwareUpdateSuccess(int count);
  void showFirmwareUpdateFailed(void);
  void showFirmwareUpdateSkipped(void);
  void showFirmwareUpdateUpToDate(void);
#else

#endif
  void showRebooting(void);
};

#endif /** _AG_OLED_DISPLAY_H_ */
