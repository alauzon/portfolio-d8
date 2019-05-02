<?php

/**
 * Create the clone form.
 *
 * @param form_state reference to the state of the form
 * @param menu the menu object received from hook_load()
 * @see menu_clone_load()
 *
 * TODO: See if we can improve the $form['edit_menu']['edit_tree'] field. Use
 * something else instead of 'markup'.
 */
function menu_clone_clone_form($form, &$form_state, $menu) {

  module_load_include('inc', 'menu', 'menu.admin');
  $form = array_merge($form, _menu_overview_tree_form($menu['tree']));

  $form['#menu'] =  $menu;

  $form['menu_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Menu name'),
    '#default_value' => $menu['menu_name'],
    '#maxlength' => MENU_MAX_MENU_NAME_LENGTH_UI - drupal_strlen('menu-'),
    '#description' => t('The machine-readable name of this menu. This text will be used for constructing the URL of the <em>menu overview</em> page for this menu. This name must contain only lowercase letters, numbers, and hyphens, and must be unique.'),
    '#required' => TRUE,
  );
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => $menu['title'],
    '#required' => TRUE,
  );
  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#default_value' => $menu['description'],
  );
  if (!empty($menu['tree'])) {
    $form['edit_menu'] = array(
      '#type' => 'fieldset',
      '#title' => t('Customise menu'),
      '#description' => t('Before actually cloning the menu, you can customise it first.'),
      '#collapsible' => FALSE,
      '#collapsed' => FALSE,
      '#tree' => TRUE,
    );
    if (module_exists('i18nmenu')) {
      $form['edit_menu']['menu_lang'] = array(
        '#type' => 'select',
        '#title' => t('Change language'),
        '#description' => t('You can globally change the language of <em>all</em> the available menu items.'),
        '#options' => array_merge(array('' => t('No change')), i18n_language_list()),
      );
    }
    $form['edit_menu']['edit_tree'] = array(
      '#markup' => '<div class="form-item"><label>' . t('Edit menu') . ':</label><div class="description">' . t('You can reorder the menu items and adjust basic settings here. Drag unwanted items below the <em>Ignore</em> row, or drag the <em>Ignore</em> row itself.') . '</div></div>',
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create new menu'),
  );
  return $form;
}

/**
 * Validation function for menu_clone_cone_form().
 *
 * @param form the form object
 * @param form_state reference to the form state
 * @see menu_clone_cone_form()
 */
function menu_clone_clone_form_validate($form, &$form_state) {
  $menu = $form_state['values'];
  if (preg_match('/[^a-z0-9-]/', $menu['menu_name'])) {
    form_set_error('menu_name', t('The menu name may only consist of lowercase letters, numbers, and hyphens.'));
  }
  $sql = "SELECT menu_name FROM {menu_custom} WHERE menu_name = :menu_name";
  $result = db_query($sql, array(':menu_name' => 'menu-' . $menu['menu_name']))->fetchField();
  if (!empty($result)) {
    form_set_error('menu_name', t("The machine-readable name '@menu_name' must be unique. A menu named '@menu_name' already exists.", array('@menu_name' => $menu['menu_name'])));
  }
}

/**
 * Submit function for menu_clone_cone_form(). Parts were taken from the Drupal
 * core Menu module.
 *
 * @param form the form object
 * @param form_state reference to the form state
 * @see menu_clone_cone_form(), menu_overview_form_submit(), menu_edit_menu_submit()
 */
