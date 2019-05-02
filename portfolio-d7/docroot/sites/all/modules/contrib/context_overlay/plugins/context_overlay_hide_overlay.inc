<?php

/**
 * Expose themes as context reactions.
 */
class context_overlay_hide_overlay extends context_reaction {

  /**
   * Allow admins to provide a section title, section subtitle and section class.
   */
  function options_form($context) {
    $form = array(
      '#tree' => TRUE,
      '#title' => t('Hide overlay settings'),
      'enabled' => array(
        '#type' => 'checkbox',
        '#title' => t('Make sure this page is not shown in the overlay.'),
        '#default_value' => 1,
      ),
    );
    return $form;
  }

  function execute() {
    foreach ($this->get_contexts() as $k => $v) {
      dpm($v);
      $current_path = current_path();
      overlay_close_dialog($current_path);
    }
  }
}
