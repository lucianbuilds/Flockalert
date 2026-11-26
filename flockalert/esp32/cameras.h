#ifndef CAMERAS_H
#define CAMERAS_H

// ============================================================
// CAMERA DATABASE
// ============================================================
//
// Cameras are loaded into RAM at boot, sorted by latitude, so a
// position check only looks at cameras in a narrow band around the
// car instead of every entry.
//
// Source, in order of preference:
//   1. /cameras.csv on LittleFS, uploaded from the Wi-Fi portal.
//      This is the raw Overpass Turbo output of
//      tools/overpass/cameras.overpassql (tab-separated
//      name, lat, lon, direction, maxspeed).
//   2. The built-in waypoints[] list in coordinates.h.

#include <LittleFS.h>
#include <WebServer.h>
#include <math.h>

#include "coordinates.h"

struct Camera {
  float lat;
  float lon;
  int16_t direction;   // Degrees the camera faces, -1 = unknown
  uint8_t limitMph;    // Speed limit for speed cameras, 0 = unknown
  uint8_t nameIdx;     // Index into cameraNames[]
};

constexpr int MAX_CAMERA_NAMES = 64;

constexpr const char* CAMERA_FILE = "/cameras.csv";
constexpr const char* CAMERA_TEMP_FILE = "/cameras.tmp";

Camera* cameras = nullptr;
size_t cameraCount = 0;
bool camerasFromFile = false;

char* cameraNames[MAX_CAMERA_NAMES];
bool cameraNameIsSpeed[MAX_CAMERA_NAMES];
int cameraNameCount = 0;

extern WebServer configServer;
extern bool alertActive;
extern int activeWaypoint;


// ============================================================
// LOOKUPS
// ============================================================

const char* cameraName(int index) {
  return cameraNames[cameras[index].nameIdx];
}

bool isSpeedCamera(int index) {
  return index >= 0 &&
    cameraNameIsSpeed[cameras[index].nameIdx];
}

// Ground distance in meters (equirectangular approximation, well
// under 1% error at the distances that matter here).
double fastDistanceM(
  double lat1, double lon1,
  double lat2, double lon2
) {
  double x = (lon2 - lon1) *
    cos((lat1 + lat2) * 0.5 * DEG_TO_RAD);
  double y = lat2 - lat1;
  return sqrt(x * x + y * y) * 111195.0;
}


// ============================================================
// NAMES
// ============================================================

// Names are shared: thousands of cameras use a handful of names.
// Slots 0 and 1 are generic fallbacks used if the table fills up.
static uint8_t internCameraName(const char* name) {

  for (int i = 0; i < cameraNameCount; i++) {
    if (strcmp(cameraNames[i], name) == 0) {
      return i;
    }
  }

  bool speed = strncmp(name, "Speed Cam", 9) == 0;

  if (cameraNameCount >= MAX_CAMERA_NAMES) {
    return speed ? 0 : 1;
  }

  cameraNames[cameraNameCount] = strdup(name);
  cameraNameIsSpeed[cameraNameCount] = speed;
  return cameraNameCount++;
}

static void resetCameraNames() {

  for (int i = 0; i < cameraNameCount; i++) {
    free(cameraNames[i]);
  }

  cameraNameCount = 0;
  internCameraName("Speed Camera");
  internCameraName("Unknown ALPR");
}


// ============================================================
// PARSING
// ============================================================

// "45", "45;225" (first value wins), or a compass point like "NE".
int16_t parseDirection(const char* s) {

  while (*s == ' ') s++;

  if (!*s) return -1;

  if (isdigit((unsigned char)*s) || *s == '-') {
    int v = atoi(s);
    return ((v % 360) + 360) % 360;
  }

  static const char* points[16] = {
    "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
  };

  char buf[4];
  int n = 0;

  while (n < 3 && isalpha((unsigned char)s[n])) {
    buf[n] = toupper((unsigned char)s[n]);
    n++;
  }

  buf[n] = '\0';

  for (int i = 0; i < 16; i++) {
    if (strcmp(buf, points[i]) == 0) {
      return (int16_t)(i * 22.5 + 0.5);
    }
  }

  return -1;
}

