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
 * Implements hook_form()
 *
 * @param  object $node
 * @param  object $form_state
 * @return array              An array containing the title and any custom form elements
 *                            to be displayed in the node editing form.
 */
function invoice_form($node, &$form_state) {
  _invoice_extend_node($node);

  // If an invoice number is available we are in editing mode
  if (!empty($node->invoice['invoice_number'])) {
    $mode = 'edit';
  }
  else {
    $mode = 'create';
  }

  $form = array();
  $form['invoice_template'] = array(
    '#type' => 'fieldset',
    '#title' => t('Invoice template'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#weight' => 1,
  );

  // Get template names
  $templates = _invoice_get_templates();

  // Build array for selecting the default template
  $template_options = array();
  foreach ($templates as $template) {
    $template_options[$template] = ucfirst($template);
  }

  if (empty($node->invoice['template'])) {
    $active_template = _invoice_get_chosen_template();
  }
  else {
    $active_template = $node->invoice['template'];
  }

  $form['invoice_template']['template'] = array(
    '#type' => 'select',
    '#title' => '',
    '#options' => $template_options,
    '#default_value' => $active_template,
    '#attributes' => empty($node->invoice['template']) ?
      array('onchange' => 'invoice.setTemplate(this.value)') : array(),
    '#description' => t("When editing this invoice, you'll have to save first before you can see"
      . " template changes."),
  );

  if (empty($node->invoice['template'])) {
    $_SESSION['invoice_template'] = _invoice_get_chosen_template();
  }

  $form['customer'] = array(
    '#type' => 'fieldset',
    '#title' => t('Customer details'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('You either have to fill in Company name or Lastname. Firstname is optional'
      . ' and is only saved if a Lastname is filled in.'),
    '#weight' => 2,
  );

  if (!empty($node->customer['company_name'])) {
    $customer_search_default_value = $node->customer['company_name'];
  }
  else {
    $customer_search_default_value = $node->customer['lastname'] .
      (!empty($node->customer['firstname']) ? ', ' . $node->customer['firstname'] : '');
  }

  $form['customer']['search'] = array(
    '#type' => 'textfield',
    '#title' => t('Search customer'),
    '#maxlength' => 60,
    '#autocomplete_path' => 'invoice/search/customer',
    '#default_value' => $customer_search_default_value
  );
  $form['customer']['company_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Company name'),
    '#required' => FALSE,
    '#default_value' => $node->customer['company_name'],
  );
  $form['customer']['firstname'] = array(
    '#type' => 'textfield',
    '#title' => t('Firstname'),
    '#required' => FALSE,
    '#default_value' => $node->customer['firstname'],
  );
  $form['customer']['lastname'] = array(
    '#type' => 'textfield',
    '#title' => t('Lastname'),
    '#required' => FALSE,
    '#default_value' => $node->customer['lastname'],
  );

  $invoice_items_table_header = array(
    t('Description'),
    t('VAT'),
    t('Count'),
    t('Unitcost (ex. VAT)'),
    t('Unitcost (inc. VAT)'),
    t('Subtotal (ex. VAT)'),
    t('Subtotal (inc. VAT)'),
    '',
  );

  $form['invoice_items'] = array(
    '#type' => 'fieldset',
    '#title' => t('Invoice items'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#weight' => 4,
  );

  // Set locale so money has the right format for the preferred culture
  if ((int) $node->invoice['invoice_number'] === 0) {
    $locale = _invoice_get_variable(_invoice_get_chosen_template(), 'locale');
    if ($locale) {
      setlocale(LC_MONETARY, $locale);
    }
  }
  else {
    $locale = _invoice_get_variable($active_template, 'locale');
    if ($locale) {
      setlocale(LC_MONETARY, $locale);
    }
  }

  // Get invoice items
  $items = _invoice_get_invoice_items($node->invoice['invoice_number']);
  $count = $items['count'];
  $invoice_items_table_rows = $items['rows'];
  $totals = array('extotal' => null, 'inctotal' => null, 'vattotal' => null);

  // If now rows are found add an empty row
  if ($count == 0) {
    $invoice_items_table_rows = array(
      array(
        'data' => array(array('data' => t('Empty') . '...', 'colspan' => '8')),
        'class' => array('class' => 'invoice-items-empty')
      ),
    );
  }
  else {
    // Count the added items
    $totals = _invoice_get_invoice_totals((int) $node->invoice['invoice_number'], $GLOBALS['user']->uid);
  }

  $variables = array(
    'header' => $invoice_items_table_header,
    'rows' => $invoice_items_table_rows,
    'sticky' => FALSE,
    'attributes' => array('id' => 'invoice-items-table')
  );
  $invoice_items_table = theme('table', $variables);

  $invoice_items_table_footer = '<tfoot><tr><td colspan="5"></td><td class="extotal">'
    . _invoice_round_and_format_money($totals['extotal'], 2) . '</td><td class="inctotal">'
    . _invoice_round_and_format_money($totals['inctotal'], 2) . '</td><td></td></tr></tfoot>';
  $invoice_items_table = str_replace('</table>', $invoice_items_table_footer . '</table>', $invoice_items_table);

  $form['invoice_items']['items'] = array(
    '#markup' => '<div class="invoice-items">' . $invoice_items_table . '</div>',
  );
  $form['invoice_items']['iid'] = array(
    '#type' => 'hidden',
    '#title' => t('Invoice item id'),
    '#required' => FALSE,
    '#attributes' => array('id' => 'edit-iid')
  );
  $form['invoice_items']['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#required' => FALSE,
    '#rows' => 1,
  );
  $form['invoice_items']['quantity'] = array(
    '#type' => 'textfield',
    '#title' => t('Quantity'),
    '#required' => FALSE,
    '#size' => 5,
  );
  $form['invoice_items']['price_without_vat'] = array(
    '#type' => 'textfield',
    '#title' => t('Price without VAT'),
    '#required' => FALSE,
    '#description' => t("If you don't fill in this field, you'll have to fill in \"Price with VAT\""),
  );
  $form['invoice_items']['price_with_vat'] = array(
    '#type' => 'textfield',
    '#title' => t('Price with VAT'),
    '#required' => FALSE,
    '#description' => t("If you don't fill in this field, you'll have to fill in \"Price without VAT\""),
  );
  $form['invoice_items']['vat'] = array(
    '#type' => 'textfield',
    '#title' => t('VAT percentage'),
    '#required' => FALSE,
    '#attributes' => array('style' => 'width:40px;'), // size attribute didn't work here for some strange reason
    '#default_value' => _invoice_get_variable($active_template, 'vat'),
  );
  $form['invoice_items']['item_actions'] = array(
    '#markup' => '<input type="button" id="button-save-item" name="button_save_item" value="'
      . t('Add item') . '" />' . ' <input type="button" id="button-cancel-item" value="'
      . t('Cancel') . '" />',
  );

  $form['customer_optional'] = array(
    '#type' => 'fieldset',
    '#title' => t('Customer details') . ' (' . t('optional') . ')',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 3,
  );
  $form['customer_optional']['street'] = array(
    '#type' => 'textfield',
    '#title' => t('Street'),
    '#required' => FALSE,
    '#default_value' => $node->customer['street'],
  );
  $form['customer_optional']['building_number'] = array(
    '#type' => 'textfield',
    '#title' => t('Building number'),
    '#required' => FALSE,
    '#default_value' => $node->customer['building_number'],
  );
  $form['customer_optional']['zipcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Zipcode'),
    '#required' => FALSE,
    '#default_value' => $node->customer['zipcode'],
  );
  $form['customer_optional']['city'] = array(
    '#type' => 'textfield',
    '#title' => t('City'),
    '#required' => FALSE,
    '#default_value' => $node->customer['city'],
  );
  $form['customer_optional']['state'] = array(
    '#type' => 'textfield',
    '#title' => t('State'),
    '#required' => FALSE,
    '#default_value' => $node->customer['state'],
  );
  $form['customer_optional']['country'] = array(
    '#type' => 'textfield',
    '#title' => t('Country'),
    '#required' => FALSE,
    '#default_value' => $node->customer['country'],
  );
  $form['customer_optional']['coc_number'] = array(
    '#type' => 'textfield',
    '#title' => t('Chamber of Commerce number'),
    '#required' => FALSE,
    '#default_value' => $node->customer['coc_number'],
  );
  $form['customer_optional']['vat_number'] = array(
    '#type' => 'textfield',
    '#title' => t('VAT number'),
    '#required' => FALSE,
    '#default_value' => $node->customer['vat_number'],
  );
  $form['customer_optional']['customer_description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#required' => FALSE,
    '#default_value' => $node->customer['description'],
  );

  $form['invoice_details'] = array(
    '#type' => 'fieldset',
    '#title' => t('Invoice details') . ' (' . t('optional') . ')',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 5,
  );

  // Only display this form field when creating a node
  if (empty($node->invoice['invoice_number'])) {
    $form['invoice_details']['user_defined_invoice_number'] = array(
      '#type' => 'textfield',
      '#title' => t('User defined invoice number'),
      '#required' => FALSE,
      '#default_value' => '',
      // size attribute didn't work here for some strange reason
      '#attributes' => array('style' => 'width:200px;'),
      '#description' => t('You can define an invoice number here. The number has to be higher than'
        . ' the latest invoice number though. It also has to be numeric.'),
    );
  }
  $form['invoice_details']['invoice_number'] = array(
    '#type' => 'hidden',
    '#title' => t('Invoice number'),
    '#required' => FALSE,
    '#default_value' => $node->invoice['invoice_number'],
    '#attributes' => array('id' => 'edit-invoice-number')
  );
  $form['invoice_details']['pay_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Pay limit'),
    '#required' => FALSE,
    '#default_value' => empty($node->invoice['pay_limit']) ?
      _invoice_get_variable($active_template, 'pay_limit') : $node->invoice['pay_limit'],
    '#description' => t('Pay limit in days'),
    // size attribute didn't work here for some strange reason
    '#attributes' => array('style' => 'width:40px;'),
  );
  $form['invoice_details']['invoice_description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#required' => FALSE,
    '#default_value' => $node->invoice['description'],
  );
  $form['invoice_details']['invoice_invoice_number_zerofill'] = array(
    '#type' => 'textfield',
    '#title' => t('Invoice number zerofill'),
    '#required' => FALSE,
    '#default_value' => empty($node->invoice['invoice_number_zerofill']) && $mode == 'create' ? variable_get('invoice_invoice_number_zerofill', 0) : $node->invoice['invoice_number_zerofill'],
    '#attributes' => array('style' => 'width:40px;'), // size attribute didn't work here for some strange reason
    '#description' => t('If you want an invoice number to be displayed as "0001" fill in 4. If you just want to display invoice number "1" leave/set empty.'),
  );
  $form['invoice_details']['invoice_invoice_number_prefix'] = array(
    '#type' => 'textfield',
    '#title' => t('Invoice number prefix'),
    '#required' => FALSE,
    '#default_value' => empty($node->invoice['invoice_number_prefix']) && $mode == 'create' ? variable_get('invoice_invoice_number_prefix', '') : $node->invoice['invoice_number_prefix'],
    '#attributes' => array('style' => 'width:150px;'), // size attribute didn't work here for some strange reason
    '#description' => t('If you want an invoice number to be displayed as "@year0001" fill in "%Y". Fillin 4 in the zerofill field above for extra zero values.', array('@year' => date('Y')))
    . ' ' . t('If a new year is reached the numbering will still continue sequentially, so if the year ended with "!year0578", the next year will start with "!next_year0579"', array('!year' => date('Y'), '!next_year' => date('Y') + 1))
    . ' ' . t('All !date values may be entered here with a "%" sign before it or every other text you like.', array('!date' => l(t('date'), 'http://www.php.net/date', array('absolute' => TRUE)))),
  );

  return $form;
}

