/*
   @title     StarMod
   @file      SysModules.cpp
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModules.h"
#include "Sys/SysModPrint.h"
#include "Sys/SysModUI.h"
#include "Sys/SysModWeb.h"
#include "Sys/SysModModel.h"

bool SysModules::newConnection = false;
bool SysModules::isConnected = false;

SysModules::SysModules() {
};

void SysModules::setup() {
  for (SysModule *module:modules) {
    module->setup();
  }

  //do its own setup: will be shown as last module
  JsonObject parentVar;
  parentVar = ui->initSysMod(parentVar, "Modules");
  if (parentVar["o"] > -1000) parentVar["o"] = -5000; //set default order. Don't use auto generated order as order can be changed in the ui (WIP)

  JsonObject tableVar = ui->initTable(parentVar, "mdlTbl", nullptr, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_UIFun:
      web->addResponse(var["id"], "label", "Modules");
      web->addResponse(var["id"], "comment", "List of modules");
      return true;
    default: return false;
  }});

  ui->initText(tableVar, "mdlName", nullptr, 32, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_ValueFun:
      mdl->setValue(var, modules[rowNr]->name, rowNr);
      return true;
    case f_UIFun:
      web->addResponse(var["id"], "label", "Name");
      return true;
    default: return false;
  }}, modules.size());

  ui->initCheckBox(tableVar, "mdlSuccess", UINT16_MAX, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_ValueFun:
      mdl->setValue(var, modules[rowNr]->success, rowNr);
      return true;
    case f_UIFun:
      web->addResponse(var["id"], "label", "Success");
      return true;
    default: return false;
  }}, modules.size());

  ui->initCheckBox(tableVar, "mdlEnabled", UINT16_MAX, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun not readonly! (tbd)
    case f_ValueFun:
      mdl->setValue(var, modules[rowNr]->isEnabled, rowNr);
      return true;
    case f_UIFun:
      //initially set to true, but as enabled are table cells, will be updated to an array
      web->addResponse(var["id"], "label", "Enabled");
      return true;
    case f_ChangeFun:
      if (rowNr != UINT8_MAX && rowNr < modules.size()) {
        modules[rowNr]->isEnabled = mdl->getValue(var, rowNr);
        modules[rowNr]->enabledChanged();
      }
      else {
        USER_PRINTF(" no rowNr or > modules.size!!", rowNr);
      }
      // print->printJson(" ", var);
      return true;
    default: return false;
  }}, modules.size());
}

void SysModules::loop() {
  // bool oneSec = false;
  bool tenSec = false;
  // if (millis() - oneSecondMillis >= 1000) {
  //   oneSecondMillis = millis();
  //   oneSec = true;
  // }
  // if (millis() - tenSecondMillis >= 10000) {
  //   tenSecondMillis = millis();
  //   tenSec = true;
  // }
  for (SysModule *module:modules) {
    if (module->isEnabled && module->success) {
      module->loop();
      if (millis() - module->oneSecondMillis >= 1000) {
        module->oneSecondMillis = millis();
        module->loop1s();
      }
      if (millis() - module->tenSecondMillis >= 10000) {
        module->tenSecondMillis = millis();
        module->loop10s();
      }
      // module->testManager();
      // module->performanceManager();
      // module->dataSizeManager();
      // module->codeSizeManager();
    }
  }
  if (newConnection) {
    newConnection = false;
    isConnected = true;
    connectedChanged();
  }

}

void SysModules::reboot() {
  for (SysModule *module:modules) {
    module->reboot();
  }
}

void SysModules::add(SysModule* module) {
  modules.push_back(module);
}

void SysModules::connectedChanged() {
  for (SysModule * module:modules) {
    module->connectedChanged();
  }
}