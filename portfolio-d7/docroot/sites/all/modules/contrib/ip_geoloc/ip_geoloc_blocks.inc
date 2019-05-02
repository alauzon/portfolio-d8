<?php

/**
 * @file
 * Blocks available in IP Geolocation.
 */

/**
 * Implements hook_block_info().
 */
function ip_geoloc_block_info() {
  $blocks['current_visitor_map'] = array(
    'info' => t("Map centered on current visitor's location"),
    'cache' => DRUPAL_NO_CACHE
  );
  $blocks['recent_visitors_map'] = array(
    'info' => t('Map showing locations of @n most recent visitors', array('@n' => variable_get('ip_geoloc_recent_visitors_map_number_of_visitors', 10))),
    'cache' => DRUPAL_NO_CACHE
  );
  $blocks['map_of_view'] = array(
    'info' =>  t('Map of view'),
    'cache' => DRUPAL_NO_CACHE
  );
  if (!module_exists('views')) {
    $blocks['map_of_view']['info'] .= '<em> ('. t('requires Views module') . ')</em>';
  }
  $blocks['address_lookup'] = array(
    'info' => t('Address lookup based on IP'),
    'cache' => DRUPAL_NO_CACHE
  );
  return $blocks;
}

/**
 * Implements hook_block_configure().
 */
function ip_geoloc_block_configure($delta = '') {
  $form = array();
  if ($delta == 'address_lookup') {
    return $form;
  }
  if ($delta == 'recent_visitors_map') {
    $form['ip_geoloc_recent_visitors_map_number_of_visitors'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of recent visitor locations to show on the map.'),
      '#default_value' => variable_get('ip_geoloc_recent_visitors_map_number_of_visitors', 10),
      '#description' => t('Must be greater than 0. Values greater than 25 are not recommended.')
    );
  }
  $form['ip_geoloc_' . $delta . '_div_style'] = array(
    '#type' => 'textfield',
    '#title' => t('CSS style attribute(s) applied to the HTML DIV element that is placeholding the map'),
    '#default_value' => variable_get('ip_geoloc_' . $delta . '_div_style', IP_GEOLOC_MAP_DIV_DEFAULT_STYLE),
    '#description' => t('The default %default_style results in a map of 300 pixels high, with a width bounded by the element that contains it. Do not enter quotes or equal signs.',
      array('%default_style' => IP_GEOLOC_MAP_DIV_DEFAULT_STYLE))
  );

  switch ($delta) {

    case 'current_visitor_map':
      $form['ip_geoloc_current_visitor_map_options'] = array(
        '#type' => 'textfield',
        '#size' => 120,
        '#title' => t('Map options'),
        '#default_value' => variable_get('ip_geoloc_current_visitor_map_options', IP_GEOLOC_CURRENT_VISITOR_MAP_OPTIONS),
        '#description' => t("The default %default_options produces a street map zoomed in to level 15. A list of map options can be found <a href='!google_map_docs'>here</a>. Remember to separate options with comma's, not semi-colons, and make sure your quotes match.",
          array('%default_options' => IP_GEOLOC_CURRENT_VISITOR_MAP_OPTIONS,
                '!google_map_docs' => DOC_GOOGLE_MAP_OPTIONS))
      );
      break;

    case 'recent_visitors_map':
      $form['ip_geoloc_recent_visitors_map_options'] = array(
        '#type' => 'textfield',
        '#size' => 120,
        '#title' => t('Map options'),
        '#default_value' => variable_get('ip_geoloc_recent_visitors_map_options', IP_GEOLOC_RECENT_VISITORS_MAP_OPTIONS),
        '#description' => t("The default %default_options produces a world map zoomed in to level 2. A list of map options can be found <a href='!google_map_docs'>here</a>. Remember to separate options with comma's, not semi-colons, and make sure your quotes match.",
          array('%default_options' => IP_GEOLOC_RECENT_VISITORS_MAP_OPTIONS,
                '!google_map_docs' => DOC_GOOGLE_MAP_OPTIONS))
      );
      break;

    case 'map_of_view':
      $views = array_merge(
        array('' => t('- None -')),
        ip_geoloc_get_names_of_views_suitable_for_maps()
      );
      asort($views);
      $form['ip_geoloc_view_for_map'] = array(
        '#type' => 'select',
        '#title' => t('Choose a view to populate the map appearing in this block'),
        '#default_value' => variable_get('ip_geoloc_view_for_map'),
        '#options' => $views,
        '#description' => t('The view must output fields labeled <strong>latitude</strong> and <strong>longitude</strong>.<br/>Remaining fields in the view will be added to pop up in balloons when map markers are clicked.')
      );
      $form['ip_geoloc_map_of_view_options'] = array(
        '#type' => 'textfield',
        '#size' => 120,
        '#title' => t('Map options'),
        '#default_value' => variable_get('ip_geoloc_map_of_view_options', IP_GEOLOC_RECENT_VISITORS_MAP_OPTIONS),
        '#description' => t("The default %default_options produces a world map zoomed in to level 2. A list of map options can be found <a href='!google_map_docs'>here</a>. Remember to separate options with comma's, not semi-colons, and make sure your quotes match.",
          array('%default_options' => IP_GEOLOC_RECENT_VISITORS_MAP_OPTIONS,
                '!google_map_docs' => DOC_GOOGLE_MAP_OPTIONS))
      );
      break;
  }
  return $form;
}