function menu_clone_clone_form_submit($form, &$form_state) {

  $menu = $form_state['values'];
  $path = 'admin/structure/menu/manage/';
  $menu['menu_name'] = 'menu-' . $menu['menu_name'];
  $link['link_title'] = $menu['title'];
  $link['link_path'] = $path . $menu['menu_name'];
  $link['router_path'] = $path . '%';
  $link['module'] = 'menu';
  $link['plid'] = db_query("SELECT mlid FROM {menu_links} WHERE link_path = :link_path AND module = :module", array(':link_path' => 'admin/build/menu', ':module' => 'system'))->fetchField();
  menu_link_save($link);
  db_query("INSERT INTO {menu_custom} (menu_name, title, description) VALUES (:menu_name, :title, :description)", array(':menu_name' => $menu['menu_name'], ':title' => $menu['title'], ':description' => $menu['description']));

  $order = array_flip(array_keys($form_state['input'])); // Get the $_POST order.
  $form = array_merge($order, $form); // Update our original form with the new order.
  $updated_items = array();
  $fields = array('weight', 'plid');
  foreach (element_children($form) as $mlid) {
    if (isset($form[$mlid]['#item'])) {
      $element = $form[$mlid];
      foreach ($fields as $field) {
        $element['#item'][$field] = $element[$field]['#value'];
        $updated_items[$mlid] = $element['#item'];
      }
      $element['#item']['hidden'] = $element['hidden']['#value'] ? 0 : 1;
      $element['#item']['menu_name'] = $menu['menu_name'];
      $element['#item']['mlid'] = NULL;
      $element['#item']['module'] = 'menu';
      $element['#item']['customized'] = 1;
      $updated_items[$mlid] = $element['#item'];
    }
  }

  // Save all our changed items to the database.
  foreach ($updated_items as $key => $item) {
    if (isset ($item['plid']) && $item['plid'] != 0) {
      $item['plid'] = $updated_items['mlid:' . $item['plid']]['mlid'];
    }
    $mlid = menu_link_save($item);
    if (isset ($item['mlid']) && $item['mlid'] != 0) {
      $updated_items[$key]['mlid'] = $mlid;
    }
  }
  cache_clear_all('*', 'cache_menu', TRUE);
  $form_state['redirect'] = $path . $menu['menu_name'];
}

/**
 * Theme the menu tree of the clone form into a table and display it inside its
 * proper fieldset.
 *
 * @param form The form array to be themed
 * @see menu_clone_clone_form()
 * @ingroup themeable
 */
function theme_menu_clone_clone_form($variables) {

  $form = $variables['form'];

  drupal_add_tabledrag('menu-overview', 'match', 'parent', 'menu-plid', 'menu-plid', 'menu-mlid', TRUE, MENU_MAX_DEPTH - 1);
  drupal_add_tabledrag('menu-overview', 'order', 'sibling', 'menu-weight');

  $header = array(
    t('Menu link'),
    array('data' => t('Enabled'), 'class' => array('checkbox')),
    t('Weight'),
    array('data' => t('Operations'), 'colspan' => '3'),
  );

  $rows = array();
  foreach (element_children($form) as $mlid) {
    if (isset($form[$mlid]['hidden'])) {
      $element = &$form[$mlid];
      // Build a list of operations.
      $operations = array();
      foreach (element_children($element['operations']) as $op) {
        $operations[] = array('data' => drupal_render($element['operations'][$op]), 'class' => array('menu-operations'));
      }
      while (count($operations) < 2) {
        $operations[] = '';
      }

      // Add special classes to be used for tabledrag.js.
      $element['plid']['#attributes']['class'] = array('menu-plid');
      $element['mlid']['#attributes']['class'] = array('menu-mlid');
      $element['weight']['#attributes']['class'] = array('menu-weight');

      // Change the parent field to a hidden. This allows any value but hides the field.
      $element['plid']['#type'] = 'hidden';
      $row = array();
      $row[] = theme('indentation', array('size' => $element['#item']['depth'] - 1)) . drupal_render($element['title']);
      $row[] = array('data' => drupal_render($element['hidden']), 'class' => array('checkbox', 'menu-enabled'));
      $row[] = drupal_render($element['weight']) . drupal_render($element['plid']) . drupal_render($element['mlid']);
      $row = array_merge($row, $operations);

      $row = array_merge(array('data' => $row), $element['#attributes']);
      $row['class'][] = 'draggable';
      $rows[] = $row;
    }
  }
  $output = '';
  $form['edit_menu']['tree']['#children'] = theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'menu-overview')));
  $output .= drupal_render_children($form);
  return $output;

}
