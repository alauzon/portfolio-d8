<?php

/**
 * @file
 * Provides admin functions for the Cloud Zoom module.
 */

function cloud_zoom_settings_form() {
  $form['cloud_zoom_zoom_type'] = array(
    '#type' => 'radios',
    '#title' => t('What style zoom should be used?'),
    '#options' => array(
      'popup' => t('Popup window'),
      'inner' => t('Inner zoom'),
    ),
    '#default_value' => variable_get('cloud_zoom_zoom_type', 'popup'),
  );

  $form['cloud_zoom_multiple_thumbs'] = array(
    '#type' => 'radios',
    '#title' => t('Display multiple images as thumbnails?'),
    '#options' => array(t('No'), t('Yes')),
    '#default_value' => variable_get('cloud_zoom_multiple_thumbs', 0),
  );

  $form['cloud_zoom_hide_active_thumb'] = array(
    '#type' => 'radios',
    '#title' => t('Hide active thumbnail?'),
    '#description' => t('When multiple images are shown as thumbnails, should
      the active thumbnail be hidden?'),
    '#options' => array(t('No'), t('Yes')),
    '#default_value' => variable_get('cloud_zoom_hide_active_thumb', 0),
  );

  $form['cloud_zoom_display_title'] = array(
    '#type' => 'radios',
    '#title' => t('Display the image title in the zoom window?'),
    '#options' => array(t('No'), t('Yes')),
    '#default_value' => variable_get('cloud_zoom_display_title', 0),
  );

  $form['cloud_zoom_loading'] = array(
    '#type' => 'fieldset',
    '#title' => t('Script Loading'),
    '#collapsible' => TRUE,
  );

  $form['cloud_zoom_loading']['cloud_zoom_load_all'] = array(
    '#type' => 'radios',
    '#title' => t('Load on all pages of the following types?'),
    '#options' => array(t('No'), t('Yes')),
    '#default_value' => variable_get('cloud_zoom_load_all', 0),
  );

  $types = node_type_get_names();
  $form['cloud_zoom_loading']['cloud_zoom_load_all_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Content types'),
    '#options' => $types,
    '#default_value' => variable_get('cloud_zoom_load_all_types', array()),
  );

  return system_settings_form($form);
}
