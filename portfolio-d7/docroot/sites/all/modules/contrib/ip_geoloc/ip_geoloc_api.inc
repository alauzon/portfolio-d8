<?php

/**
 * @file
 * API functions of IP geolocation module
 *
 * Reusable functions that may be called from other modules.
 */

define('IP_GEOLOC_GEO_JS', 'http://geo-location-javascript.googlecode.com/svn/trunk/js/geo.js'); // size 14k uncompressed
define('IP_GEOLOC_GOOGLE_GEARS_INIT',  'http://code.google.com/apis/gears/gears_init.js'); // size 4K uncompressed
define('IP_GEOLOC_GOOGLE_MAPS',        'http://maps.google.com/maps/api/js?sensor=false'); // size 5k
define('IP_GEOLOC_GOOGLE_MAPS_GPS',    'http://maps.google.com/maps/api/js?sensor=true');  // size 5k
define('IP_GEOLOC_GOOGLE_MAPS_SERVER', 'http://maps.googleapis.com/maps/api/geocode/json?sensor=false');

/**
 * Store the supplied IP geolocation info on the database.
 *
 * This will overwrite any existing info for the IP address in question.
 *
 * @param
 *   $location, array with up to 13 location info fields; must at least
 *   contain a non-empty $location['ip_address'] and a non-empty
 *   $location['formatted_address'] for anything to happen
 * @return
 *   0, when no insert or update was necessary
 *   SAVED_NEW (=1), when a new location record was inserted into the db
 *   SAVED_UPDATED (=2), when an existing location record was updated
 *   FALSE, when a db insert or db update failed
 */
function ip_geoloc_store_location($location) {
  // Give contributed modules a chance to add their 2 cents by implementing
  // hook_get_ip_geolocation_alter()
  drupal_alter('get_ip_geolocation', $location);

  $result = 0;
  if (empty($location['ip_address']) || empty($location['formatted_address'])) {
    // ip_geoloc_debug('IP Geolocation: location object must contain both IP address and formatted address -- not stored.');
  }
  else {
    if ($location['ip_address'] != '127.0.0.1' && (!isset($location['latitude']) || !isset($location['latitude']))) {
      drupal_set_message(t('IP Geolocation: lat or long missing for IP address %ip (location still stored).', array('%ip' => $location['ip_address'])));
    }
    // See if this IP is already on the db
    $result = db_query('SELECT * FROM {ip_geoloc} WHERE ip_address = :ip', array(':ip' => $location['ip_address']));
    $existing_location = $result->fetchAssoc();

    if ($existing_location == FALSE) { // New entry, insert
      ip_geoloc_debug(t('IP Geolocaton: adding new record to db: !location', array('!location' => ip_geoloc_pretty_print($location))));
      $full_location = &$location;
    }
    else {
      // When updating, drupal_write_record() does not erase fields not present in $location
      $empty_location['latitude'] = '';
      $empty_location['longitude'] = '';
      $empty_location['country'] = '';
      $empty_location['country_code'] = '';
      $empty_location['region'] = '';
      $empty_location['region_code'] = '';
      $empty_location['city'] = '';
      $empty_location['locality'] = '';
      $empty_location['route'] = '';
      $empty_location['street_number'] = '';
      $empty_location['postal_code'] = '';
      $empty_location['administrative_area_level_1'] = '';
      $empty_location['formatted_address'] = '';
      $location['id'] = $existing_location['id'];
      $full_location = array_merge($empty_location, $location);
      ip_geoloc_debug(t('IP Geolocation: updating db with: !location', array('!location' => ip_geoloc_pretty_print($full_location))));
    }
    try {
      $result = drupal_write_record('ip_geoloc', $full_location, $existing_location ? array('id') : array());
    }
    catch (PDOException $e) {
      // May happen when a fields contains illegal characters
      drupal_set_message(check_plain($e->getMessage()), 'error');
      $result = FALSE;
    }
    if ($result === FALSE) {
      drupal_set_message(t('IP Geolocation: could not save location to db: !location', array('!location' => ip_geoloc_pretty_print($full_location))), 'error');
    }
  }
  return $result;
}