// OSM maxspeed: "25 mph", or a plain number in km/h.
uint8_t parseLimitMph(const char* s) {

  int v = atoi(s);

  if (v <= 0) return 0;

  if (strstr(s, "mph")) return min(v, 255);

  return (uint8_t)min(255, (int)lround(v * 0.621371));
}

// Splits a tab-separated line in place. Handles "quoted" fields
// with "" escapes, which Overpass uses for names containing commas.
static int splitTsv(char* line, char** fields, int maxFields) {

  int n = 0;
  char* p = line;

  while (n < maxFields) {

    if (*p == '"') {

      char* start = ++p;
      char* w = p;

      while (*p) {
        if (*p == '"') {
          if (p[1] == '"') { *w++ = '"'; p += 2; continue; }
          p++;
          break;
        }
        *w++ = *p++;
      }

      char* q = p;
      while (*q && *q != '\t') q++;
      bool more = *q == '\t';

      *w = '\0';
      fields[n++] = start;

      if (!more) break;
      p = q + 1;

    } else {

      fields[n++] = p;
      while (*p && *p != '\t') p++;

      if (!*p) break;
      *p++ = '\0';
    }
  }

  return n;
}


// ============================================================
// LOADING
// ============================================================

static bool loadCamerasFromFile() {

  File f = LittleFS.open(CAMERA_FILE, "r");

  if (!f) return false;

  // Pass 1: count lines to size the array.
  size_t lines = 1;
  uint8_t chunk[256];

  while (f.available()) {
    size_t got = f.read(chunk, sizeof(chunk));
    for (size_t i = 0; i < got; i++) {
      if (chunk[i] == '\n') lines++;
    }
  }

  cameras = (Camera*)malloc(lines * sizeof(Camera));

  if (!cameras) {
    Serial.println("Camera file too large for RAM.");
    f.close();
    return false;
  }

  // Pass 2: parse.
  f.seek(0);

  char line[192];
  char* fld[5];

  while (f.available() && cameraCount < lines) {

    size_t len = f.readBytesUntil('\n', line, sizeof(line) - 1);
    line[len] = '\0';

    if (len && line[len - 1] == '\r') line[len - 1] = '\0';

    int k = splitTsv(line, fld, 5);

    if (k < 3) continue;

    char* end;
    double lat = strtod(fld[1], &end);

    if (end == fld[1]) continue;   // header row

    Camera& c = cameras[cameraCount++];
    c.lat = lat;
    c.lon = atof(fld[2]);
    c.direction = k > 3 ? parseDirection(fld[3]) : -1;
    c.limitMph = k > 4 ? parseLimitMph(fld[4]) : 0;
    c.nameIdx = internCameraName(fld[0]);
  }

  f.close();

  if (cameraCount == 0) {
    free(cameras);
    cameras = nullptr;
    return false;
  }

  return true;
}

static void loadCamerasFromBuiltIn() {

  cameras = (Camera*)malloc(WAYPOINT_COUNT * sizeof(Camera));

  if (!cameras) return;

  for (size_t i = 0; i < WAYPOINT_COUNT; i++) {
    Camera& c = cameras[cameraCount++];
    c.lat = waypoints[i].latitude;
    c.lon = waypoints[i].longitude;
    c.direction = waypoints[i].direction;
    c.limitMph = waypoints[i].maxspeedMph;
    c.nameIdx = internCameraName(waypoints[i].name);
  }
}

static int compareCameraLat(const void* a, const void* b) {
  float d = ((const Camera*)a)->lat - ((const Camera*)b)->lat;
  return (d > 0) - (d < 0);
}

