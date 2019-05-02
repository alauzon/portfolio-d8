<?php

/**
 * @file device-geolocation-visitor-info.tpl.php
 * Default theme implementation for rendering user's geolocation details block.
 *
 * Available variables:
 * - $location: An associative array with possible array items:
 * -- latitude: May came from Smart IP or W3C Geolocation API or Google Gears
 * -- longitude: May came from Smart IP or W3C Geolocation API or Google Gears
 * -- street_number: Google Geocoder service specific item
 * -- postal_code: Google Geocoder service specific item
 * -- route: Google Geocoder service specific item
 * -- neighborhood: Google Geocoder service specific item
 * -- locality: Google Geocoder service specific item
 * -- sublocality: Google Geocoder service specific item
 * -- establishment: Google Geocoder service specific item
 * -- administrative_area_level_N: Google Geocoder service specific item
 * -- country: May came from Smart IP or Google Geocoder service
 * -- country_code: May came from Smart IP or Google Geocoder service
 * -- zip: Smart IP specific item
 * -- region: Smart IP specific item
 * -- region_code: Smart IP specific item
 * -- timestamp: Timestamp of these data stored
 *
 * @ingroup themeable
 */
?>
<?php if (!empty($location)): ?>
  <dl>
    <?php foreach ($location as $item => $value): ?>
      <?php if (!empty($value) && $item != 'region_code' && $item != 'timestamp'): ?>
        <?php
          $item = str_replace('_', ' ', $item);
          $item[0] = strtoupper($item[0]);
        ?>
        <dt><?php print $item; ?></dt>
        <dd><?php print $value; ?></dd>
      <?php endif; ?>
    <?php endforeach; ?>
  </dl>
<?php endif; ?>