/**
 * Outputs an HTML div placeholder into which will be injected a map.
 *
 * The map is centered on the supplied lat,long coordinates.
 * Handy for use in Views.
 *
 * @param
 *   latitude,  string or double, e.g. "-37.87" or -37.87
 * @param
 *   longitude, string or double, e.g. "144.98" or 144.98
 * @param
 *   div_id, id of the div placeholder, can be anything as long as it's unique
 * @param
 *   style, CSS style applied to the div, e.g "height:250px; width:300px"
 */
function ip_geoloc_output_map($latitude, $longitude, $div_id = 'ip-geoloc-map', $style = '', $balloon_text = '') {
  $javascript_files = drupal_add_js();
  if (empty($javascript_files[IP_GEOLOC_GOOGLE_MAPS_GPS])) {
    // Only add non-GPS version when GPS version is already there
    drupal_add_js(IP_GEOLOC_GOOGLE_MAPS, array('group' => JS_LIBRARY));
  }
  drupal_add_js(drupal_get_path('module', 'ip_geoloc') . '/js/ip_geoloc_gmap.js');

  $script_code = "displayGMap($latitude, $longitude, '$div_id', '$balloon_text');";
  drupal_add_js($script_code, array('type' => 'inline', 'scope' => 'footer'));

  $map_placeholder = "<div id='$div_id'" . (empty($style) ? '' : " style='$style'") . '></div>';
  return $map_placeholder;
}

/**
 * Outputs an HTML div placeholder into which is injected a map.
 *
 * The map is centered on the visitor's current location and features a position
 * marker, which when clicked reveals latitude and longitude, as well as the
 * street address and the accuracy of the position shown.
 *
 * @param
 *   div_id, id of the div placeholder, can be anything as long as it's unique
 * @param
 *   map_options, as a JSON string e.g '{"mapTypeId":"roadmap", "zoom":15}'
 * @param
 *   map_style, CSS style applied to the div, e.g "height:200px; width:300px"
 */
function ip_geoloc_output_map_current_location($div_id = 'ip-geoloc-map-current-location', $map_options = NULL, $map_style = NULL) {
  $js_options = array('group' => JS_LIBRARY);
  drupal_add_js(IP_GEOLOC_GOOGLE_GEARS_INIT, $js_options);
  drupal_add_js(IP_GEOLOC_GEO_JS, $js_options);
  drupal_add_js(IP_GEOLOC_GOOGLE_MAPS_GPS, $js_options);
  drupal_add_js(drupal_get_path('module', 'ip_geoloc') . '/js/ip_geoloc_gmap_current_loc.js');

  if (!isset($map_options)) {
    $map_options = IP_GEOLOC_CURRENT_VISITOR_MAP_OPTIONS;
  }
  if (!isset($map_style)) {
    $map_style = IP_GEOLOC_MAP_DIV_DEFAULT_STYLE;
  }
  drupal_add_js(array('ip_geoloc_current_location_map_div' => $div_id), 'setting');
  drupal_add_js(array('ip_geoloc_current_location_map_options' => drupal_json_decode($map_options)), 'setting');

  $map_placeholder = "<div id='$div_id'" . (empty($map_style) ? '' : " style='$map_style'") . '></div>';
  return $map_placeholder;
}

/**
 * Outputs an HTML div placeholder into which will be injected a map.
 *
 * The locations to be mapped are supplied as an array of lat,long coordinates.
 *
 * @param
 *   $locations, array of location objects each containing lat/long pair
 *   and optionally address, visit count and last visit to appear when the
 *   marker is clicked
 * @param
 *   $div_id, id of the div placeholder, can be anything as long as it's unique
 * @param
 *   $map_options, as a JSON string, .e.g '{"mapTypeId":"roadmap", "zoom":15}'
 * @param
 *   $map_style, CSS style applied to the div, e.g "height:250px; width:300px"
 */