/**
 * Invoice settings form
 */
function invoice_settings_form() {

  // Get template names
  $templates = _invoice_get_templates();

  $form['general'] = array(
    '#type' => 'fieldset',
    '#title' => t('General settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['general']['locale'] = array(
    '#type' => 'textfield',
    '#title' => t('Locale'),
    '#required' => TRUE,
    '#default_value' => variable_get('invoice_locale', ''),
    '#description' => t('Category/locale names can be found in !rfc1766 and !iso639. Systems can have different naming schemes for locales.',
      array(
        '!rfc1766' => l(t('» RFC 1766'), 'http://www.faqs.org/rfcs/rfc1766', array('absolute' => TRUE)),
        '!iso639' => l(t('» ISO 639'), 'http://www.w3.org/WAI/ER/IG/ert/iso639.htm', array('absolute' => TRUE)),
      )
    ) . ' ' . t('On linux you can check the available locales on the server with the command "locale -a" or click below to see the same list.')
    . ' ' . t('If your system/server is ubuntu (debian like) you can install more languages with the aptitude or synaptic package manager. Search for -language-pack-*-base.')
    . ' ' . t('If you install for example -language-pack-en-base you get over 10+ locales extra, like en_US, en_GB, en_AU, en_CA etc.')
    . ' ' . t('For more information see !link.', array('!link' => l(t('http://www.php.net/setlocale'), 'http://www.php.net/setlocale', array('absolute' => TRUE))))
    . ' ' . l(t('Click here to see an overview of installed locales on your system.'), 'invoice/installed_locales'),
    '#size' => 8,
  );
  $form['general']['date_format'] = array(
    '#type' => 'textfield',
    '#title' => t('Date format'),
    '#required' => TRUE,
    '#default_value' => variable_get('invoice_date_format', ''),
    '#description' => t('For example m/d/Y.') . ' ' . t('See !link.', array('!link' => l(t('http://www.php.net/date'), 'http://www.php.net/date', array('absolute' => TRUE)))) . ' ' . t('The date on the invoice will look like: @date_format', array('@date_format' => date(variable_get('invoice_date_format', '')))),
    '#size' => 20,
  );
  $form['general']['vat'] = array(
    '#type' => 'textfield',
    '#title' => t('Default VAT precentage'),
    '#required' => TRUE,
    '#default_value' => variable_get('invoice_vat', ''),
    '#size' => 3,
  );
  $form['general']['pay_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Pay limit'),
    '#required' => TRUE,
    '#default_value' => variable_get('invoice_pay_limit', ''),
    '#description' => t('Pay limit in days'),
    '#size' => 3,
  );
  $form['general']['invoice_number_zerofill'] = array(
    '#type' => 'textfield',
    '#title' => t('Invoice number zerofill'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_invoice_number_zerofill', ''),
    '#description' => t('If you want an invoice number to be displayed as "0001" fill in 4. If you just want to display invoice number "1" leave/set empty.'),
    '#size' => 3,
  );
  $form['general']['invoice_number_prefix'] = array(
    '#type' => 'textfield',
    '#title' => t('Invoice number prefix'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_invoice_number_prefix', ''),
    '#description' => t('If you want an invoice number to be displayed as "@year0001" fill in "%Y". Fillin 4 in the zerofill field above for extra zero values.', array('@year' => date('Y')))
    . ' ' . t('If a new year is reached the numbering will still continue sequentially, so if the year ended with "@year0578", the next year will start with "@next_year0579"', array('@year' => date('Y'), '@next_year' => date('Y') + 1))
    . ' ' . t('All !date values may be entered here with a "%" sign before it or every other text you like.', array('!date' => l(t('date'), 'http://www.php.net/date', array('absolute' => TRUE)))),
    '#size' => 20,
  );

  /* ------------------------------------------------------ */

  $form['general']['display_column'] = array(
    '#type' => 'fieldset',
    '#title' => t('Display invoice columns'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['general']['display_column']['display_column_vat'] = array(
    '#type' => 'checkbox',
    '#title' => t('VAT'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_display_column_vat', ''),
  );
  $form['general']['display_column']['display_column_exunitcost'] = array(
    '#type' => 'checkbox',
    '#title' => t('Unitcost (ex)'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_display_column_exunitcost', ''),
  );
  $form['general']['display_column']['display_column_incunitcost'] = array(
    '#type' => 'checkbox',
    '#title' => t('Unitcost (inc)'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_display_column_incunitcost', ''),
  );
  $form['general']['display_column']['display_column_extotal'] = array(
    '#type' => 'checkbox',
    '#title' => t('Total (ex)'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_display_column_extotal', ''),
  );
  $form['general']['display_column']['display_column_inctotal'] = array(
    '#type' => 'checkbox',
    '#title' => t('Total (inc)'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_display_column_inctotal', ''),
  );

  /* ------------------------------------------------------ */

  $form['general']['supplier'] = array(
    '#type' => 'fieldset',
    '#title' => t('Supplier details'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['general']['supplier']['supplier_company_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Company name'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_company_name', ''),
  );
  $form['general']['supplier']['supplier_street'] = array(
    '#type' => 'textfield',
    '#title' => t('Street'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_street', ''),
  );
  $form['general']['supplier']['supplier_building_number'] = array(
    '#type' => 'textfield',
    '#title' => t('Building number'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_building_number', ''),
  );
  $form['general']['supplier']['supplier_zipcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Zipcode'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_zipcode', ''),
  );
  $form['general']['supplier']['supplier_city'] = array(
    '#type' => 'textfield',
    '#title' => t('City'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_city', ''),
  );
  $form['general']['supplier']['supplier_state'] = array(
    '#type' => 'textfield',
    '#title' => t('State'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_state', ''),
  );
  $form['general']['supplier']['supplier_country'] = array(
    '#type' => 'textfield',
    '#title' => t('Country'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_country', ''),
  );
  $form['general']['supplier']['supplier_phone'] = array(
    '#type' => 'textfield',
    '#title' => t('Phone'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_phone', ''),
  );
  $form['general']['supplier']['supplier_fax'] = array(
    '#type' => 'textfield',
    '#title' => t('Fax'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_fax', ''),
  );
  $form['general']['supplier']['supplier_email'] = array(
    '#type' => 'textfield',
    '#title' => t('Email'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_email', ''),
  );
  $form['general']['supplier']['supplier_web'] = array(
    '#type' => 'textfield',
    '#title' => t('Web address'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_web', ''),
  );
  $form['general']['supplier']['supplier_coc_number'] = array(
    '#type' => 'textfield',
    '#title' => t('CoC Number'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_coc_number', ''),
  );
  $form['general']['supplier']['supplier_vat_number'] = array(
    '#type' => 'textfield',
    '#title' => t('VAT Number'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_supplier_vat_number', ''),
  );

  /* ------------------------------------------------------ */

  $form['general']['api'] = array(
    '#type' => 'fieldset',
    '#title' => t('API details'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['general']['api']['api_root_username'] = array(
    '#type' => 'textfield',
    '#title' => t('API root username'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_api_root_username', ''),
    '#description' => t('The username defined here has access to perform all invoice API operations. But you can also'
        . ' only set an API username per template or both.')
  );
  $form['general']['api']['api_allowed_ips'] = array(
    '#type' => 'textfield',
    '#title' => t('Allowed ips'),
    '#required' => FALSE,
    '#default_value' => variable_get('invoice_api_allowed_ips', ''),
    '#description' => t('Restrict authentication access to these ips (multiple comma seperated)')
  );

  // Build array for selecting the default template
  $default_templates = array();
  foreach ($templates as $template) {
    $default_templates[$template] = ucfirst($template);
  }

  $form['general']['default_template'] = array(
    '#type' => 'select',
    '#title' => t('Default template'),
    '#options' => $default_templates,
    '#default_value' => variable_get('invoice_default_template', 'default'),
    '#required' => TRUE,
    '#size' => 1,
  );

  /* ------------------------------------------------------ */
  // Build form for template values

  foreach ($templates as $template) {
    $form[$template] = array(
      '#type' => 'fieldset',
      '#title' => t('Template') . ' (' . $template . ')',
      '#collapsible' => TRUE,
      '#collapsed' => $template == 'default' ? FALSE : TRUE,
      '#description' => t('If fields are also set in invoice general settings and the template field is empty, the general setting of the field will be used.'),
    );
    $form[$template][$template . '_locale'] = array(
      '#type' => 'textfield',
      '#title' => t('Locale'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'locale', ''),
      '#description' => t('Category/locale names can be found in !rfc1766 and !iso639. Systems can have different naming schemes for locales.',
        array(
          '!rfc1766' => l(t('» RFC 1766'), 'http://www.faqs.org/rfcs/rfc1766', array('absolute' => TRUE)),
          '!iso639' => l(t('» ISO 639'), 'http://www.w3.org/WAI/ER/IG/ert/iso639.htm', array('absolute' => TRUE)),
        )
      ) . ' ' . t('On linux you can check the available locales on the server with the command "locale -a" or click below to see the same list.')
      . ' ' . t('If your system/server is ubuntu (debian like) you can install more languages with the aptitude or synaptic package manager. Search for -language-pack-*-base.')
      . ' ' . t('If you install for example -language-pack-en-base you get over 10+ locales extra, like en_US, en_GB, en_AU, en_CA etc.')
      . ' ' . t('For more information see !link.', array('!link' => l(t('http://www.php.net/setlocale'), 'http://www.php.net/setlocale', array('absolute' => TRUE))))
      . ' ' . l(t('Click here to see an overview of installed locales on your system.'), 'invoice/installed_locales'),
      '#size' => 20,
    );
    $form[$template][$template . '_date_format'] = array(
      '#type' => 'textfield',
      '#title' => t('Date format'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'date_format', ''),
      '#description' => t('For example m/d/Y.') . ' ' . t('See !link.',
        array('!link' => l(t('http://www.php.net/date'), 'http://www.php.net/date',
          array('absolute' => TRUE)))) . ' ' .
            t('The date on the invoice will look like: @date_format',
              array('@date_format' => date(_invoice_get_variable($template, 'date_format')))),
      '#size' => 20,
    );
    $form[$template][$template . '_vat'] = array(
      '#type' => 'textfield',
      '#title' => t('Default vat percentage'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'vat', ''),
      '#size' => 3,
    );
    $form[$template][$template . '_pay_limit'] = array(
      '#type' => 'textfield',
      '#title' => t('Pay limit'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'pay_limit', ''),
      '#description' => t('Pay limit in days'),
      '#size' => 3,
    );

    /* ------------------------------------------------------ */

    $s_fieldset_name = '_display_column';

    $form[$template][$template . '_display_column'] = array(
      '#type' => 'fieldset',
      '#title' => t('Display invoice columns'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form[$template][$template . '_display_column'][$template . $s_fieldset_name . '_vat'] = array(
      '#type' => 'checkbox',
      '#title' => t('VAT'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'display_column_vat', ''),
    );
    $form[$template][$template . '_display_column'][$template . $s_fieldset_name . '_exunitcost'] = array(
      '#type' => 'checkbox',
      '#title' => t('Unitcost (ex)'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'display_column_exunitcost', ''),
    );
    $form[$template][$template . '_display_column'][$template . $s_fieldset_name . '_incunitcost'] = array(
      '#type' => 'checkbox',
      '#title' => t('Unitcost (inc)'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'display_column_incunitcost', ''),
    );
    $form[$template][$template . '_display_column'][$template . $s_fieldset_name . '_extotal'] = array(
      '#type' => 'checkbox',
      '#title' => t('Total (ex)'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'display_column_extotal', ''),
    );
    $form[$template][$template . '_display_column'][$template . $s_fieldset_name . '_inctotal'] = array(
      '#type' => 'checkbox',
      '#title' => t('Total (inc)'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'display_column_inctotal', ''),
    );

    /* ------------------------------------------------------ */

    $form[$template][$template . '_supplier'] = array(
      '#type' => 'fieldset',
      '#title' => t('Supplier details'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form[$template][$template . '_supplier'][$template . '_supplier_company_name'] = array(
      '#type' => 'textfield',
      '#title' => t('Company name'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'supplier_company_name', ''),
    );
    $form[$template][$template . '_supplier'][$template . '_supplier_street'] = array(
      '#type' => 'textfield',
      '#title' => t('Street'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'supplier_street', ''),
    );
    $form[$template][$template . '_supplier'][$template . '_supplier_building_number'] = array(
      '#type' => 'textfield',
      '#title' => t('Building number'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'supplier_building_number', ''),
    );
    $form[$template][$template . '_supplier'][$template . '_supplier_zipcode'] = array(
      '#type' => 'textfield',
      '#title' => t('Zipcode'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'supplier_zipcode', ''),
    );
    $form[$template][$template . '_supplier'][$template . '_supplier_city'] = array(
      '#type' => 'textfield',
      '#title' => t('City'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'supplier_city', ''),
    );
    $form[$template][$template . '_supplier'][$template . '_supplier_state'] = array(
      '#type' => 'textfield',
      '#title' => t('State'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'supplier_state', ''),
    );
    $form[$template][$template . '_supplier'][$template . '_supplier_country'] = array(
      '#type' => 'textfield',
      '#title' => t('Country'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'supplier_country', ''),
    );
    $form[$template][$template . '_supplier'][$template . '_supplier_phone'] = array(
      '#type' => 'textfield',
      '#title' => t('Phone'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'supplier_phone', ''),
    );
    $form[$template][$template . '_supplier'][$template . '_supplier_fax'] = array(
      '#type' => 'textfield',
      '#title' => t('Fax'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'supplier_fax', ''),
    );
    $form[$template][$template . '_supplier'][$template . '_supplier_email'] = array(
      '#type' => 'textfield',
      '#title' => t('Email'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'supplier_email', ''),
    );
    $form[$template][$template . '_supplier'][$template . '_supplier_web'] = array(
      '#type' => 'textfield',
      '#title' => t('Web address'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'supplier_web', ''),
    );
    $form[$template][$template . '_supplier'][$template . '_supplier_coc_number'] = array(
      '#type' => 'textfield',
      '#title' => t('CoC number'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'supplier_coc_number', ''),
    );
    $form[$template][$template . '_supplier'][$template . '_supplier_vat_number'] = array(
      '#type' => 'textfield',
      '#title' => t('VAT number'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'supplier_vat_number', ''),
    );

    /* ------------------------------------------------------ */

    $form[$template][$template . '_api'] = array(
      '#type' => 'fieldset',
      '#title' => t('API details'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form[$template][$template . '_api'][$template . '_api_username'] = array(
      '#type' => 'textfield',
      '#title' => t('Username'),
      '#required' => FALSE,
      '#default_value' => _invoice_get_variable($template, 'api_username', ''),
      '#description' => t('Username must exist in the database, the corresponding password is used')
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

/**
 * Form helper function to get all invoice items
 *
 * @param integer $invoice_id
 *
 * @return array
 */
function _invoice_get_invoice_items($invoice_id = 0) {
  $i = 0;
  $invoice_id = intval($invoice_id);

  $invoice_items_table_rows = array();
  $sql_addition = $invoice_id > 0 ? ' ' . intval($invoice_id) : '';

  $result = db_query("SELECT * FROM {invoice_items} WHERE uid = :uid AND invoice_id = :invoice_id
    ORDER BY weight, created ASC", array(
    ':uid' => $GLOBALS['user']->uid,
    ':invoice_id' => $invoice_id
  ))->fetchAll();

  $form['page_names'] = array();

  foreach ($result as $row) {
    $i++;
    $invoice_items_table_rows[] = array(
      'data' => array(
        nl2br($row->description),
        $row->vat . '%',
        $row->quantity,
        _invoice_round_and_format_money($row->unitcost, 3),
        _invoice_round_and_format_money($row->unitcost * _invoice_vat_percent_to_decimal($row->vat), 2),
        _invoice_round_and_format_money($row->quantity * $row->unitcost, 2),
        _invoice_round_and_format_money($row->quantity * $row->unitcost
          * _invoice_vat_percent_to_decimal($row->vat), 2),
        array('data' =>
          _invoice_get_icon('edit', NULL, array(
            'class' => 'action-button edit-action mouse-pointer',
            'title' => t('Edit'),
          ))
          .
          _invoice_get_icon('delete', NULL, array(
            'class' => 'action-button delete-action mouse-pointer',
            'title' => t('Delete'),
          )),
          'class' => array('actions')
        ),
      ),
      'class' => array('item-' . $row->iid, 'invoice-item', 'draggable'),
    );
  }

  return array('rows' => $invoice_items_table_rows, 'count' => $i);
}

/**
 * Implements hook_form_alter()
 *
 * @param array $form
 * @param array $form_state
 * @param string $form_id
 */
function invoice_form_alter(&$form, &$form_state, $form_id) {
  if ($form_id == 'invoice_node_form') {
    unset($form['revision_information']);
    unset($form['comment_settings']);
    unset($form['options']['promote']);
    unset($form['options']['sticky']);
    unset($form['actions']['preview']);
  }

  if ($form_id == 'node_type_form' && $form['#node_type']->type == 'invoice') {
    unset($form['submission']['node_preview']);
    unset($form['workflow']['node_options']['#options']['promote']);
    unset($form['workflow']['node_options']['#options']['sticky']);
    unset($form['workflow']['node_options']['#options']['revision']);
    unset($form['comment']);
  }
}