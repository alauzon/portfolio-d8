<?php
 /**
  * This template is used to output a map of marker locations in a view.
  *
  * Variables available:
  * - $view: the view object, if needed
  * - $locations: array of locations with lat/long coordinates and balloon texts
  * - $div_id: id of the div in which the map will be injected
  * - $map_options
  * - $map_style: CSS style string, like "height: 200px; width: 500px"
  */
?>
<div class="ip-geoloc-map">
  <?php print ip_geoloc_output_map_multi_locations($locations, $div_id, $map_options, $map_style); ?>
</div>