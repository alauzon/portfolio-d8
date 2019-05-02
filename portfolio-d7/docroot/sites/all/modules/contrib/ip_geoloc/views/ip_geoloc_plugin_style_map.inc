<?php

class ip_geoloc_plugin_style_map extends views_plugin_style {

  /**
   * Set default map options.
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['ip_geoloc_map_of_view_div_style'] = array('default' => '');
    $options['ip_geoloc_map_of_view_options'] = array('default' => '');
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['ip_geoloc_map_of_view_div_style'] = array(
      '#title' => t('Map style (CSS attributes)'),
      '#type' => 'textfield',
      '#size' => 150,
      '#default_value' => $this->options['ip_geoloc_map_of_view_div_style'],
      '#description' => t('If left blank, the default %default_style will result in a map of 300 pixels high, with a width bounded by the element that contains it. Separate style settings with semi-colons. Do not enter quotes or equal signs.',
        array('%default_style' => IP_GEOLOC_MAP_DIV_DEFAULT_STYLE))
    );
    $form['ip_geoloc_map_of_view_options'] = array(
      '#title' => t('Map options'),
      '#type' => 'textfield',
      '#size' => 150,
      '#default_value' => $this->options['ip_geoloc_map_of_view_options'],
      '#description' => t("If left blank, the default %default_options will produce a world map zoomed in to level 2. A list of map options can be found <a href='!google_map_docs'>here</a>. Remember to separate options with comma's, not semi-colons, and make sure your quotes match.", array(
        '%default_options' => IP_GEOLOC_RECENT_VISITORS_MAP_OPTIONS,
        '!google_map_docs' => DOC_GOOGLE_MAP_OPTIONS))
    );
  }

  /**
   * Transform the View result in a list of maker locations and render as a map.
   *
   * @param type $result
   *   This is superfluous, same as $this->view->result.
   *
   */
  function render($result) {

    if (!empty($this->view->live_preview)) {
      return t('The preview function is incompatible with the map format so cannot be used. Please visit the page path or block to see the results.');
    }

    // Do token substitutions, render timestamps etc, all the pre-theming stuff.
    // Exclude lat/long from this.
    unset($this->view->field['latitude']);
    unset($this->view->field['longitude']);
    $this->render_fields($result);

    $locations = array();
    foreach ($result as $i => $row) {
      if (!empty($row->ip_geoloc_latitude) && !empty($row->ip_geoloc_longitude)) {
        $location = new stdClass();
        $location->latitude  = $row->ip_geoloc_latitude;
        $location->longitude = $row->ip_geoloc_longitude;
        if (!empty($this->rendered_fields)) {
          // Remaining row values go into the balloon
          $location->balloonText = implode('<br/>', $this->rendered_fields[$i]);
        }
        $locations[] = $location;
      }
    }
    $map_options = $this->options['ip_geoloc_map_of_view_options'];
    $map_style = $this->options['ip_geoloc_map_of_view_div_style'];

    $output = theme(array('ip_geoloc_map'), array(
      'locations' => $locations,
      'div_id' => 'ip-geoloc-map-of-view',
      'map_options' => empty($map_options) ? IP_GEOLOC_RECENT_VISITORS_MAP_OPTIONS : $map_options,
      'map_style' => empty($map_style) ? IP_GEOLOC_MAP_DIV_DEFAULT_STYLE : $map_style,
      'view' => $this->view
    ));
    return $output;
  }

  /**
   * Perform token replacement, convert timestamps to date strings etc. for
   * the entire result set. Store the rendered rows on the object.
   *
   * @param $result
   *   The result array from $this->view->result
   */
  function render_fields($result) {
    if (!$this->uses_fields()) {
      return;
    }
    if (!isset($this->rendered_fields)) {
      $this->rendered_fields = array();
      $field_ids = array_keys($this->view->field);
      foreach ($result as $i => $row) {
        $this->view->row_index = $i; // God knows why we need this...
        foreach ($field_ids as $field_id) {
          $this->rendered_fields[$i][$field_id] = $this->view->field[$field_id]->theme($row);
        }
        //$this->row_tokens[$i] = $this->view->field[$field_id]->get_render_tokens(array());
      }
      unset($this->view->row_index);
    }
    return $this->rendered_fields;
  }

}
