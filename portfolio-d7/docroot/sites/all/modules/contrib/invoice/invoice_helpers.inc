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
 * Adds additional structure to the specified node
 *
 * @param object $node
 */
function _invoice_extend_node($node) {
  if (!isset($node->invoice)) {
    $node->invoice = array(
      'invoice_number'            => null,
      'formatted_invoice_number'  => null,
      'invoice_number_zerofill'   => null,
      'invoice_number_prefix'     => null,
      'description'               => null,
      'vat'                       => null,
      'pay_status'                => null,
      'pay_limit'                 => null,
      'template'                  => null,
      'extotal'                   => null,
      'inctotal'                  => null,
      'formatted_vattotal'        => null,
      'formatted_extotal'         => null,
      'formatted_inctotal'        => null,
      'formatted_created'         => null,
    );
  }

  if (!isset($node->customer)) {
    $node->customer = array(
      'cid'                       => null,
      'customer_number'           => null,
      'name'                      => null,
      'company_name'              => null,
      'firstname'                 => null,
      'lastname'                  => null,
      'fullname'                  => null,
      'street'                    => null,
      'building_number'           => null,
      'zipcode'                   => null,
      'city'                      => null,
      'state'                     => null,
      'country'                   => null,
      'coc_number'                => null,
      'vat_number'                => null,
      'description'               => null,
    );
  }

  if (!isset($node->invoice_items)) {
    $node->invoice_items = array();
  }
}

/**
 * Alternative for the default PHP money_format() function
 *
 * The function money_format() is only defined if the system has strfmon capabilities.
 * For example, Windows does not, so money_format() is undefined in Windows. This function will
 * then be used as alternative, it tries to work just the same as the original function.
 *
 * This function is tested using a XAMPP installation with Apache and PHP 5.2.6 on Windows XP.
 * The code of this function is based on http://nl.php.net/manual/en/function.money-format.php#81901
 */
function _invoice_money_format($format, $number) {
  $regex = array(
    '/%((?:[\^!\-]|\+|\(|\=.)*)([0-9]+)?(?:#([0-9]+))?',
    '(?:\.([0-9]+))?([in%])/'
  );
  $regex = implode('', $regex);

  if (setlocale(LC_MONETARY, NULL) == '') {
    setlocale(LC_MONETARY, '');
  }

  $locale = localeconv();
  $number = floatval($number);

  if (!preg_match($regex, $format, $fmatch)) {
    trigger_error("No format specified or invalid format", E_USER_WARNING);
    return $number;
  }

  $flags = array(
    'fillchar' => preg_match('/\=(.)/', $fmatch[1], $match) ? $match[1] : ' ',
    'nogroup' => preg_match('/\^/', $fmatch[1]) > 0,
    'usesignal' => preg_match('/\+|\(/', $fmatch[1], $match) ? $match[0] : '+',
    'nosimbol' => preg_match('/\!/', $fmatch[1]) > 0,
    'isleft' => preg_match('/\-/', $fmatch[1]) > 0
  );

  $width = trim($fmatch[2]) ? (int) $fmatch[2] : 0;
  $left = trim($fmatch[3]) ? (int) $fmatch[3] : 0;
  $right = trim($fmatch[4]) ? (int) $fmatch[4] : $locale['int_frac_digits'];
  $conversion = $fmatch[5];
  $positive = TRUE;

  if ($number < 0) {
    $positive = FALSE;
    $number *= - 1;
  }

  $letter = $positive ? 'p' : 'n';
  $prefix = $suffix = $cprefix = $csuffix = $signal = '';
  if (!$positive) {
    $signal = $locale['negative_sign'];
    switch (TRUE) {
      case $locale['n_sign_posn'] == 0 || $flags['signal'] == '(': // Probably doesn't work right with negative numbers, bug fix on the line below
        //case $locale['n_sign_posn'] == 0 || $flags['usesignal'] == '(': // Fixed bug according to: http://nl.php.net/manual/en/function.money-format.php#86914
        $prefix = '(';
        $suffix = ')';
        break;
      case $locale['n_sign_posn'] == 1:
        $prefix = $signal;
        break;
      case $locale['n_sign_posn'] == 2:
        $suffix = $signal;
        break;
      case $locale['n_sign_posn'] == 3:
        $cprefix = $signal;
        break;
      case $locale['n_sign_posn'] == 4:
        $csuffix = $signal;
        break;
    }
  }

  if (!$flags['nosimbol']) {
    $currency = $cprefix;
    $currency .= ( $conversion == 'i' ? $locale['int_curr_symbol'] : $locale['currency_symbol']);
    $currency .= $csuffix;
  }
  else {
    $currency = '';
  }

  $space = $locale["{$letter}_sep_by_space"] ? ' ' : '';

  $number = number_format($number, $right, $locale['mon_decimal_point'], $flags['nogroup'] ? '' : $locale['mon_thousands_sep']);
  $number = explode($locale['mon_decimal_point'], $number);

  $n = strlen($prefix) + strlen($currency);

  if ($left > 0 && $left > $n) {
    if ($flags['isleft']) {
      $number[0] .= str_repeat($flags['fillchar'], $left - $n);
    }
    else {
      $number[0] = str_repeat($flags['fillchar'], $left - $n) . $number[0];
    }
  }

  $number = implode($locale['mon_decimal_point'], $number);

  if ($locale["{$letter}_cs_precedes"]) {
    $number = $prefix . $currency . $space . $number . $suffix;
  }
  else {
    $number = $prefix . $number . $space . $currency . $suffix;
  }

  if ($width > 0) {
    $number = str_pad($number, $width, $flags['fillchar'], $flags['isleft'] ? STR_PAD_RIGHT : STR_PAD_LEFT);
  }

  $format = str_replace($fmatch[0], $number, $format);

  return $format;
}