/**
 * Implements hook_block_save().
 */
function ip_geoloc_block_save($delta = '', $edit = array()) {
  if ($delta == 'address_lookup') {
    return;
  }
  // Save options common to all maps
  variable_set('ip_geoloc_' . $delta . '_div_style', $edit['ip_geoloc_' . $delta . '_div_style']);
  ip_geoloc_save_map_options('ip_geoloc_' . $delta . '_options', $edit['ip_geoloc_' . $delta . '_options']);

  switch ($delta) {

    case 'recent_visitors_map':
      variable_set('ip_geoloc_recent_visitors_map_number_of_visitors', $edit['ip_geoloc_recent_visitors_map_number_of_visitors']);
      break;

    case 'map_of_view':
      variable_set('ip_geoloc_view_for_map', $edit['ip_geoloc_view_for_map']);
      break;
  }
}

/**
 * Implements hook_block_view().
 */
function ip_geoloc_block_view($delta = '') {
  $block = array();
  $map_style = variable_get('ip_geoloc_' . $delta . '_div_style', IP_GEOLOC_MAP_DIV_DEFAULT_STYLE);

  switch ($delta) {

    case 'current_visitor_map':
      $block['subject'] = t('Your location');
      $map_options = variable_get('ip_geoloc_current_visitor_map_options', IP_GEOLOC_CURRENT_VISITOR_MAP_OPTIONS);
      // @todo theme this
      //$block['content'] = ip_geoloc_output_map_current_location('ip-geoloc-block-current-visitor-map', $map_options, $map_style);
      $block['content'] = theme(array('ip_geoloc_map_current_visitor'), array(
        'div_id' => 'ip-geoloc-block-current-visitor-map',
        'map_options' => $map_options,
        'map_style' => $map_style
      ));
      break;

    case 'recent_visitors_map':
      $block['subject'] = t('Recent visitors');
      $how_many = variable_get('ip_geoloc_recent_visitors_map_number_of_visitors', 10);
      if ($how_many > 0) {
        $locations = ip_geoloc_get_recent_visitor_locations($how_many);
        $map_options = variable_get('ip_geoloc_recent_visitors_map_options', IP_GEOLOC_RECENT_VISITORS_MAP_OPTIONS);
        $block['content'] = theme(array('ip_geoloc_map'), array(
          'locations' => $locations,
          'div_id' => 'ip-geoloc-block-recent-visitors-map',
          'map_options' => $map_options,
          'map_style' => $map_style
        ));
      }
      break;

    case 'map_of_view':
      if (module_exists('views')) {
        $locations = ip_geoloc_get_view_output(variable_get('ip_geoloc_view_for_map'));
        $map_options = variable_get('ip_geoloc_recent_map_of_view_options', IP_GEOLOC_RECENT_VISITORS_MAP_OPTIONS);
        $block['content'] = theme(array('ip_geoloc_map'), array(
          'locations' => $locations,
          'div_id' => 'ip-geoloc-block-map-of-view',
          'map_options' => $map_options,
          'map_style' => $map_style
        ));
      }
      else {
        $block['content'] = t('Views module must be enabled to show map of a view.');
      }
      break;

    case 'address_lookup':
      $block['subject'] = t('Global address lookup');
      $block['content'] = drupal_get_form('ip_geoloc_address_lookup');
      break;
  }
  return $block;
}

