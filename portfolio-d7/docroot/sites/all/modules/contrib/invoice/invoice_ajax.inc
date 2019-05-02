<?php

/**
 * @file
 * Invoice module
 *
 * This module was developed by Platina Designs, http://www.platinadesigns.nl
 *
 * @author Pieter Vogelaar <ps.vogelaar@platinadesigns.nl>
 */

/**
 * Set the chosen invoice when creating a node
 */
function invoice_set_template() {
  $template = check_plain($_GET['value']);
  $_SESSION['invoice_template'] = $template;
  $a_items['vat'] = _invoice_get_variable($template, 'vat');
  drupal_json_output($a_items);
  exit;
}

/**
 * Search if the customer already exists
 */
function invoice_search_customer($value) {
  $items = array();
  $customer_value = check_plain($value);

  $template = _invoice_get_chosen_template();
  $tid = (int) db_query("SELECT tid FROM {invoice_templates} WHERE name = :name",
    array(':name' => $template))->fetchField();

  $query = db_select('invoice_customers', 'ic');
  $query->fields('ic');
  $query->join('invoice_invoices', 'ii', 'ic.invoice_id = ii.iid AND ii.tid = ' . $tid);
  $query->condition(db_or()
    ->condition('company_name', '%' . $customer_value . '%', 'LIKE')
    ->condition('lastname', '%' . $customer_value . '%', 'LIKE')
    ->condition('firstname', '%' . $customer_value . '%', 'LIKE')
  );

  $query->groupBy('vat_number, coc_number, company_name, country, state, city, zipcode,'
    . ' building_number, lastname, firstname');
  $query->orderBy('company_name, lastname, firstname, invoice_id', 'DESC');
  $result = $query->execute();

  foreach ($result as $row) {
    $key = $row->cid;
    $value = NULL;
    if (!empty($row->company_name)) {
      $value = $row->company_name;
      $value = !empty($row->zipcode) ? $value . " - " . $row->zipcode : $value;
      $value = !empty($row->building_number) ? $value . " - " . $row->building_number : $value;
      $value = !empty($row->city) ? $value . " - " . $row->city : $value;
      $value = !empty($row->state) ? $value . " - " . $row->state : $value;
      $value = !empty($row->country) ? $value . " - " . $row->country : $value;
      $value = !empty($row->vat_number) ? $value . " - " . $row->vat_number : $value;
      $value = !empty($row->coc_number) ? $value . " - " . $row->coc_number : $value;
      $value = !empty($row->lastname) ? $value . "\n    " . $row->lastname : $value;
      $value = !empty($row->firstname) ? $value . "\n    " . $row->firstname : $value;
    }
    else {
      $value = $row->lastname . (!empty($row->firstname) ? ', ' . $row->firstname : '');
    }
    $items[$key] = check_plain($value);
  }

  drupal_json_output($items);
  exit;
}

/**
 * Get customer info
 */
function invoice_get_customer_info() {
  // Create alias
  $fv =& $_GET;

  $query = db_select('invoice_customers', 'ic')
    ->fields('ic', array('company_name', 'firstname', 'lastname', 'street', 'building_number', 'zipcode',
      'city', 'state', 'country', 'coc_number', 'vat_number', 'description'));
  $query->leftJoin('invoice_invoices', 'ii', 'ic.invoice_id = ii.iid');
  $data = $query
    ->condition('ic.cid', $fv['value'])
    ->orderBy('ic.company_name', 'DESC')->orderBy('ic.lastname', 'DESC')
    ->orderBy('ic.firstname', 'DESC')->orderBy('ic.invoice_id', 'DESC')
    ->range(0, 1)
    ->execute()
    ->fetchAssoc();

  if (count($data) == 0) {
    $data['set_empty'] = TRUE;
  }
  else {
    $data['search_customer'] = '';
    if (!empty($data['company_name'])) {
      $data['search_customer'] = $data['company_name'];
    }
    else {
      $data['search_customer'] = $data['lastname']
        . (!empty($data['firstname']) ? ', ' . $data['firstname'] : '');
    }
  }

  drupal_json_output($data);
  exit;
}

/**
 * Add an invoice item
 */