/**
 * Helper function to calculate invoice totals
 *
 * The ROUND() function of MySQL is not very reliable, especially in MySQL 4.x,
 * that's why this function came into play
 *
 * @param integer $invoice_number
 * @param integer $user_id
 *
 * @return array $a_totals
 */
function _invoice_get_invoice_totals($invoice_number, $user_id=0) {
  $totals = array('extotal' => null, 'inctotal' => null, 'vattotal' => null);

  $sql = "SELECT vat,quantity*unitcost as extotal,
    (quantity*unitcost)*((vat / 100) +1) as inctotal,
    (quantity*unitcost)*((vat / 100) +1) * (1 - (1 / ((vat / 100) +1))) as vattotal
    FROM {invoice_items}
    WHERE invoice_id = :invoice_id";

  $params = array(':invoice_id' => $invoice_number);

  if ($user_id > 0) {
    $sql .= " AND uid = :uid";
    $params[':uid'] = (int) $user_id;
  }

  $result = db_query($sql, $params);
  foreach ($result as $row) {
    $totals['extotal'] += _invoice_round($row->extotal, 2);
    $totals['inctotal'] += ( $row->extotal * _invoice_vat_percent_to_decimal($row->vat));
    $totals['vattotal'] += $row->vattotal;
  }

  $totals['extotal'] = _invoice_round($totals['extotal'], 2);
  $totals['inctotal'] = _invoice_round($totals['inctotal'], 2);
  $totals['vattotal'] = _invoice_round($totals['vattotal'], 2);

  return $totals;
}

/**
 * Converts a VAT value in percent to decimal
 *
 * For examle:
 * A vat value of 20,5 will be converted into 1.205
 *
 * @param mixed $vat
 */
function _invoice_vat_percent_to_decimal($percent_vat) {
  $decimal_vat = 1 + ($percent_vat / 100);
  return $decimal_vat;
}

/**
 * Returns a rounded and formatted number according to the locale set on the invoice settings page
 *
 * @param mixed $number
 * @param integer $precision
 *
 * @return mixed
 */