function ip_geoloc_save_map_options($var_name, $map_options) {
  $map_options_decoded = drupal_json_decode($map_options);
  if ($map_options_decoded == NULL) {
    drupal_set_message(t("Sytax error in map options. These map options may not work: '%map_options'", array('%map_options' => $map_options)), 'warning');
  }
  variable_set($var_name, $map_options);
}

function ip_geoloc_get_recent_visitor_locations($how_many) {
  $locations = array();
  if (db_table_exists('accesslog')) {
    // A LEFT JOIN would also pick up new IP addresses that are about to be
    // inserted into the {accesslog}.
    // However a LEFT JOIN in this query can easily make it 100 times slower
    // than the INNER JOIN used below and would only be relevant for the very
    // first click from a new IP address or in the case where the IP address was
    // removed from the {accesslog}.
    $result = db_query_range('SELECT DISTINCT ip_address, latitude, longitude, formatted_address, COUNT(a.timestamp) AS visit_count, MAX(a.timestamp) AS last_visit FROM {ip_geoloc} i INNER JOIN {accesslog} a ON i.ip_address = a.hostname GROUP BY i.ip_address ORDER BY last_visit DESC', 0, $how_many);
    foreach ($result as $location) {
      // Prevent older IP address locations overwriting the latest location.
      if (!isset($locations[$location->ip_address])) {
        $loc_rendered = new stdClass();
        $loc_rendered->latitude = $location->latitude;
        $loc_rendered->longitude = $location->longitude;
        $loc_rendered->balloonText = t('IP address') . ' ' . $location->ip_address . '<br/>'
         . $location->formatted_address . '<br/>'
         . t('#visits') . ' ' . (empty($location->visit_count) ? '?' : $location->visit_count)
         . ', '. t('last visit') . ' ' . (empty($location->last_visit) ? '?' : format_date($location->last_visit, 'short'));
        // $loc-rendered->balloonText = filter_xss($loc_rendered->balloonText);
        $locations[$location->ip_address] = $loc_rendered;
      }
    }
  }
  return $locations;
}

/**
 * Generates a simple form for collecting the IP address to be reverse-geocoded.
 *
 * @ingroup forms
 */
function ip_geoloc_address_lookup($form, &$form_state) {
  $last_visit = isset($form_state['storage']['last_visit']) ? $form_state['storage']['last_visit'] : '';
  $formatted_address = isset($form_state['storage']['formatted_address']) ? $form_state['storage']['formatted_address'] : '';
  if (!empty($last_visit) || !empty($formatted_address)) {
    $prefix = '<div class="ip_geoloc_address_lookup">' . $formatted_address . (empty($last_visit) ? '' : "<br/>$last_visit") . '</div>';
  }
  $form['ip_address'] = array(
    '#type' => 'textfield',
    '#title' => t('IP address'),
    '#default_value' => '',
    '#size' => 16,
    '#required' => FALSE,
    '#prefix' => isset($prefix) ? $prefix : NULL
  );
  if (user_access('administer site configuration')) {
    $form['store'] = array(
      '#type' => 'checkbox',
      '#title' => t('If found, store on IP Geolocation database for future reference.'),
      '#default_value' => FALSE
    );
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Lookup'),
    '#submit' => array('ip_geoloc_lookup')
  );
  return $form;
}

