<?php

/**
 * Expose themes as context reactions.
 */
class context_overlay_overlay_properties extends context_reaction {

  /**
   * Allow admins to provide a section title, section subtitle and section class.
   */
  function options_form($context) {
    $values = $this->fetch_from_context($context);
    $form = array(
      '#tree' => TRUE,
      '#title' => t('Overlay settings'),
      'width' => array(
        '#title' => t('Custom width'),
        '#description' => t('Specify a custom width for the overlay when this context is active. Examples: 50%, or 400px. Leave blank to use Drupal default (88%).'),
        '#type' => 'textfield',
        '#size' => 5,
        '#default_value' => $values['width'],
      ),
      'margin_top' => array(
        '#title' => t('Top margin'),
        '#description' => t('Specify a margin-top css value. Helpful for trying to center overlays with little content. Examples: 10%, or 200px.'),
        '#type' => 'textfield',
        '#default_value' => $values['margin_top'],
        '#size' => 5,
      ),
      'hide_title' => array(
        '#title' => t('Hide the title'),
        '#type' => 'checkbox',
        '#description' => t('Hide the overlay title.'),
        '#default_value' => $values['hide_title'],
      ),
      'hide_add_shortcut' => array(
        '#title' => t('Hide the add shortcut button'),
        '#type' => 'checkbox',
        '#description' => t('Hide the "+" button that allows users to add a shortcut.'),
        '#default_value' => $values['hide_add_shortcut'],
      ),
      'hide_branding' => array(
        '#title' => t('Hide branding'),
        '#type' => 'checkbox',
        '#description' => t('Hide the div#branding branding container.'),
        '#default_value' => $values['hide_branding'],
      ),
    );
    return $form;
  }

  function execute() {
    foreach ($this->get_contexts() as $k => $v) {
      drupal_add_js(array('context_overlay' => $v->reactions['overlay_properties']), 'setting');
      drupal_add_js(drupal_get_path('module', 'context_overlay') . '/context-overlay.js');
    }
  }
}