function _invoice_round_and_format_money($number, $precision = 2) {

  // If the precision is larger than 2 and the rounded number has no more than 2 decimals,
  // then still only 2 decimals will be displayed.
  $rounded_number = _invoice_round($number, $precision);

  // Count the amount of decimals
  $exp = explode('.', $rounded_number);
  $decimals = isset($exp[1]) ? strlen($exp[1]) : 0;

  // Display at least 2 decimals
  if ($decimals < 2) {
    $decimals = 2;
  }

  // Money format the rounded number
  return $rounded_and_formatted_number = _invoice_format_money($rounded_number, $decimals);
}

/**
 * Returns a number in money format according to the locale set on the invoice settings page
 *
 * @param mixed $number
 * @param integer $precision
 * @return mixed
 */
function _invoice_format_money($number, $precision) {

  if (!function_exists('money_format')) {
    $formatted_number = _invoice_money_format('%.' . $precision . 'n', $number);
  }
  else {
    $formatted_number = money_format('%.' . $precision . 'n', $number);
  }

  return $formatted_number;
}

/**
 * Returns if the user has access to administer the given invoice
 *
 * @param integer $invoice_id
 * @return boolean
 */
function _invoice_user_has_admin_access_to_invoice($invoice_id) {
  $has_access = FALSE;

  if (empty($invoice_id)) {
    $has_access = TRUE;
  }
  else {
    if (user_access('administer invoices')) {
      $has_access = TRUE;
    }
    elseif (user_access('administer own invoices')) {
      $count = db_query("SELECT COUNT(*) FROM {invoice_invoices} WHERE iid = :iid AND uid = :uid", array(
        ':iid' => $invoice_id,
        ':uid' => $GLOBALS['user']->uid
      ))->fetchField();
      if ($count > 0) {
        $has_access = TRUE;
      }
    }
  }

  return $has_access;
}

/**
 * Helper function to easily get the name of the chosen template when adding an invoice
 */
function _invoice_get_chosen_template() {
  return !isset($_SESSION['invoice_template']) || empty($_SESSION['invoice_template']) ?
    variable_get('invoice_default_template', 'default') : $_SESSION['invoice_template'];
}

/**
 * Helper function to get the invoice in HTML format
 *
 * @param integer $invoice_number
 * @param object  $node
 * @param string  $type
 */
function _invoice_get_html($invoice_number, $node = NULL, $type = 'print') {
  if (is_null($node)) {
    $nid = db_query("SELECT nid FROM {invoice_invoices} WHERE iid = :iid",
      array(':iid' => $invoice_number))->fetchField();
    $node = node_load($nid);
  }

  $html = '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"'
    . ' "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">' . "\n";
  $html .= '<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en" dir="ltr">' . "\n";
  $html .= '<head>' . "\n";
  $html .= '<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />' ."\n";

  $protocol = _invoice_get_transfer_protocol() . '://';
  $stylesheetPath = drupal_get_path('module', 'invoice')
      . '/templates/' . $node->invoice['template'] . '.css';

  if ($type == 'pdf') {
    $html .= '<style type="text/css">';
    $html .= "\n". file_get_contents(DRUPAL_ROOT . '/' . $stylesheetPath) ."\n";
    $html .= '</style>' ."\n";
  }
  else {
    $html .= '<link type="text/css" rel="stylesheet" media="all" href="' . $protocol
      . $_SERVER['HTTP_HOST'] . base_path() . $stylesheetPath .'" />' . "\n";
  }

  $html .= '</head><body class="' . $type . '">';

  $html .= theme('invoice_body', array('node' => $node, 'type' => $type));

  // The eurosign is not supported as applicable character, so replace it with a ascii code
  $html = str_replace('€', '&#0128;', $html);

  $html .= '</body></html>';
  return $html;
}

/**
 * Helper function to get invoice id
 *
 * @return integer
 */
