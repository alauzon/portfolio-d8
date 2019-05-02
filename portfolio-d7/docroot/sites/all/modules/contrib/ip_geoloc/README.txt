
IP GEOLOCATION
==============
This documentation concentrates on the installation and configuration of the
IP Geolocation module. A full description of the module and its features can be
found at http://drupal.org/project/ip_geoloc.

INSTALLATION & CONFIGURATION
============================

A. Present and future: reporting and mapping of location information about
guests visiting after you enabled IP Geolocation

1. Install and enable like any other module, use Drush if you wish. Remain
connected to the internet.

2. Make sure the core Statistics module is enabled. Then at Configuration >>
Statistics, section System, verify that the access log is enabled. Select the
"Discard access logs older than" option as you please. "Never" is good.

3. Visit the IP Geolocation configuration page at Configuration >> IP
Geolocation. If you don't see any errors or warnings (usually yellow) you're
good to proceed. Don't worry about any of the configuration options for now,
the defaults are fine.

4. At Structure >> Blocks put the block "Map showing locations of 10 most recent
visitors" in the content region of all or a selected page. View the page. That
marker represents you (or your service provider). Clicking the marker reveals
more details.

5. Enable the Views and Views PHP (http://drupal.org/project/views_php) modules.
Then have a look at Structure >> Views for a couple of handy Views, e.g. the
"Visitor log", which shows for each IP address that visited, its street address,
as well as a local map. Or "Visitor log (lite)", which combines nicely when put
on the same page with the "Map showing locations of 10 most recent visitors".
Modify these views as you please.


B. Historic data: location info about visits to your site from way back when

Note, that this step relies on you having had the Statistics module enabled
before you installed IP Geolocation, as the access log is used as the source of
IP addresses that have visited your site previously.
There are a couple of options here. Use either http://drupal.org/project/smart_ip
and the IPinfoDB web service it uses, or http://drupal.org/project/geoip, which
takes its data from a file you download for free.

1a. If you decide to employ Smart IP....
Install and enable Smart IP. There is no need to enable the Device Geolocation
submodule as IP Geolocation already has that funtionality, plus more. At
Configuration >> Smart IP you'll find two options to upload historic lat/long
data. The one using the IPinfoDB web service is probably the quickest. The API
key required on the Smart IP configuration page is free and is sent to you
immediately by return email after you have filled out the short form at
http://ipinfodb.com/register.php. On the Smart IP configuration page perform an
IP lookup to verify that the key is correct.

1b. If you decide to employ GeoIP instead of Smart IP...
Download and enable the module. Then download and uncompress
http://geolite.maxmind.com/download/geoip/database/GeoLiteCity.dat.gz and place
it in sites/all/libraries/geoip. Go to the GeoIP configuration page and type
the name of the file you've just downloaded, GeoLiteCity.dat. Save. That should
be it.

2. With either Smart IP or GeoIP configured, visit Configuration >> IP Geolocation.
Tick the check boxes as appropriate.

3. On the same page, start a small batch import of, say, size 10. Data for the
most recent visitors will be loaded first, so you don't have to complete the
import to check it's all working. For instance, the block "Map showing locations
of 10 most recent visitors" should now show more markers.

4. Go back the Configuration >> IP Geolocation and complete the import process
with a larger batch size until the IP Geolocation database is up to date with
the access log. It will automatically remain in synch from now on.


FOR PROGRAMMERS
===============
First of all, check out file ip_geoloc_api.inc for a number of useful utility
functions for creating maps and markers, calculating distances between locations
etc. All functions are documented and should be straightforward to use.

Secondly, if you want to hook your own gelocation data provider into IP
Geolocation, then you can -- it's simple.
In your module, let's call it MYMODULE, all you have to do is flesh out the
following function.

<?php
  /*
   *  Implements hook_get_ip_geolocation_alter().
   */
  function MYMODULE_get_ip_geolocation_alter(&$location) {

    if (empty($location['ip_address'])) {
      return;
    }
    // ... your code here to retrieve geolocation data ...

    $location['provider'] = 'MYMODULE';

    // Then fill out some or all of the location fields that IP Geolocation
    // knows how to store.
    $location['latitude'] =  ....;
    $location['longitude'] = ....;
    $location['country'] = ....;
    $location['country_code'] = ....;
    $location['region'] = ....;
    $location['region_code'] = ....;
    $location['city'] = ... ;
    $location['locality'] = ....; // eg suburb
    $location['route'] = ....;     // eg street
    $location['street_number'] = ....;
    $location['postal_code'] = ....; // eg ZIP
    $location['administrative_area_level_1'] = ....; // eg state or province
    $location['formatted_address'] = ....; // address as a human-readible string
  }
?>

That's all!
Note that when IP Geolocation calls this function the $location object may be
partially fleshed out. If $location['ip_address'] is empty, this means that
IP Geolocation is still waiting for more details to arrive from the Google
reverse-geocoding AJAX call. If $location['ip_address'] is not empty, then IP
Geolocation does not expect any further details and will store the $location
with your modifications (if any) on the IP Geolocation database. You must set
$location['formatted_address'] in order for the location to be stored.


RESTRICTIONS IMPOSED BY GOOGLE
==============================
Taken from http://code.google.com/apis/maps/documentation/geocoding :

"Use of the Google Geocoding API is subject to a query limit of 2500 geolocation
requests per day. (Users of Google Maps API Premier may perform up to 100,000
requests per day.) This limit is enforced to prevent abuse and/or repurposing of
the Geocoding API, and this limit may be changed in the future without notice.
Additionally, we enforce a request rate limit to prevent abuse of the service.
If you exceed the 24-hour limit or otherwise abuse the service, the Geocoding
API may stop working for you temporarily. If you continue to exceed this limit,
your access to the Geocoding API may be blocked.

Note: the Geocoding API may only be used in conjunction with a Google map;
geocoding results without displaying them on a map is prohibited. For complete
details on allowed usage, consult the Maps API Terms of Service License
Restrictions."

AUTHOR
======
Rik de Boer of flink dot com dot au, Melbourne, Australia.