function ip_geoloc_output_map_multi_locations($locations, $div_id = 'ip-geoloc-map-multi-locations', $map_options = NULL, $map_style = NULL) {
  $javascript_files = drupal_add_js();
  if (empty($javascript_files[IP_GEOLOC_GOOGLE_MAPS_GPS])) {
    // Only add non-GPS version when GPS version is already there
    drupal_add_js(IP_GEOLOC_GOOGLE_MAPS, array('group' => JS_LIBRARY));
  }
  drupal_add_js(drupal_get_path('module', 'ip_geoloc') . '/js/ip_geoloc_gmap_multi_loc.js');
  if (!isset($map_options)) {
    $map_options = IP_GEOLOC_RECENT_VISITORS_MAP_OPTIONS;
  }
  if (!isset($map_style)) {
    $map_style = IP_GEOLOC_MAP_DIV_DEFAULT_STYLE;
  }
  drupal_add_js(array('ip_geoloc_locations' => $locations), 'setting');
  drupal_add_js(array('ip_geoloc_multi_location_map_div' => $div_id), 'setting');
  drupal_add_js(array('ip_geoloc_multi_location_map_options' => drupal_json_decode($map_options)), 'setting');

  $map_placeholder = "<div id='$div_id'" . (empty($map_style) ? '' : " style='$map_style'") . '></div>';
  return $map_placeholder;
}

/**
 * Places in $_POST the lat/long and accuracy of the visitor's current location.
 *
 * Also outputs the formatted address (a single string) reverse-geocoded from
 * the lat/long.
 *
 * Note: this is done via an asynchronous javascript call, so the result is not
 * immediately available on return from this function, hence the $menu_callback.
 * Upon page load the included javascript will, when ready, instigate an AJAX
 * call to the $menu_callback, which should invoke a function to retrieve the
 * lat/long and address values from the $_POST variable.
 * See ip_geoloc_current_location_ajax_recipient() for an example.
 */
function ip_geoloc_get_current_location($menu_callback = 'ip-geoloc-current-location') {
  $js_options = array('group' => JS_LIBRARY);
  drupal_add_js(IP_GEOLOC_GOOGLE_GEARS_INIT, $js_options);
  drupal_add_js(IP_GEOLOC_GEO_JS, $js_options);
  drupal_add_js(IP_GEOLOC_GOOGLE_MAPS_GPS, $js_options);
  drupal_add_js(drupal_get_path('module', 'ip_geoloc') . '/js/ip_geoloc_current_location.js');

  drupal_add_js(array('ip_geoloc_menu_callback' => $menu_callback), 'setting');
}

/**
 * Returns the location details associated with the supplied IP address.
 *
 * Performs a lookup in IP Geolocation's own database to see if the supplied IP
 * address has visited already and if so returns their location details (as an
 * array). If the IP address is not yet in the IP Geolocation database, then
 * retrieve lat/long using either Smart IP or GeoIP API (if enabled) and
 * reverse-geocode the lat/long (if the Google Maps service is enabled) into a
 * location. If the second argument is TRUE, then store the new location.
 *
 * @param
 *   $ip_address; note if you're looking for location data for the current
 *   visitor look in $_SESSION['ip_geoloc']['location']
 * @param
 *   $resample, if set to TRUE and the supplied IP address is already on
 *   the db, ignore the existing location data and retrieve the latest
 * @param
 *   $store, if TRUE, store the new or resampled location on the db
 * @param
 *   $reverse_geocode, applies only when the supplied IP address is not
 *   yet on the database or $resample=TRUE; use TRUE, FALSE or NULL; TRUE will
 *   produce greater detail in the location returned; if NULL or omitted the
 *   value is taken from the tick box on the IP Geolocation configuration page
 * @return
 *   location as an array
 */