function _invoice_get_new_invoice_number($user_defined_invoice_number_check = FALSE) {
  if ($user_defined_invoice_number_check == TRUE) {
    $count = db_query("SELECT COUNT(*) FROM {invoice_invoices}")->fetchField();
    if ($count == 0) {
      return 0;
    }
    else {
      return db_query_range("SELECT iid FROM {invoice_invoices} ORDER BY nid DESC", array(), 1)->fetchField();
    }
  }

  $new_invoice_number = db_query_range("SELECT iid FROM {invoice_invoices} ORDER BY nid DESC", array(), 1)
    ->fetchField() + 1;

  return $new_invoice_number;
}

/**
 * Helper function get a formatted invoice number
 *
 * @param integer $invoice_number
 * @param mixed   $formatted_invoice_number
 * @param mixed   $created NULL or a UNIX timestamp
 */
function _invoice_get_formatted_invoice_number($invoice_number, $node = NULL, $created = NULL) {
  $formatted_invoice_number = $invoice_number;

  if (!is_null($node) && !empty($node->created)) {
    $created = $node->created;
  }
  elseif (is_null($created)) {
    $created = REQUEST_TIME;
  }

  if (is_null($node)) {
    $invoice_settings = db_query("SELECT leading_zeros,prefix FROM {invoice_invoices} WHERE iid = :iid", array(
      ':iid' => $invoice_number))->fetchAssoc();

    $invoice_number_zerofill = $invoice_settings['leading_zeros'];
    $invoice_number_prefix = $invoice_settings['prefix'];
  }
  else {
    $invoice_number_zerofill = $node->invoice_invoice_number_zerofill;
    $invoice_number_prefix = $node->invoice_invoice_number_prefix;
  }

  // Add leading zeros
  $formatted_invoice_number = sprintf("%0" . $invoice_number_zerofill . "d", $formatted_invoice_number);

  // Add prefix
  $prefix_string = $invoice_number_prefix;
  $possible_date_arguments = _invoice_get_possible_date_arguments();

  $exp = explode('%', $prefix_string);
  $i = 0;
  foreach ($exp as $key => $value) {
    $i++;
    // If prefix string didn't start with %, the first characters are never a date argument
    if (substr($prefix_string, 0, 1) == '%' || $i > 1) {
      if (strlen($value) == 1) {
        if (in_array($value, $possible_date_arguments)) {
          $exp[$key] = date($value, $created);
        }
      }
      else {
        if (in_array(substr($value, 0, 1), $possible_date_arguments)) {
          $exp[$key] = date(substr($value, 0, 1), $created);

          // Add the rest of the string which was not supposed to be a date argument
          $exp[$key] .= substr($value, 1);
        }
      }
    }
  }

  $prefix = implode('', $exp);

  $formatted_invoice_number = $prefix . $formatted_invoice_number;

  return $formatted_invoice_number;
}

/**
 * Returns all possible date arguments
 *
 * @return array
 */
function _invoice_get_possible_date_arguments() {
  $possible_date_arguments = array(
    // Day
    'd', 'D', 'j', 'l', 'N', 'S', 'w', 'z',
    // Week
    'W',
    // Month
    'F', 'm', 'M', 'n', 't',
    // Year
    'L', 'o', 'Y', 'y',
    // Time
    'a', 'A', 'B', 'g', 'G', 'h', 'H', 'i', 's', 'u',
    // timezone
    'e', 'I', 'O', 'P', 'T', 'Z',
    // Full Date / Time
    'c', 'r', 'U'
  );

  return $possible_date_arguments;
}

/**
 * Helper function to add JS or CSS to Drupal
 */
function _invoice_add_css_js() {
  $invoice_js_settings = array(
    'host' => $_SERVER['HTTP_HOST'],
    'clean_urls' => (bool) variable_get('clean_url', FALSE),
  );
  drupal_add_js(array('invoice' => $invoice_js_settings), 'setting');
  drupal_add_js(drupal_get_path('module', 'invoice') . '/javascript/invoice.js');
  drupal_add_css(drupal_get_path('module', 'invoice') . '/invoice.css');
}