function ip_geoloc_lookup($form, &$form_state) {
  $store = !empty($form_state['values']['store']);
  $location = ip_geoloc_get_location_by_ip(trim($form_state['values']['ip_address']), TRUE, $store);
  if (db_table_exists('accesslog')) {
    $last_visit = db_query('SELECT MAX(timestamp) FROM {accesslog} WHERE hostname = :ip_address', array(':ip_address' => $location['ip_address']))->fetchField();
    if ($last_visit) {
      $last_visit = format_date($last_visit, 'short');
      $form_state['storage']['last_visit'] = t('<strong>Last visit:</strong> %date', array('%date' => $last_visit));
    }
    else {
      // $form_state['storage']['last_visit'] = t('There is no record of this IP address visiting this site.');
    }
  }
  $form_state['storage']['formatted_address'] = isset($location['formatted_address'])
    ? t('%ip: <strong>%address</strong>', array('%ip' => $location['ip_address'], '%address' => $location['formatted_address']))
    : '';
  $form_state['rebuild'] = TRUE; // to preserve entered values and storage array
}

/**
 * Returns names of all views (whether enabled or disabled) that have
 * 'latitude' and 'longitude' fields, or fields re-labelled as such.
 *
 * @return array of View names, indexed by their machine names.
 */
function ip_geoloc_get_names_of_views_suitable_for_maps() {
  $views = array();
  if (module_exists('views')) {
    $required_field_names = array('latitude', 'longitude');
    foreach (views_get_all_views() as $view) {
      foreach ($required_field_names as $field_name_or_label) {
        $all_found = _ip_geoloc_find_real_field_name($view, $field_name_or_label);
        if (!$all_found) {
          break;
        }
      }
      if ($all_found) {
        $views[$view->name] = empty($view->human_name) ? $view->name : $view->human_name;
      }
    }
  }
  return $views;
}

/**
 * Executes the view by the supplied name and transfers the semie-rendered
 * output to an array of locations ready to be mapped, with marker balloons.
 *
 * @param $view_id
 *
 * @return
 *   array of locations to be passed to ip_geoloc_output_map_multi_locations().
 */
function ip_geoloc_get_view_output($view_id) {

  $locations = array();

  $view = views_get_view($view_id);
  if (!is_object($view)) {
    return $locations;
  }
  $view->init_display();
  $view->pre_execute();
  $view->execute();
  // At this point we'll have access to the raw data via $view->result.
  // However for the marker balloons we want semi-rendered, including token
  // substitutions, but without all the HTML tags around it.
  // After $view->render(), $view->style_plugin->rendered_fields contains what
  // we need.
  $view->render();

  if (!empty($view->style_plugin->rendered_fields)) {
    $latitude_real_field_name  = _ip_geoloc_find_real_field_name($view, 'latitude');
    $longitude_real_field_name = _ip_geoloc_find_real_field_name($view, 'longitude');

    foreach ($view->style_plugin->rendered_fields as $row) {
      $location = new stdClass();
      $location->latitude  = $row[$latitude_real_field_name];
      $location->longitude = $row[$longitude_real_field_name];

      if (!empty($location->latitude) && !empty($location->longitude)) {
        unset($row[$latitude_real_field_name]);
        unset($row[$longitude_real_field_name]);

        // Remaining rendered values on the row go into balloon.
        $location->balloonText = implode('<br/>', array_values($row));

        $locations[] = $location;
      }
    }
  }
  return $locations;
}

function _ip_geoloc_find_real_field_name($view, $field_name_or_label) {
  if (isset($view->display['default']->display_options['fields'])) {
    $fields = $view->display['default']->display_options['fields'];
    $field_names = array();
    foreach ($fields as $field_name => $field) {
      if ($field_name == $field_name_or_label || (isset($field['label']) && $field['label'] == $field_name_or_label)) {
        return $field_name;
      }
    }
  }
  return FALSE;
}
