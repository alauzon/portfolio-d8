<?php

/**
 * @file
 * Implements views_plugin_style for graphapi
 */

/**
 * Implements views_plugin_style
 */
class views_plugin_style_graphapi extends views_plugin_style {

  private $graph_fields = array(
    'from' => array(
      'id' => array(
        'title' => 'unique ID',
        'description' => 'Used as a unique identifier within the graph',
        'required' => true,
      ),
      'label' => array(
        'title' => 'Label to display',
      ),
      'uri' => array(
        'title' => '(tbd) Link to more info',
        'description' => 'Used for popup or link to the detailed information',
      ),
    ),
    'to' => array(
      'id' => array(
        'title' => 'unique ID',
        'description' => 'Used as a unique identifier within the graph',
        'required' => true,
      ),
      'label' => array(
        'title' => 'Label to display',
      ),
      'uri' => array(
        'title' => '(tdb) Link to more info',
        'description' => 'Used for popup or link to the detailed information',
      ),
    ),
  );

  /**
   * Implementation of views_plugin_style::option_definition
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['engine'] = array('default' => 'graphapi');
    foreach ($this->graph_fields as $id => $data) {
      $options['mapping'][$id] = array('default' => NULL);
    }
    return $options;
  }

  /**
   * Provide a form for setting options.
   */
  function options_form(&$form, &$form_state) {
    $view = $form_state['view'];
    // We map system table ourself
    $is_system = $view->base_table == 'system';
    // TODO: next line gives grouping option
    // parent::options_form($form, $form_state);

    $options = $this->options;
    $handlers = $this->display->handler->get_handlers('field');
    if (!$is_system && (empty($handlers) || count($handlers) < 2)) {
      $form['error_markup'] = array(
        '#markup' => '<div class="error messages">' . t('You need at least two field before you can configure your graph settings') . '</div>',
      );
      return;
    }
    $form['engine'] = array(
      '#type' => 'radios',
      '#title' => t('Render type'),
      '#description' => t('The render engine to use. Known render engines are graph_phyz and !thejit. graphviz_filter is supported only in concept.', array('!thejit' => l('thejit', 'http://drupal.org/project/thejit'))),
      '#options' => graphapi_views_formats(),
      '#default_value' => $options['engine'],
    );
    if (!isset($from_state['values'])) {
      $form_state['values'] = $options;
    }
    $form = graphapi_settings_form($form, $form_state);
    if ($is_system) {
      // We are done. No need for columns.
      return $form;
    }
    $field_names = $this->display->handler->get_field_labels();
    $fields = $this->display->handler->get_option('fields');
    array_unshift($field_names, "- none -");

    $form['mapping'] = array(
      '#type' => 'fieldset',
      '#title' => 'Field mapping',
      '#collapsible' => TRUE,
    );

    $form['mapping']['from'] = array(
      '#type' => 'fieldset',
      '#title' => 'From settings',
      '#collapsible' => TRUE,
      '#collapsed' => !empty($options['mapping']['from']['id']),
    );
    foreach ($this->graph_fields['from'] as $id => $info) {
      $form['mapping']['from'][$id] = array(
        '#type' => 'select',
        '#required' => isset($info['required']) && $info['required'],
        '#title' => t($info['title']),
        '#description' => isset($info['description']) ? $info['description'] : '',
        '#options' => $field_names,
        '#default_value' => $options['mapping']['from'][$id],
      );
    }
    $form['mapping']['to'] = array(
      '#type' => 'fieldset',
      '#title' => 'To settings',
      '#collapsible' => TRUE,
      '#collapsed' => !empty($options['mapping']['to']['id']),
    );
    foreach ($this->graph_fields['to'] as $id => $info) {
      $form['mapping']['to'][$id] = array(
        '#type' => 'select',
        '#required' => isset($info['required']) && $info['required'],
        '#title' => t($info['title']),
        '#options' => $field_names,
        '#default_value' => $options['mapping']['to'][$id],
      );
    }
    return $form;
  }

  /**
   * Implementation of views_style_plugin::theme_functions(). Returns an array of theme functions to use.
   * for the current style plugin
   * @return array
   */
  function theme_functions() {
    $options = $this->options + $this->option_definition();
    $hook =  'views_graphapi_style_graphapi';
    return views_theme_functions($hook, $this->view, $this->display);
  }

  /**
   * Implements views_style_plugin::additional_theme_functions(). Returns empty array.
   * @return array
   */
  function additional_theme_functions() {
    return array();
  }

  /**
   * Implementation of view_style_plugin::render()
   */
  function render() {
    $view = $this->view;
    $options = $this->options;
    $field = $view->field;
    $rows = array();
    $engine = $options['engine'];
    $settings = array();
    // Grab global settings
    if (isset($options['graphapi'])) {
      $settings = $options['graphapi'];
    }
    // Grab engine settings
    if (isset($options[$engine])) {
      $settings += $options[$engine];
    }
    $settings['engine'] = $engine;
    $vars = array(
      'view' => $view,
      'options' => $options,
      'rows' => $rows,
      'settings' => $settings
    );
    return theme($this->theme_functions(), $vars);
  }

}
