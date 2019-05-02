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
 * Function to create the invoice layout
 *
 * Documentation available at:
 * - http://www.digitaljunkies.ca/dompdf/faq.php
 * - http://www.digitaljunkies.ca/dompdf/usage.php
 */
function _invoice_default_get_template_output($invoice, $type = NULL) {
  ob_start();

  // Create alias
  $template =& $invoice->invoice['template'];

  // Set full url path to template
  $path_to_template = _invoice_get_transfer_protocol() . '://' . $_SERVER['HTTP_HOST'] . base_path() . path_to_theme('module', 'invoice') . '/templates';

  if ($type != 'pdf') {
    $pagenum = '1/1';
  }
  else {
    $pagenum = '';
  }

  $footer_content = '' . _invoice_get_variable($template, 'supplier_company_name')
    . ' | Acc. 12345, Foobank Foocity | CoC nr. ' . _invoice_get_variable($template, 'supplier_coc_number')
    . ' | VAT nr. ' . _invoice_get_variable($template, 'supplier_vat_number') . '';

  if ($type == 'pdf') {
    $footer = '<script type="text/php">
                if (isset($pdf)) {
                  // Open the object: all drawing commands will
                  // go to the object instead of the current page
                  $footer = $pdf->open_object();

                  // Get page width and height
                  $w = $pdf->get_width();
                  $h = $pdf->get_height();

                  // Get font
                  $font = Font_Metrics::get_font("arial");

                  // Set RGB line color
                  $r=175; $g=175; $b=175;

                  // Leave this array alone
                  $a_line_color = array(0 => $r / 0xff, 1 => $g / 0xff, 2 => $b / 0xff,
                    "r"=>$r / 0xff, "g"=>$g / 0xff, "b"=>$b / 0xff,
                    "hex" => sprintf("#%02X%02X%02X", $r, $g, $b));

                  // Set RGB text color
                  $r=119; $g=119; $b=119;

                  // Leave this array alone
                  $a_text_color = array(0 => $r / 0xff, 1 => $g / 0xff, 2 => $b / 0xff,
                    "r"=>$r / 0xff, "g"=>$g / 0xff, "b"=>$b / 0xff,
                    "hex" => sprintf("#%02X%02X%02X", $r, $g, $b));

                  // Set line
                  $pdf->line(34, $h-40, $w-34, $h-40, $a_line_color, 1, array());

                  // Set page text
                  $pdf->text(40, $h-39, "' . $footer_content . '", $font, 10, $a_text_color);

                  $pdf->page_text($w-56, $h-39, "{PAGE_NUM}/{PAGE_COUNT}", $font, 10, $a_text_color);

                  // Close the object (stop capture)
                  $pdf->close_object();

                  // Add the object to every page. You can
                  // also specify "odd" or "even"
                  $pdf->add_object($footer, "all");
                }
                </script>';
    echo $footer;
    $footer = '';
  }
  else {
    $footer = $footer_content;
  }

  // Invoice header
  echo '
  <table class="invoice-header">
  <thead>
  <tr class="one">
    <td class="one">
     <h1>' . t('Invoice') . '</h1>
     <div class="client-wrapper">';
  if (!empty($invoice->customer['company_name'])) {
    echo '<div class="company-name break">' . $invoice->customer['company_name'] . '</div>';
  }
  if (!empty($invoice->customer['fullname'])) {
    echo '<div class="fullname ' . (empty($invoice->customer['company_name']) ? 'break' : '') . '">' . $invoice->customer['fullname'] . '</div>';
  }

  echo '
       <div class="address"><span class="street">' . $invoice->customer['street'] . '</span> <span class="building-number">' . $invoice->customer['building_number'] . '</span></div>
       <div class="zipcode-city"><span class="zipcode">' . $invoice->customer['zipcode'] . '</span> <span class="city">' . $invoice->customer['city'] . '</span></div>
       <div class="state">' . $invoice->customer['state'] . '</div>
       <div class="country break">' . $invoice->customer['country'] . '</div>
       <div class="coc-number">' . (!empty($invoice->customer['coc_number']) ? t('C.o.C.') . ' ' . t('nr.') . ': ' . $invoice->customer['coc_number'] : '') . '</div>
       <div class="vat-number">' . (!empty($invoice->customer['vat_number']) ? t('VAT') . ' ' . t('nr.') . ': ' . $invoice->customer['vat_number'] : '') . '</div>
       <div class="description">' . $invoice->customer['description'] . '</div>
     </div>
    </td>
    <td class="two">

    </td>
    <td class="three">
     <div class="supplier-wrapper">
       <div class="company-name"><img id="invoice-supplier-logo" src="'. $path_to_template .'/default_logo.png" alt="'. _invoice_get_variable($template, 'supplier_company_name') .'" /></div>
       <div class="address"><span class="street">' . _invoice_get_variable($template, 'supplier_street') . '</span> <span class="building-number">' . _invoice_get_variable($template, 'supplier_building_number') . '</span></div>
       <div class="zipcode-city"><span class="zipcode">' . _invoice_get_variable($template, 'supplier_zipcode') . '</span> <span class="city">' . _invoice_get_variable($template, 'supplier_city') . '</span></div>
       <div class="state">' . _invoice_get_variable($template, 'supplier_state') . '</div>
       <div class="country break">' . _invoice_get_variable($template, 'supplier_country') . '</div>
       <div class="phone">' . _invoice_get_variable($template, 'supplier_phone') . '</div>
       <div class="fax">' . _invoice_get_variable($template, 'supplier_fax') . ' </div>
       <div class="email">' . _invoice_get_variable($template, 'supplier_email') . '</div>
       <div class="web">' . _invoice_get_variable($template, 'supplier_web') . '</div>
     </div>
    </td>
  </tr>
  </thead>
  </table>';

  // Invoice details
  echo '<table class="invoice-details">
    <thead>
    <tr>
      <td class="one">
        <strong>' . t('Date') . '</strong>
        <div>' . $invoice->invoice['formatted_created'] . '</div>
      </td>
      <td class="two">
        <strong>' . t('Invoice number') . '</strong>
        <div>' . $invoice->invoice['formatted_invoice_number'] . '</div>
      </td>
      <td class="three">
       <strong>' . t('Client number') . '</strong>
        <div>' . $invoice->customer['customer_number'] . '</div>
      </td>';

  if (!empty($pagenum)) {
    echo '<td class="four">
            <strong>' . t('Page') . '</strong>
            <div>
              ' . $pagenum . '
            </div>
          </td>';
  }
  echo '
      </tr>
      </thead>
    </table>';

  /* --------- invoice items header --------- */

  // If an invoice has items with different vat values this column must always be shown.
  $vat = NULL;
  $multiple_vat_values = FALSE;
  foreach ($invoice->invoice_items as $item) {
    // Set vat value for the first time
    if (is_null($vat)) {
      $vat = $item['vat'];
    }

    if ($item['vat'] !== $vat) {
      $multiple_vat_values = TRUE;
    }
  }

  // Invoice items
  echo '
    <table class="invoice-items">
      <tr class="one">';

  $colspan = 0;

  echo '<th class="description">' . t('Description') . '</th>';

  if (_invoice_get_variable($template, 'display_column_vat') || $multiple_vat_values === TRUE) {
    echo '<th>' . t('VAT') . '</th>';
    $colspan++;
  }

  echo '<th>' . t('Quantity') . '</th>';

  if (_invoice_get_variable($template, 'display_column_exunitcost')) {
    echo '<th>' . t('Unitcost') . ' (' . t('ex') . ')</th>';
    $colspan++;
  }

  if (_invoice_get_variable($template, 'display_column_incunitcost')) {
    echo '<th>' . t('Unitcost') . ' (' . t('inc') . ')</th>';
    $colspan++;
  }

  if (_invoice_get_variable($template, 'display_column_extotal')) {
    echo '<th>' . t('Total') . ' (' . t('ex') . ')</th>';
    $colspan++;
  }

  if (_invoice_get_variable($template, 'display_column_inctotal')) {
    echo '<th>' . t('Total') . ' (' . t('inc') . ')</th>';
    $colspan++;
  }

  echo '</tr>';
  /* --------------- end -------------------- */

  /* ------------ invoice items ------------- */

  $i = 0;
  foreach ($invoice->invoice_items as $item) {
    if ($i == 0) {
      $class = 'odd';
      $i++;
    }
    else {
      $class = 'even';
      $i = 0;
    }

    echo '<tr class="' . $class . '">';

    echo '<td class="description">' . $item['description'] . '</td>';

    if (_invoice_get_variable($template, 'display_column_vat') || $multiple_vat_values === TRUE) {
      echo '<td>' . $item['vat'] . '%</td>';
    }

    echo '<td>' . $item['quantity'] . '</td>';

    if (_invoice_get_variable($template, 'display_column_exunitcost')) {
      echo '<td>' . $item['formatted_exunitcost'] . '</td>';
    }

    if (_invoice_get_variable($template, 'display_column_incunitcost')) {
      echo '<td>' . $item['formatted_incunitcost'] . '</td>';
    }

    if (_invoice_get_variable($template, 'display_column_extotal')) {
      echo '<td>' . $item['formatted_extotal'] . '</td>';
    }

    if (_invoice_get_variable($template, 'display_column_inctotal')) {
      echo '<td>' . $item['formatted_inctotal'] . '</td>';
    }

    echo '</tr>';
  }
  /* --------------- end -------------------- */

  echo '
      <tr class="total">
        <td colspan="' . $colspan . '"></td>
        <td class="extotal">' . $invoice->invoice['formatted_extotal'] . '</td>
        <td class="inctotal">' . $invoice->invoice['formatted_inctotal'] . '</td>
      </tr>
    </table>
  ';

  // Invoice totals
  echo '<table class="invoice-totals">
      <tr class="extotal">
        <td class="one"></td>
        <td class="two">' . t('Subtotal excl. VAT') . ':</td>
        <td class="three">' . $invoice->invoice['formatted_extotal'] . '</td>
     </tr>
  ';

  foreach ($invoice->invoice['vat'] as $percentage => $vat) {
    echo '<tr class="vat">
            <td class="one"></td>
            <td class="two">' . t('VAT') . ' ' . $percentage . '%:</td>
            <td class="three">' . $vat['formatted_vattotal'] . '</td>
          </tr>';
  }

  echo '
    <tr class="inctotal">
      <td class="one"></td>
      <td class="two">' . t('Total incl. VAT') . ':</td>
      <td class="three"><div class="inctotal"><div class="inctotal inner">' . $invoice->invoice['formatted_inctotal'] . '</div></div></td>
    </tr>
  </table>';

  // Invoice footer
  echo '<table class="invoice-footer">
    <tbody>
      <tr>
      <td colspan="3" class="additional-information">
        <div class="invoice-description">' . nl2br($invoice->invoice['description']) . '';

  if ($invoice->invoice['pay_limit'] > 0) {
    echo '<div class="pay-limit">' . t('We kindly ask you to pay this invoice within @days days.', array('@days' => $invoice->invoice['pay_limit']));
  } else {
    echo '<div class="pay-limit">' . t('This invoice is already paid.') . '</div>';
  }

  echo '</td>
      </tr>
    </tbody>
    <tfoot>
      <tr>
        <td colspan="3" class="footer">
          ' . $footer . '
        </td>
      </tr>
    </tfoot>
  </table>';

  $content = ob_get_contents();
  ob_end_clean();
  return $content;
}
