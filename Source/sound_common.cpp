#include "diablo.h"
#include "storm/storm.h"

void sound_store_volume(const char *key, int value) {
  SRegSaveValue("Diablo", key, 0, value);
}

void sound_load_volume(const char *value_name, int *value) {
  int v = *value;
  if (!SRegLoadValue("Diablo", value_name, 0, &v)) {
    v = VOLUME_MAX;
  }
  *value = v;

  if (*value < VOLUME_MIN) {
    *value = VOLUME_MIN;
  } else if (*value > VOLUME_MAX) {
    *value = VOLUME_MAX;
  }
  *value -= *value % 100;
}
