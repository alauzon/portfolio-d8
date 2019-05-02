<?php

/**
 * Expose themes as context reactions.
 */
class context_overlay_show_overlay extends context_reaction {

  /**
   * The form. Currently just includes an enabled checkbox
   */
  function options_form($context) {
    $form = array(
      '#tree' => TRUE,
      '#title' => t('Show overlay settings'),
      'enabled' => array(
        '#type' => 'checkbox',
        '#title' => t('Make sure this page is shown in an overlay window.'),
        '#default_value' => 1,
      ),
    );
    return $form;
  }

  function execute() {
    //dpm($hook);
    //dpm(context_get('context'));
    foreach ($this->get_contexts() as $k => $v) {
      if ($v->reactions['show_overlay']['enabled']){
        if ((isset($_GET['render']) && $_GET['render'] == 'overlay')){
          // In overlay
          overlay_set_mode('child');
        } else {
          // Not in overlay.  Redirect to page with overlay.
          $url = $_SERVER['HTTP_REFERER'];
          $current_path = current_path();
          $url = ($url == $current_path) ? '<front>' : $url; // Try to reload the current page in the process, but fail back to <front> to avoid redirect error.
          drupal_goto($url, array('fragment' => 'overlay=' . $current_path));
        }
      }
    }
  }
}