function invoice_save_item() {
  $fv =& $_POST;
  $data = array();

  $fv['invoice_number'] = isset($fv['invoice_number']) ? $fv['invoice_number'] : 0;

  // Check user admin access for this invoice
  if (!_invoice_user_has_admin_access_to_invoice($fv['invoice_number'])) {
    $data['error'] = t('You are not the owner of this item id!');
    drupal_json_output($data);
    exit;
  }

  // Set locale so money has the right format for the preferred culture
  if ((int) $fv['invoice_number'] === 0) {
    $locale = _invoice_get_variable(_invoice_get_chosen_template(), 'locale');
    if ($locale) {
      setlocale(LC_MONETARY, $locale);
    }
    $active_template = _invoice_get_chosen_template();
  }
  else {
    $query = db_select('invoice_invoices', 'ii')->fields('it', array('name'));
    $query->leftJoin('invoice_templates', 'it', 'ii.tid = it.tid');
    $template = $query
      ->condition('ii.iid', $fv['invoice_number'])
      ->execute()
      ->fetchField();

    if ($template) {
      $locale = _invoice_get_variable($template, 'locale');
      if ($locale) {
        setlocale(LC_MONETARY, $locale);
      }
      $active_template = $template;
    }
  }

  // Display error if price_wihtout_vat and price_with_vat are both not filled in
  if (empty($fv['price_without_vat']) && empty($fv['price_with_vat'])) {
    $data['error'] = t('Error') . ': ' . t('You have to fill in either "Price without VAT" or "Price with VAT"!');
  }

  if (!empty($data['error'])) {
    drupal_json_output($data);
    exit;
  }

  // Typecast strings to doubles and replace comma with a dot
  $fv['quantity'] = (double) str_replace(',', '.', $fv['quantity']);
  $fv['price_without_vat'] = (double) str_replace(',', '.', $fv['price_without_vat']);
  $fv['price_with_vat'] = (double) str_replace(',', '.', $fv['price_with_vat']);

  // Get the price without VAT
  if (!empty($fv['price_without_vat'])) {
    $unitcost = $fv['price_without_vat'];
  }
  else {
    $unitcost = $fv['price_with_vat'] / _invoice_vat_percent_to_decimal(variable_get('invoice_vat', 0));
  }

  // Round the price to 3 decimals
  $unitcost = round($unitcost, 3);

  // Round quantity to 2 decimals
  $fv['quantity'] = round($fv['quantity'], 2);

  if (intval($fv['iid']) > 0) {
    // item id is greater than zero, so we are saving an existing invoice item
    db_update('invoice_items')->fields(array(
      'description' => $fv['description'],
      'vat' => $fv['vat'],
      'quantity' => $fv['quantity'],
      'unitcost' => $unitcost
    ))
      ->condition('iid', $fv['iid'])
      ->condition('uid', $GLOBALS['user']->uid)
      ->condition('invoice_id', $fv['invoice_number'])
      ->execute();
  }
  else {
    // Insert invoice item into the invoice items table
    $lastInsertId = db_insert('invoice_items')->fields(array(
      'description' => $fv['description'],
      'vat' => $fv['vat'],
      'quantity' => $fv['quantity'],
      'unitcost' => $unitcost,
      'invoice_id' => $fv['invoice_number'] > 0 ? $fv['invoice_number'] : 0,
      'uid' => $GLOBALS['user']->uid,
      'created' => time()
    ))->execute();
  }

  // Count the added items and calculate invoice totals
  $count = db_query("SELECT COUNT(*) FROM {invoice_items} WHERE uid = :uid AND invoice_id = :invoice_id", array(
    ':uid' => $GLOBALS['user']->uid,
    ':invoice_id' => $fv['invoice_number']
  ))->fetchField();

  if (intval($fv['iid']) > 0) {
    // item id is greater than zero, so we are dealing with an existing invoice item
    $data['iid'] = check_plain($fv['iid']);
    $data['description'] = nl2br(check_plain($fv['description']));
    $data['vat'] = check_plain($fv['vat']) . '%';
    $data['quantity'] = check_plain($fv['quantity']);
    $data['exunitcost'] = _invoice_round_and_format_money($unitcost, 3);
    $data['incunitcost'] = _invoice_round_and_format_money($unitcost * _invoice_vat_percent_to_decimal($fv['vat']), 2);
    $data['exsubtotal'] = _invoice_round_and_format_money($fv['quantity'] * $unitcost, 2);
    $data['incsubtotal'] = _invoice_round_and_format_money($fv['quantity'] * $unitcost * _invoice_vat_percent_to_decimal($fv['vat']), 2);
    $data['actionvalue'] = t('Add item');
  }
  else {
    // Set row class name
    $class = 'item-' . $lastInsertId . ' invoice-item draggable';

    // Compose content to send back to the browser
    $data['content'] = sprintf('<tr class="%s"><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td class="actions">%s</td></tr>',
        $count & 1 ? $class . ' odd' : $class . ' even',
        nl2br(check_plain($fv['description'])),
        check_plain($fv['vat']) . '%',
        check_plain($fv['quantity']),
        _invoice_round_and_format_money($unitcost, 3),
        _invoice_round_and_format_money($unitcost * _invoice_vat_percent_to_decimal($fv['vat']), 2),
        _invoice_round_and_format_money($fv['quantity'] * $unitcost, 2),
        _invoice_round_and_format_money($fv['quantity'] * $unitcost * _invoice_vat_percent_to_decimal($fv['vat']), 2),
        _invoice_get_icon('edit', NULL, array(
          'class' => 'action-button edit-action mouse-pointer',
          'title' => t('Edit'),
        ))
        .
        _invoice_get_icon('delete', NULL, array(
          'class' => 'action-button delete-action mouse-pointer',
          'title' => t('Delete'),
        ))
    );

    // Remove the empty from the page
    $data['remove_empty_row'] = TRUE;
  }

  // Define active vat value to put back on the resetted item form
  $data['activevat'] = _invoice_get_variable($active_template, 'vat');

  // Get invoice totals
  $totals = _invoice_get_invoice_totals($fv['invoice_number'], $GLOBALS['user']->uid);

  // Set total
  $data['extotal'] = _invoice_round_and_format_money($totals['extotal'], 2);
  $data['inctotal'] = _invoice_round_and_format_money($totals['inctotal'], 2);

  drupal_json_output($data);
  exit;
}