/**
 * Helper function to get a rounded value
 *
 * Read the comments on http://www.php.net/round why the
 * standard PHP round() function doesn't work right.
 *
 * For example with PHP round(38.675, 2) gives 38.67, but that must be 38.68
 *
 * @param mixed $value
 * @param integer $precision
 * @return float
 */
function _invoice_round($value, $precision=0) {
  $rounded_value = round(round($value * pow(10, $precision + 1), 0), -1) / pow(10, $precision + 1);
  return $rounded_value;
}

/**
 * Helper function to get the available template names
 */
function _invoice_get_templates() {
  $templates = array('default');
  $files = file_scan_directory(dirname(__FILE__) . '/templates', '/\.inc$/i');
  foreach ($files as $file) {
    if (!empty($file->name) && $file->name != 'default') {
      $templates[] = check_plain($file->name);
    }
  }

  return $templates;
}

/**
 * Helper function to get template variables
 */
function _invoice_get_variable($template, $name, $default=NULL) {
  // if $template is empty, check if a general/default value is available
  if (empty($template)) {
    return variable_get('invoice_' . $name, '');
  }

  static $templates = NULL;

  // Get all template settings from the database only one time
  if (!is_array($templates)) {
    // Get template names available on disk
    $template_names = _invoice_get_templates();

    // Add templates to the database that exist on the disk but not in the database yet.
    $database_template_names = array();
    $result = db_query("SELECT name FROM {invoice_templates} WHERE name IN (:names)", array(
      ':names' => $template_names))->fetchAll(PDO::FETCH_ASSOC);
    foreach ($result as $row) {
      $database_template_names[] = $row['name'];
    }

    foreach ($template_names as $template_name) {
      if (!in_array($template_name, $database_template_names)) {
        // Add template
        db_insert('invoice_templates')->fields(array(
          'name' => $template_name,
          'locale' => '',
          'date_format' => '',
          'vat'  => variable_get('invoice_vat', 0),
          'pay_limit' => variable_get('invoice_pay_limit', 0),
          'supplier_company_name' => '',
          'supplier_street' => '',
          'supplier_building_number' => '',
          'supplier_zipcode' => '',
          'supplier_city' => '',
          'supplier_state' => '',
          'supplier_country' => '',
          'supplier_phone' => '',
          'supplier_fax' => '',
          'supplier_email' => '',
          'supplier_web' => '',
          'supplier_coc_number' => '',
          'supplier_vat_number' => '',
          'api_username' => '',
          'display_column_vat' => variable_get('invoice_display_column_vat', 0),
          'display_column_exunitcost' => variable_get('invoice_display_column_exunitcost', 1),
          'display_column_incunitcost' => variable_get('invoice_display_column_incunitcost', 1),
          'display_column_extotal' => variable_get('invoice_display_column_extotal', 1),
          'display_column_inctotal' => variable_get('invoice_display_column_inctotal', 1)
        ))->execute();
      }
    }

    // Get all template settings
    $templates = array();
    $result = db_query("SELECT * FROM {invoice_templates} WHERE name IN (:names)", array(
      ':names' => $template_names))->fetchAll(PDO::FETCH_ASSOC);
    foreach ($result as $row) {
      $templates[$row['name']] = $row;
    }
  }

  // Get template info that is stored in the database
  $template = $templates[$template];

  if (!empty($template[$name]) || is_numeric($template[$name])) {
    return $template[$name];
  }
  else {
    if (!is_null($default)) {
      return $default;
    }
    else {
      // if $default is not null, check if a general/default value is available
      return variable_get('invoice_' . $name, '');
    }
  }
}

/**
 * Helper function to get an icon
 */
