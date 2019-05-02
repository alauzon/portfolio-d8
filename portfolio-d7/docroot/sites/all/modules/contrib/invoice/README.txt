/**
 * @file README.txt
 * @brief Little help file
 */

With this module you're able to create and administer invoices.

The module creates an invoice node type. The module can be used in combination with the path auto module to generate the title.

PDF document
It's possible to view the invoice as full node view on your site, as html print view and as pdf document. The nice dompdf library is used to generate pdf documents. The library is not included into this module. You'll have to download it yourself and paste the dompdf folder into the libraries folder, so you'll get "sites/all/libraries/dompdf".
The library can be downloaded from the official site, but has not the nice Arial font included by default. Fortunately, according to the End User License Agreement of Microsoft I have the right to redistribute Microsoft core fonts in unaltered form. So I already included the Arial font into the dompdf package. You can download this bundled package at: http://www.platinadesigns.nl/downloads/dompdf_0.5.1.tar.gz

Different templates
If your company has more than 1 brand name, it's possible to have different invoice templates. You have the possibility to have different company settings per template. Even the VAT (value added tax) value, date format and locale. With locale you're able to change the money format. For example en_US will give you the dollar symbol and nl_NL will give you the euro symbol. IMPORTANT: On probably all ubuntu servers the addition of ".utf8" should be used to get a working locale! So you get en_US.utf8.

To add a different template copy templates/default.inc and .css and create new files with different names. Then go to "admin/settings/invoice" and the template will automatically be visible.

This module is developed by P. Vogelaar of Platina Designs.

===========================================

Other comments:
* This module supports the pathauto module and is also recommended: http://drupal.org/project/pathauto
* Multiple invoice templates can be created (see above)  