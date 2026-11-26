/*
  FlockAlert camera export for Overpass Turbo (https://overpass-turbo.eu)

  Pulls from OpenStreetMap:
    - ALPRs, the cameras DeFlock maps (surveillance:type=ALPR)
    - Speed cameras (highway=speed_camera, plus the "device" members
      of enforcement=maxspeed relations)

  Output is tab-separated: name, lat, lon, direction (which way the
  camera faces, if mapped), maxspeed (speed cameras only). ALPRs are named
  "<manufacturer> ALPR" ("Unknown ALPR" if untagged); speed cameras are
  named "Speed Camera".

  How to use:
    1. Paste this into Overpass Turbo.
    2. Pan and zoom the map to the area you want. {{bbox}} is the map view.
       To use a fixed area instead, see SEARCH AREA below.
    3. Click Run. Then Export > "raw data directly from Overpass API",
       or copy everything in the Data tab into a file.
    4. Either upload the saved file on the device's Wi-Fi config page
       (no rebuild needed), or build it into the firmware with:
         tools/overpass/make-coordinates.ps1 -InputFile <the saved file>

  A whole state can take a minute or two. If it times out, split the area.
*/

[out:csv(name, lat, lon, direction, maxspeed; true; "\t")][timeout:300][maxsize:536870912];

// ---------------- SEARCH AREA ----------------
// Default: the current map view ({{bbox}}, filled in by Overpass Turbo).
// For a named area instead: uncomment the two "area" lines, then replace
// every ({{bbox}}) below with (area.searchArea).
//
// area["name"="California"]["boundary"="administrative"]["admin_level"="4"];
// area._->.searchArea;

// ---------------- ALPRs (DeFlock) ----------------
node["surveillance:type"="ALPR"]({{bbox}})->.alpr;

// ---------------- Speed cameras ----------------
(
  node["highway"="speed_camera"]({{bbox}});
  rel["type"="enforcement"]["enforcement"="maxspeed"]({{bbox}});
  node(r:"device");
  // Red-light cameras: uncomment the next two lines to include them.
  // rel["type"="enforcement"]["enforcement"="traffic_signals"]({{bbox}});
  // node(r:"device");
)->.speedAll;
// Keep only nodes (drops the relations) and skip anything already
// counted as an ALPR, so nothing is listed twice.
(node.speedAll; - node.alpr;)->.speed;

// ---------------- Output ----------------
.alpr convert row
  name = (is_tag("manufacturer") ? t["manufacturer"] : "Unknown") + " ALPR",
  lat  = lat(),
  lon  = lon(),
  direction = is_tag("camera:direction") ? t["camera:direction"] : t["direction"];
out;

.speed convert row
  name = "Speed Camera",
  lat  = lat(),
  lon  = lon(),
  direction = is_tag("camera:direction") ? t["camera:direction"] : t["direction"],
  maxspeed = t["maxspeed"];
out;