function _invoice_get_icon($name, $url = NULL, $attributes = array(), $extension = 'png') {
  $attributes['alt'] = isset($attributes['alt']) ? $attributes['alt'] : '';
  if (empty($attributes['alt']) && isset($attributes['title']) && !empty($attributes['title'])) {
    $attributes['alt'] = $attributes['title'];
  }

  $img_addition = '';
  foreach ($attributes as $key => $value) {
    $img_addition .= ' ' . $key . '="' . $value . '"';
  }

  $icon = '<img src="' . base_path() . drupal_get_path('module', 'invoice') . '/images/' . $name
    . '.' . $extension . '"' . $img_addition . ' />';
  if (!empty($url)) {
    $icon = l($icon, $url, array('html' => TRUE));
  }

  return $icon;
}

/**
 * Helper function to collect all $_GET variables and put them in a string
 */
function _invoice_getvars_array_to_string($vars = array()) {

  unset($vars['q']);

  $vars_string = '?';
  foreach ($vars as $key => $value) {
    $vars_string .= $key . '=' . $value . '&';
  }

  return rtrim($vars_string, '&');
}

/**
 * Helper function to convert the string with get variables to an array
 *
 * @param  string $vars_string
 * @return array
 */
function _invoice_getvars_string_to_array($vars_string) {
  $exp = explode('&', $vars_string);
  $query_vars = array();
  // the first element is always "q", we don't want that element so start with $i=1
  for ($i = 1; $i < count($exp); $i++) {
    $sub_exp = explode('=', $exp[$i]);
    $query_vars[$sub_exp[0]] = $sub_exp[1];
  }
  return $query_vars;
}

/**
 * Returns installed locales on the system
 *
 * @return array
 */
function _invoice_get_installed_system_locales() {
  ob_start();
  system('locale -a');
  $str = ob_get_contents();
  ob_end_clean();
  $list = explode("\n", trim($str));

  return $list;
}

/**
 * Returns the current transfer protocol HTTP or HTTPS
 */
function _invoice_get_transfer_protocol() {
  return isset($_SERVER["HTTPS"]) && $_SERVER["HTTPS"] == 'on' ? 'https' : 'http';
}

/**
 * Sets the invoice content type settings
 */
function _invoice_set_content_type_settings() {
  // Disable preview submission
  variable_set('node_preview_invoice', 0); // DRUPAL_DISABLED

  // Disable comments
  variable_set('comment_invoice', 1); // COMMENT_NODE_CLOSED

  // Remove promote setting
  $node_options = variable_get('node_options_invoice', array());
  if (in_array('promote', $node_options)) {
    foreach ($node_options as $key => $option) {
      if ($option == 'promote') {
        unset($node_options[$key]);
      }
    }
  }

  // Set published status
  if (!in_array('status', $node_options)) {
    $node_options[] = 'status';
  }
  variable_set('node_options_invoice', $node_options);
}

/**
 * Helper function to include the dompdf library
 */
function _invoice_dompdf_include_lib() {
  $error = FALSE;
  $file = NULL;
  if (file_exists(DRUPAL_ROOT . '/sites/all/libraries/dompdf/dompdf_config.inc.php')) {
    $file = DRUPAL_ROOT . '/sites/all/libraries/dompdf/dompdf_config.inc.php';
  }
  elseif (file_exists(dirname(__FILE__) . '/../../libraries/dompdf/dompdf_config.inc.php')) {
    $file = dirname(__FILE__) . '/../../libraries/dompdf/dompdf_config.inc.php';
  }
  elseif (file_exists(dirname(__file__) . '/dompdf/dompdf_config.inc.php')) { // Backwards compatible
    $file = dirname(__file__) . '/dompdf/dompdf_config.inc.php';
  }
  else {
    drupal_set_message(t('The DOMPDF library could not be found!'), 'error');
    $error = TRUE;
  }

  if ($file !== NULL) {
    $content = file_get_contents($file);
    if (strpos($content, 'spl_autoload_register') !== false) {
      require_once $file;
    }
    else {
      drupal_set_message(t('Update your DOMPDF library, version 6 or higher is required!'), 'error');
      $error = TRUE;
    }
  }

  return $error;
}