void loadCameraDatabase() {

  static bool fsReady = LittleFS.begin(true);

  free(cameras);
  cameras = nullptr;
  cameraCount = 0;

  alertActive = false;
  activeWaypoint = -1;

  resetCameraNames();

  camerasFromFile = fsReady && loadCamerasFromFile();

  if (!camerasFromFile) {
    cameraCount = 0;
    resetCameraNames();
    loadCamerasFromBuiltIn();
  }

  qsort(cameras, cameraCount, sizeof(Camera), compareCameraLat);

  Serial.printf(
    "Loaded %u cameras from %s. Free heap: %u bytes\n",
    (unsigned)cameraCount,
    camerasFromFile ? "uploaded file" : "built-in list",
    (unsigned)ESP.getFreeHeap()
  );
}


// ============================================================
// SEARCH
// ============================================================

// Returns true if a camera should be considered (e.g. it is ahead).
typedef bool (*CameraFilter)(int index, double distanceM);

static size_t firstCameraAtOrAbove(float lat) {

  size_t lo = 0, hi = cameraCount;

  while (lo < hi) {
    size_t mid = (lo + hi) / 2;
    if (cameras[mid].lat < lat) lo = mid + 1;
    else hi = mid;
  }

  return lo;
}

// Nearest camera that passes the filter. Searches a small latitude
// band first and widens it only when nothing close enough is found.
int findClosestCamera(
  double lat,
  double lon,
  double& closestDistance,
  CameraFilter filter
) {

  static const double bands[] = { 0.02, 0.2, 2.0, 180.0 };

  double lonScale = max(cos(lat * DEG_TO_RAD), 0.01);

  for (double band : bands) {

    int best = -1;
    closestDistance = 1e30;

    double lonBand = band / lonScale;

    for (size_t i = firstCameraAtOrAbove(lat - band);
         i < cameraCount && cameras[i].lat <= lat + band;
         i++) {

      if (fabs(cameras[i].lon - lon) > lonBand) continue;

      double d = fastDistanceM(lat, lon, cameras[i].lat, cameras[i].lon);

      if (d < closestDistance && (!filter || filter(i, d))) {
        closestDistance = d;
        best = i;
      }
    }

    // Anything outside the band is farther than band * 111 km.
    if (best >= 0 && closestDistance <= band * 111195.0) {
      return best;
    }
  }

  return -1;
}


// ============================================================
// PORTAL UPLOAD
// ============================================================

File cameraUploadFile;
bool cameraUploadOk = false;

static void redirectHome() {
  configServer.sendHeader("Location", "/", true);
  configServer.send(303, "text/plain", "");
}

void handleCameraUploadData() {

  HTTPUpload& up = configServer.upload();

  if (up.status == UPLOAD_FILE_START) {

    cameraUploadFile = LittleFS.open(CAMERA_TEMP_FILE, "w");
    cameraUploadOk = (bool)cameraUploadFile;

  } else if (up.status == UPLOAD_FILE_WRITE) {

    if (cameraUploadOk &&
        cameraUploadFile.write(up.buf, up.currentSize) != up.currentSize) {
      cameraUploadOk = false;   // Flash full
    }

  } else {

    if (cameraUploadFile) cameraUploadFile.close();

    if (up.status == UPLOAD_FILE_ABORTED) cameraUploadOk = false;
  }
}

void handleCameraUploadDone() {

  if (cameraUploadOk) {
    LittleFS.remove(CAMERA_FILE);
    LittleFS.rename(CAMERA_TEMP_FILE, CAMERA_FILE);
    loadCameraDatabase();
  } else {
    LittleFS.remove(CAMERA_TEMP_FILE);
  }

  if (!cameraUploadOk || !camerasFromFile) {

    LittleFS.remove(CAMERA_FILE);
    loadCameraDatabase();

    configServer.send(
      400,
      "text/plain",
      "Upload failed: the file could not be saved, had no camera rows, "
      "or did not fit in memory. Using the built-in list."
    );
    return;
  }

  redirectHome();
}

void handleCameraReset() {
  LittleFS.remove(CAMERA_FILE);
  loadCameraDatabase();
  redirectHome();
}

#endif
