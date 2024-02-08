/*
   @title     StarMod
   @file      SysModModel.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"
#include "SysModPrint.h"
#include "SysModWeb.h"

#include "ArduinoJson.h"

typedef std::function<void(JsonObject)> FindFun;
typedef std::function<void(JsonObject, size_t)> ChangeFun;

struct Coord3D {
  uint16_t x;
  uint16_t y;
  uint16_t z;

  // Coord3D() {
  //   x = 0;
  //   y = 0;
  //   z = 0;
  // }
  // Coord3D(uint16_t x, uint16_t y, uint16_t z) {
  //   this->x = x;
  //   this->y = y;
  //   this->z = y;
  // }

  //comparisons
  bool operator!=(Coord3D rhs) {
    // USER_PRINTF("Coord3D compare%d %d %d %d %d %d\n", x, y, z, rhs.x, rhs.y, rhs.z);
    return x != rhs.x || y != rhs.y || z != rhs.z;
  }
  bool operator==(Coord3D rhs) {
    return x == rhs.x && y == rhs.y && z == rhs.z;
  }
  bool operator>=(Coord3D rhs) {
    return x >= rhs.x && y >= rhs.y && z >= rhs.z;
  }
  bool operator<=(Coord3D rhs) {
    return x <= rhs.x && y <= rhs.y && z <= rhs.z;
  }

  //assignments
  Coord3D operator=(Coord3D rhs) {
    USER_PRINTF("Coord3D assign %d,%d,%d\n", rhs.x, rhs.y, rhs.z);
    this->x = rhs.x;
    this->y = rhs.y;
    this->z = rhs.z;
    return *this;
  }
  Coord3D operator-=(Coord3D rhs) {
    this->x -= rhs.x;
    this->y -= rhs.y;
    this->z -= rhs.z;
    return *this;
  }
  Coord3D operator-(Coord3D rhs) {
    return Coord3D{uint16_t(x - rhs.x), uint16_t(y - rhs.y), uint16_t(z - rhs.z)};
  }
  Coord3D minimum(Coord3D rhs) {
    return Coord3D{min(x, rhs.x), min(y, rhs.y), min(this->z, rhs.z)};
  }
};

//used to sort keys of jsonobjects
struct ArrayIndexSortValue {
  size_t index;
  uint32_t value;
};

//https://arduinojson.org/news/2021/05/04/version-6-18-0/
namespace ArduinoJson {
  template <>
  struct Converter<Coord3D> {
    static bool toJson(const Coord3D& src, JsonVariant dst) {
      // JsonObject obj = dst.to<JsonObject>();
      dst["x"] = src.x;
      dst["y"] = src.y;
      dst["z"] = src.z;
      USER_PRINTF("Coord3D toJson %d,%d,%d -> {x:%d,y:%d,z:%d}\n", src.x, src.y, src.z, dst["x"].as<uint8_t>(), dst["y"].as<uint8_t>(), dst["z"].as<uint8_t>());
      return true;
    }

    static Coord3D fromJson(JsonVariantConst src) {
      return Coord3D{src["x"], src["y"], src["z"]};
    }

    static bool checkJson(JsonVariantConst src) {
      return src["x"].is<uint16_t>() && src["y"].is<uint16_t>() && src["z"].is<uint16_t>();
    }
  };
}

// https://arduinojson.org/v6/api/basicjsondocument/
struct RAM_Allocator {
  void* allocate(size_t size) {
    if (psramFound()) return ps_malloc(size); // use PSRAM if it exists
    else              return malloc(size);    // fallback
    // return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  }
  void deallocate(void* pointer) {
    free(pointer);
    // heap_caps_free(pointer);
  }
  void* reallocate(void* ptr, size_t new_size) {
    if (psramFound()) return ps_realloc(ptr, new_size); // use PSRAM if it exists
    else              return realloc(ptr, new_size);    // fallback
    // return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
  }
};



class SysModModel:public SysModule {

public:

  // StaticJsonDocument<24576> model; //not static as that blows up the stack. Use extern??
  // static BasicJsonDocument<DefaultAllocator> *model; //needs to be static as loopTask and asyncTask is using it...
  static BasicJsonDocument<RAM_Allocator> *model; //needs to be static as loopTask and asyncTask is using it...

  static JsonObject modelParentVar;

  bool doWriteModel = false;

  SysModModel();
  void setup();
  void loop();
  void loop1s();

  //scan all vars in the model and remove vars where var["o"] is negative or positive, if ro then remove ro values
  void cleanUpModel(JsonArray vars, bool oPos = true, bool ro = false);

  //sets the value of var with id
  template <typename Type>
  JsonObject setValue(const char * id, Type value, uint8_t rowNr = UINT8_MAX) {
    JsonObject var = findVar(id);
    if (!var.isNull()) {
      return setValue(var, value, rowNr);
    }
    else {
      USER_PRINTF("setValue Var %s not found\n", id);
      return JsonObject();
    }
  }

  template <typename Type>
  JsonObject setValue(JsonObject var, Type value, uint8_t rowNr = UINT8_MAX) {
    // print->printJson("setValueB", var);
    if (rowNr == UINT8_MAX) { //normal situation
      if (var["value"].isNull() || var["value"].as<Type>() != value) { //const char * will be JsonString so comparison works
        JsonString oldValue = JsonString(var["value"], JsonString::Copied);
        var["value"] = value;
        //trick to remove null values
        if (var["value"].isNull() || var["value"].as<uint32_t>() == UINT16_MAX) {
          var.remove("value");
          if (oldValue.size()>0)
            USER_PRINTF("dev setValue value removed %s %s\n", var["id"].as<const char *>(), oldValue.c_str()); //old value
        }
        else {
          USER_PRINTF("setValue changed %s %s->%s\n", var["id"].as<const char *>(), oldValue.c_str(), var["value"].as<String>().c_str()); //old value
          callChFunAndWs(var);
        }
      }
    }
    else {
      //if we deal with multiple rows, value should be an array, if not we create one

      if (var["value"].isNull() || !var["value"].is<JsonArray>()) {
        USER_PRINTF("setValue var %s[%d] value %s not array, creating\n", var["id"].as<const char *>(), rowNr, var["value"].as<String>().c_str());
        // print->printJson("setValueB var %s value %s not array, creating", id, var["value"].as<String>().c_str());
        var.createNestedArray("value");
      }

      if (var["value"].is<JsonArray>()) {
        JsonArray valueArray = var["value"].as<JsonArray>();
        //set the right value in the array (if array did not contain values yet, all values before rownr are set to false)
        bool notSame = true; //rowNr >= size

        if (rowNr < valueArray.size())
          notSame = valueArray[rowNr].isNull() || valueArray[rowNr].as<Type>() != value;

        if (notSame) {
          // setValue(var, value, rowNr);
          // if (rowNr >= valueArray.size())
          //   USER_PRINTF("notSame %d %d\n", rowNr, valueArray.size());
          valueArray[rowNr] = value; //if valueArray[<rowNr] not exists it will be created
          // USER_PRINTF("  assigned %d %d %s\n", rowNr, valueArray.size(), valueArray[rowNr].as<String>().c_str());
          callChFunAndWs(var, rowNr);
        }
      }
      else {
        USER_PRINTF("setValue %s could not create value array\n", var["id"].as<const char *>());
      }
    }
    
    return var;
  }

  //Set value with argument list with rowNr (rowNr cannot have a default)
  JsonObject setValueV(const char * id, uint8_t rowNr = UINT8_MAX, const char * format = nullptr, ...) {
    va_list args;
    va_start(args, format);

    char value[128];
    vsnprintf(value, sizeof(value)-1, format, args);

    va_end(args);

    USER_PRINTF("%s\n", value);
    return setValue(id, JsonString(value, JsonString::Copied), rowNr);
  }

  JsonObject setUIValueV(const char * id, const char * format, ...) {
    va_list args;
    va_start(args, format);

    char value[128];
    vsnprintf(value, sizeof(value)-1, format, args);

    va_end(args);

    JsonObject responseObject = web->getResponseDoc()->to<JsonObject>();

    web->addResponse(id, "value", JsonString(value, JsonString::Copied));

    web->sendDataWs(responseObject);

    return JsonObject();
  }

  JsonVariant getValue(const char * id, uint8_t rowNr = UINT8_MAX) {
    JsonObject var = findVar(id);
    if (!var.isNull()) {
      return getValue(var, rowNr);
    }
    else {
      // USER_PRINTF("getValue: Var %s does not exist!!\n", id);
      return JsonVariant();
    }
  }
  JsonVariant getValue(JsonObject var, uint8_t rowNr = UINT8_MAX) {
    if (var["value"].is<JsonArray>()) {
      JsonArray valueArray = var["value"].as<JsonArray>();
      if (rowNr != UINT8_MAX && rowNr < valueArray.size())
        return valueArray[rowNr];
      else if (valueArray.size())
        return valueArray[0];
      else {
        USER_PRINTF("dev getValue no array or rownr wrong %s %s %d\n", var["id"].as<const char *>(), var["value"].as<String>().c_str(), rowNr);
        return JsonVariant();
      }
    }
    else
      return var["value"];
  }

  //returns the var defined by id (parent to recursively call findVar)
  static JsonObject findVar(const char * id, JsonArray parent = JsonArray()); //static for processJson
  void findVars(const char * id, bool value, FindFun fun, JsonArray parent = JsonArray());

  //recursively add values in  a variant
  void varToValues(JsonObject var, JsonArray values);

  //run the change function and send response to all? websocket clients
  static void callChFunAndWs(JsonObject var, uint8_t rowNr = UINT8_MAX, const char * value = nullptr);

private:
  bool doShowObsolete = false;
  bool cleanUpModelDone = false;

};

static SysModModel *mdl;