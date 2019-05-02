<?php
/**
 * @file
 * Views style plugins to render nodes in the XML data format.
 *
 * @see views_plugin_style_xml.inc
 * @ingroup views_plugins
 */

/**
 * Implements hook_views_plugins().
 */
function graphapi_views_plugins() {
  $formats = graphapi_views_formats();
  $additional_themes = array();
  $additional_themes['views_graphapi_style_graphapi'] = 'style';

  $path = drupal_get_path('module', 'graphapi') . '/views';
  return array(
    'module' => 'graphapi',
    'style' => array(
      'graphapi_style' => array(
        'title'           => t('Graph rendering'),
        'path'            => $path . '/plugins',
        'help'            => t('Display a view as an Graph.'),
        'handler'         => 'views_plugin_style_graphapi',
        'theme'           => 'views_graphapi_style',
        'theme file'      => 'views_graphapi_style.theme.inc',
        'theme path'      => $path . '/theme',
        'additional themes' => $additional_themes,
        'uses row plugin' => FALSE,
        'uses fields'     => TRUE,
        'uses options'    => TRUE,
        'type'            => 'normal',
        'help_topic'      => 'style-xml',
        'even empty'      => TRUE,
      ),
    ),
  );
}