function ip_geoloc_get_location_by_ip($ip_address, $resample = FALSE, $store = FALSE, $reverse_geocode = NULL) {
  $location = $resample ? NULL : db_query('SELECT * FROM {ip_geoloc} WHERE ip_address = :ip_address', array(':ip_address' => $ip_address))->fetchAssoc();
  if (empty($location)) {
    $location = array('ip_address' => $ip_address);
    if (ip_geoloc_use_smart_ip_if_enabled($location) || ip_geoloc_use_geoip_api_if_enabled($location)) {
      if (!isset($reverse_geocode)) {
        $reverse_geocode = variable_get('ip_geoloc_google_to_reverse_geocode', TRUE);
      }
      if ($reverse_geocode && isset($location['latitude']) && isset($location['longitude'])) {
        if ($google_address = ip_geoloc_reverse_geocode($location['latitude'], $location['longitude'])) {
          // Should we clear out whatever Smart IP or GeoIP put in the $location
          // to avoid fields contradicting eachother? Eg. Google normally
          // returns 'locality', whereas Smart IP and GeoIP return 'city'.
          // $location = array('ip_address' => $ip_address);
          ip_geoloc_flatten_google_address($google_address, $location);
        }
      }
      if ($store) {
        ip_geoloc_store_location($location);
      }
      else {
        drupal_alter('get_ip_geolocation', $location);
      }
    }
  }
  return $location;
}

/**
 * Returns the formatted address reverse-geocoded from the supplied lat,long.
 *
 * See the CALLER BEWARE note at ip_geoloc_reverse_geocode().
 *
 * @param
 *   latitude,  string or double, e.g. "-37.87" or -37.87
 * @param
 *   longitude, string or double, e.g. "144.98" or 144.98
 */
function ip_geoloc_get_address($latitude, $longitude) {
  return $google_address = ip_geoloc_reverse_geocode($latitidue, $longitude)
    ? $google_address['formatted_address'] : '';
}

/**
 * Uses the Google webservice to retrieve address information based on lat/long.
 *
 * Effectively makes calls of this form:
 * http://maps.googleapis.com/maps/api/geocode/json?sensor=false&latlng=-37,144
 *
 * CALLER BEWARE:
 * This is a server-side call. If you want to call this function repeatedly,
 * remeber that Google imposes a limit of 2500 calls per 24hr. It may return an
 * OVER_QUERY_LIMIT response.
 *
 * @param
 *   latitude,  string or double, e.g. "-37.87" or -37.87
 * @param
 *   longitude, string or double, e.g. "144.98" or 144.98
 */
function ip_geoloc_reverse_geocode($latitude, $longitude) {
  if (empty($latitude) || empty($latitude)) {
    drupal_set_message(t('IP Geolocation: cannot reverse-geocode to address as no lat/long was specified.'), 'warning');
    return FALSE;
  }
  $query_start = microtime(TRUE);
  $url = IP_GEOLOC_GOOGLE_MAPS_SERVER . "&latlng=$latitude,$longitude";
  $response = drupal_http_request($url);
  if (!empty($response->error)) {
    $msg_args = array('%url' => $url, '@code' => $response->code, '%error' => $response->error);
    drupal_set_message(t('IP Geolocation: the HTTP request %url returned the following error (code @code): %error.', $msg_args), 'error');
    watchdog('IP Geolocation', 'Error (code @code): %error. Request: %url', $msg_args, WATCHDOG_ERROR);
    return FALSE;
  }
  $data = drupal_json_decode($response->data);
  if ($data['status'] == 'OVER_QUERY_LIMIT') {
    $msg_args = array('%url' => $url);
    if (user_access('administer site configuration')) {
      drupal_set_message(t('IP Geolocation: Server is over its query limit. Request: %url', $msg_args), 'error');
    }
    watchdog('IP Geolocation', 'Server is over its query limit. Request: %url', $msg_args, WATCHDOG_ERROR);
    return FALSE;
  }
  if ($data['status'] == 'ZERO_RESULTS' || !isset($data['results'][0])) {
    $msg_args = array('@protocol' => $response->protocol, '%url' => $url);
    drupal_set_message(t('IP Geolocation: the @protocol request %url succeeded, but returned no results.', $msg_args), 'warning');
    watchdog('IP Geolocation', 'No results from @protocol request %url.', $msg_args, WATCHDOG_WARNING);
    return FALSE;
  }
  if ($data['status'] != 'OK') {
    $msg_args = array('%url' => $url, '%error' => $data['status']);
    drupal_set_message(t('IP Geolocation: unknown error %error. Request: %url..', $msg_args), 'error');
    watchdog('IP Geolocation', 'Unknown error %error. Request: %url.', $msg_args, WATCHDOG_ERROR);
    return FALSE;
  }
  $google_address = $data['results'][0];
  if (empty($google_address['formatted_address'])) {
    $msg_args = array('@lat' => $latitude, '@long' => $longitude);
    ip_geoloc_debug(t('IP Geolocation: (@lat, @long) could not be reverse-geocoded to a street address.', $msg_args), 'warning');
    watchdog('IP Geolocation', '(@lat, @long) could not be reverse-geocoded to a street address..', $msg_args, WATCHDOG_WARNING);
  }
  else {
    $sec = number_format(microtime(TRUE) - $query_start, 1);
    $msg_args = array(
      '@lat' => $latitude, '@long' => $longitude,
      '%sec' => $sec,
      '%address' => $google_address['formatted_address']
    );
    ip_geoloc_debug(t('IP Geolocation: %address reverse-geocoded from (@lat, @long) in %sec s.', $msg_args));
    watchdog('IP Geolocation', '%address reverse-geocoded from (@lat, @long) in %sec s.', $msg_args, WATCHDOG_INFO);
  }
  return $google_address;
}

