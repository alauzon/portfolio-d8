<?php

function _views_graphapi_style_build_graph_data(&$vars) {
  $view = $vars["view"];
  $rows = $vars["rows"];
  $options = $vars["options"];

  $base = $view->base_table;
  if ($base == 'system') {
    // We cannot use system_rebuild_module_data as this calls build_dependencies
    // which adds transitive dependencies which we don't want.
    // @see system_rebuild_module_data()
    $modules = _system_rebuild_module_data();
    ksort($modules);
    system_get_files_database($modules, 'module');

    // Weird dry alert (some modules declare dependency on a theme
    $themes = system_rebuild_theme_data();
    foreach ($themes as $theme => $info) {
      $themes[$theme]->info['dependencies'] = array();
      if (isset($theme->info['base theme'])) {
        $themes[$theme]->info['dependencies'][] = $theme->info['base theme'];
      }
    }
    $modules += $themes;

    $graph = graphapi_new_graph();
    foreach ($view->result as $id => $row) {
      //if ($id > 20) break;
      $from_id = $row->system_name;
      $extra = $modules[$from_id];

      if ($extra->status) {
        $color = 'green';
      }
      else {
        $color = 'yellow';
      }
      if (!isset($extra->info['hidden']) || !$extra->info['hidden']) {
        graphapi_set_node_data($graph, $from_id, array('background-color' => $color));
        foreach ($extra->info['dependencies'] as $to_id) {
          // We need to cleanup the dependency. @see drupal_parse_dependency()
          $parts = explode('(', $to_id, 2);
          $to_id = trim($parts[0]);
          if (!isset($modules[$to_id])) {
            graphapi_set_node_data($graph, $to_id, array('background-color' => 'red'));
          }
          graphapi_add_link($graph, $from_id, $to_id);
        }
      }
    }
  }
  else {
    $view = $vars['view'];
    $options = $vars['options'];
    // We need to check for the correct field labels
    $handler = $view->display_handler->get_handlers('field');

    //$fields   = &$view->field;
    //$columns  = $handler->sanitize_columns($options['columns'], $fields);
    //dsm($columns);
    //$vars['xml'] = "Empty testing";
    //return;
    $mapping = array();
    $mapping['from'] = array();
    $mapping['to'] = array();

    foreach ($options['mapping']['from'] as $id => $data) {
      if (isset($options['mapping']['from'][$id]) && isset($view->field[$options['mapping']['from'][$id]])) {
        $mapping['from'][$id] = $view->field[$options['mapping']['from'][$id]]->field_alias;
      }
    }
    foreach ($options['mapping']['to'] as $id => $data) {
      if (isset($options['mapping']['to'][$id]) && isset($view->field[$options['mapping']['to'][$id]])) {
        $mapping['to'][$id] = $view->field[$options['mapping']['to'][$id]]->field_alias;
      }
    }
    // Fix label field if not set yet
    if (empty($mapping['from']['label'])) {
      $mapping['from']['label'] = $mapping['from']['id'];
    }
    if (empty($mapping['to']['label'])) {
      $mapping['to']['label'] = $mapping['to']['id'];
    }

    //merlinofchaos>	ClemensTolboom: $view->field[$field_id]->field_alias should then match 'comment_node_cid'.
    //merlinofchaos>	ClemensTolboom: Though in general you should try to avoid ever referencing what's in $row directly if you can.
    //merlinofchaos>	ClemensTolboom: There's always things like $view->style_plugin->rendered_fields[] and $view->field[$id]->get_value()
    $graph = graphapi_new_graph();
    foreach ($view->result as $id => $row) {
      // Node values
      $from_id = $row->{$mapping['from']['id']};
      $from_title = $row->{$mapping['from']['label']};
      $to_id = $row->{$mapping['to']['id']};
      $to_title = $row->{$mapping['to']['label']};

      graphapi_add_node($graph, $from_id);
      graphapi_set_node_title($graph, $from_id, $from_title);
      // Make sure only add a link when exists
      if (isset($to_id)) {
        graphapi_add_node($graph, $to_id);
        graphapi_set_node_title($graph, $to_id, $to_title);

        graphapi_add_link($graph, $from_id, $to_id);
      }
    }
  }
  $vars['graph'] = $graph;
  $vars['graph-config'] = $vars['settings'];
}