/**
 * Edit an invoice item
 */
function invoice_edit_item() {
  $fv =& $_GET;
  $data = array();

  // Check if the item to delete exists
  $invoice_item = db_query("SELECT * FROM {invoice_items} WHERE iid = :iid",
    array(':iid' => $fv['iid']))->fetchObject();

  if ($invoice_item) {
    // Check user admin access for this invoice
    if (!_invoice_user_has_admin_access_to_invoice($invoice_item->invoice_id)) {
      $data['error'] = t('You are not the owner of this item id!');
      drupal_json_output($data);
      exit;
    }

    $data['description'] = $invoice_item->description;
    $data['vat'] = $invoice_item->vat;
    $data['quantity'] = $invoice_item->quantity;
    $data['exunitcost'] = $invoice_item->unitcost;
    $data['incunitcost'] =
      $invoice_item->unitcost * _invoice_vat_percent_to_decimal(variable_get('invoice_vat', 0));
    $data['actionvalue'] = t('Save item');
  }
  else {
    $data['error'] = t('This item id does not exist!');
  }

  drupal_json_output($data);
  exit;
}

/**
 * Delete an invoice item
 */
function invoice_delete_item() {
  $fv =& $_GET;
  $data = array();

  $fv['invoice_number'] = isset($fv['invoice_number']) ? $fv['invoice_number'] : 0;

  // Check user admin access for this invoice
  if (!_invoice_user_has_admin_access_to_invoice($fv['invoice_number'])) {
    $data['error'] = t('You are not the owner of this item id!');
    drupal_json_output($data);
    exit;
  }

  $template = db_query("SELECT it.name FROM {invoice_invoices} ii
    LEFT JOIN {invoice_templates} it ON ii.tid = it.tid
    WHERE ii.iid = :iid", array(':iid' => $fv['invoice_number']))->fetchField();

  // Set locale so money has the right format for the preferred culture
  if ((int) $fv['invoice_number'] === 0) {
    $locale = _invoice_get_variable(_invoice_get_chosen_template(), 'locale');
    if ($locale) {
      setlocale(LC_MONETARY, $locale);
    }
  }
  elseif ($template) {
    $locale = _invoice_get_variable($template, 'locale');
    if ($locale) {
      setlocale(LC_MONETARY, $locale);
    }
  }

  // Check if the item to delete exists and is owned by this owner
  $count = db_query("SELECT COUNT(*) FROM {invoice_items}
    WHERE iid = :iid AND invoice_id = :invoice_id GROUP BY iid", array(
      ':iid' => $fv['iid'],
      ':invoice_id' => $fv['invoice_number'],
  ))->fetchField();

  if ($count == 0) {
    $data['error'] = t('This item id does not exist, does not belong to this invoice or you are not the owner!');
  }
  else {
    db_delete('invoice_items')
      ->condition('iid', $fv['iid'])
      ->condition('uid', $GLOBALS['user']->uid)
      ->condition('invoice_id', $fv['invoice_number'])
      ->execute();

    // Get invoice totals
    $totals = _invoice_get_invoice_totals($fv['invoice_number'], $GLOBALS['user']->uid);

    // Set total
    $data['extotal'] = _invoice_round_and_format_money($totals['extotal'], 2);
    $data['inctotal'] = _invoice_round_and_format_money($totals['inctotal'], 2);
  }

  drupal_json_output($data);
  exit;
}