/**
 * Returns the distance (in meters) between two points on the earth's surface.
 *
 * The points are defined by their lat/long coordinates. If the second point is
 * omitted, the current visitor's location is used, as taken from their session
 * data.
 *
 * @param
 *   array $location1, must contain 'latitude' and 'longitude' keys and values
 * @param
 *   $location2, if an array, must contain 'latitude' and 'longitude' keys
 *     and values, otherwise defaults to $_SESSION['ip_geoloc']['location']
 * @return
 *   float, distance in meters.
 */
function ip_geoloc_distance($location, $ref_location = 'current visitor') {
  if (!is_array($ref_location) && !empty($_SESSION) && is_array($_SESSION['ip_geoloc']['location'])) {
    $ref_location = $_SESSION['ip_geoloc']['location'];
  }
  if (is_numeric($location['longitude']) && is_numeric($location['latitude']) && is_numeric($ref_location['longitude']) && is_numeric($ref_location['latitude'])) {
    return ip_geoloc_earth_distance($location['longitude'], $location['latitude'], $ref_location['longitude'], $ref_location['latitude']);
  }
  return '?';
}

/**
 * Returns the distance between two points on the earth's surface.
 *
 * The points are defined by their lat/long coordinates.
 *
 * Gratefully copied from the http://drupal.org/project/location module, thus
 * ensuring compatibility of results.
 *
 *  @param
 *    longitude1
 *  @param
 *    latitude1
 *  @param
 *    longitude2
 *  @param
 *    latitude2
 *  @return
 *   distance in meters
 *
 *  @see http://en.wikipedia.org/wiki/Great-circle_distance
 *    This article appears to use a slightly different algorithm.
 */
function ip_geoloc_earth_distance($longitude1, $latitude1, $longitude2, $latitude2) {
  $long1 = deg2rad($longitude1);
  $lat1  = deg2rad($latitude1);
  $long2 = deg2rad($longitude2);
  $lat2  = deg2rad($latitude2);
  $radius = ip_geoloc_earth_radius(($latitude1 + $latitude2) / 2);
  $cosangle = cos($lat1)*cos($lat2) * (cos($long1)*cos($long2) + sin($long1)*sin($long2)) + sin($lat1)*sin($lat2);
  return acos($cosangle) * $radius;
}

function ip_geoloc_earth_radius($latitude) {
  $lat = deg2rad($latitude);
  $x = cos($lat)/ip_geoloc_earth_radius_semimajor();
  $y = sin($lat)/ip_geoloc_earth_radius_semiminor();
  return 1.0 / (sqrt($x*$x + $y*$y));
}

function ip_geoloc_earth_radius_semimajor() {
  return 6378137.0;
}

function ip_geoloc_earth_radius_semiminor() {
  return (ip_geoloc_earth_radius_semimajor() * (1.0 - ip_geoloc_earth_flattening()));
}

function ip_geoloc_earth_flattening() {
  return (1.0/298.257223563);
}
