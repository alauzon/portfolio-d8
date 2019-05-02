var invoice = null;

jQuery(document).ready(function($){

  invoice = {

    /**
     * Default VAT (Value Added Tax)
     *
     * @param {float}
     */
    defaultVat: null,

    /**
     * Default value of the button with ID "button-save-item"
     *
     * @param {string}
     */
    defaultButtonSaveItemValue: null,

    /**
     * Initialization
     */
    init: function() {
      if ($('#invoice-node-form').length) {
        this.setDefaultValues();
        this.bindChangeEventToTemplateSwitch();
        this.bindBlurEventToCustomerSearchField();
        this.bindClickEventToItemActionButtons();
        this.bindClickEventToButtonCancelItem();
        this.bindClickEventToButtonSaveItem();
        this.setItemVatConstraints();
        this.convertFormSubmitButtonsToNormal();
      }
    },

    /**
     * Sets the default values
     */
    setDefaultValues: function() {
      if (parseFloat($('#edit-vat').val()) > 0) {
        this.defaultVat = parseFloat($('#edit-vat').val());
      }

      this.defaultButtonSaveItemValue = $('#button-save-item').val();
    },

    bindChangeEventToTemplateSwitch: function() {
      $('#edit-template').change(function() {
        invoice.setTemplate($(this).val());
      });
    },

    /**
     * Binds the blur event to the customer search field
     */
    bindBlurEventToCustomerSearchField: function() {
      $('#edit-search').blur(function(){
        if ($("#edit-search").val().length > 0 && $('#edit-search').val() > 0) {
          invoice.getCustomerInfo($('#edit-search').val());
        }
        else if ($('#edit-search').val().length < 1) {
          invoice.getCustomerInfo('set_empty');
        }
      });
    },

    /**
     * Binds the click event to the action buttons of an invoice item
     */
    bindClickEventToItemActionButtons: function() {
      $('#invoice-items-table .invoice-item').each(function() {
        var itemId = invoice.getItemIdFromInvoiceItemRow(this);
        invoice.bindClickEventToActionButtonsForItem(itemId);
      });
    },

    /**
     * Binds a click event to the action buttons of an invoice item
     *
     * @param {integer} itemId Invoice item ID
     */
    bindClickEventToActionButtonsForItem: function(itemId) {
      $('.edit-action', '#invoice-items-table .item-'+ itemId).bind('click', {itemId: itemId}, invoice.editItemHandler);
      $('.delete-action', '#invoice-items-table .item-'+ itemId).bind('click', {itemId: itemId}, invoice.deleteItemHandler);
    },

    /**
     * Binds the click event to the button cancel item
     */
    bindClickEventToButtonCancelItem: function() {
      // Add click event to button-cancel-item
      $('#button-cancel-item').click(function() {
        invoice.resetItemForm();
        $('#edit-description').focus();
        return false;
      });
    },

    /**
     * Binds the click event to the button save item
     */
    bindClickEventToButtonSaveItem: function() {
      // Add click event to button-save-item
      $('#button-save-item').click(this.saveItemHandler);
    },

    /**
     * Sets the VAT constraints for an invoice item
     */
    setItemVatConstraints: function() {
      // Either the price whithout VAT is filled in or the price whith VAT
      $('#edit-price-without-vat').change(function() {
        $('#edit-price-with-vat').val('');
      });
      $('#edit-price-with-vat').change(function() {
        $('#edit-price-without-vat').val('');
      });
    },

    /**
     * Sets the invoice template
     *
     * @param {string} value Template name
     */
    setTemplate: function(value) {
      $.get(invoice.getUrl('invoice/set/template'),
        {value: value
        },
        function(data) {
          if (data['error'] != undefined && data['error'] != '') {
            alert(data['error']);
          }
          else {
            $('#edit-vat').val(data['vat']);
          }
        },
        'json'
      );
    },

    /**
     * Returns customer information by keyword
     *
     * @param {string} value (Part of) the customer name
     */
    getCustomerInfo: function(value) {
      $.get(invoice.getUrl('invoice/get/customer_info'),
        {value: value
        },
        function(data) {
          if (data['error'] != undefined && data['error'] != '') {
            alert(data['error']);
          }
          else if (value == 'set_empty' || (data['set_empty'] != undefined && data['set_empty'] != '')) {
            $('#edit-company-name').val('');
            $('#edit-firstname').val('');
            $('#edit-lastname').val('');
            $('#edit-street').val('');
            $('#edit-building-number').val('');
            $('#edit-zipcode').val('');
            $('#edit-city').val('');
            $('#edit-state').val('');
            $('#edit-country').val('');
            $('#edit-customer-description').val('');
            $('#edit-coc-number').val('');
            $('#edit-vat-number').val('');
          }
          else {
            $('#edit-search').val(data['search_customer']);
            $('#edit-company-name').val(data['company_name']);
            $('#edit-firstname').val(data['firstname']);
            $('#edit-lastname').val(data['lastname']);
            $('#edit-street').val(data['street']);
            $('#edit-building-number').val(data['building_number']);
            $('#edit-zipcode').val(data['zipcode']);
            $('#edit-city').val(data['city']);
            $('#edit-state').val(data['state']);
            $('#edit-country').val(data['country']);
            $('#edit-coc-number').val(data['coc_number']);
            $('#edit-vat-number').val(data['vat_number']);
            if (window.mceToggle) {mceToggle('edit-customer-description', 'wysiwyg4customer-description');}
            $('#edit-customer-description').val(data['description']);
            if (window.mceToggle) {mceToggle('edit-customer-description', 'wysiwyg4customer-description');}
          }
        },
        'json'
      );
    },

    /**
     * Converts the submit buttons of the form to normal buttons to prevent
     * form submission with the enter key
     */
    convertFormSubmitButtonsToNormal: function() {
      var context = '#invoice-node-form';
      var button = '';
      button = '<input id="edit-button" type="button" name="op" value="'+ $('#edit-submit').val() +'" class="form-submit" />';
      $('#edit-submit', context).replaceWith(button);
      $('#edit-button', context).attr('id', 'edit-submit');

      button = '<input id="edit-button" type="button" name="op" value="'+ $('#edit-delete').val() +'" class="form-submit" />';
      $('#edit-delete', context).replaceWith(button);
      $('#edit-button', context).attr('id', 'edit-delete');

      $(context).prepend('<input id="edit-op" type="hidden" name="op" />');

      // Set form submit action
      $('.form-submit', context).click(function() {
        $('#edit-op', context).val($(this).val());
        $(context).submit();
      });
    },

    /**
     * Saves an invoice item
     */
    saveItemHandler: function() {
      $.post(invoice.getUrl('invoice/save/item'),
        {
          iid: $('#edit-iid').val(),
          invoice_number: $('#edit-invoice-number').val(),
          description: $('#edit-description').val(),
          quantity: $('#edit-quantity').val(),
          price_without_vat: $('#edit-price-without-vat').val(),
          price_with_vat: $('#edit-price-with-vat').val(),
          vat: $('#edit-vat').val()
        },
        function(data) {
          if (data['error'] != undefined && data['error'] != '') {
            alert(data['error']);
          }
          else {
            if (data['remove_empty_row'] != undefined && data['remove_empty_row'] != '') {
              $('.invoice-items-empty').remove();
            }

            // If invoice item id is not empty we just changed an invoice item, so update the row
            if (data['iid'] != undefined && data['iid'] != '') {
              $('.invoice-items .item-'+ data['iid'] +' td:nth-child(1)').html(data['description']);
              $('.invoice-items .item-'+ data['iid'] +' td:nth-child(2)').html(data['vat']);
              $('.invoice-items .item-'+ data['iid'] +' td:nth-child(3)').html(data['quantity']);
              $('.invoice-items .item-'+ data['iid'] +' td:nth-child(4)').html(data['exunitcost']);
              $('.invoice-items .item-'+ data['iid'] +' td:nth-child(5)').html(data['incunitcost']);
              $('.invoice-items .item-'+ data['iid'] +' td:nth-child(6)').html(data['exsubtotal']);
              $('.invoice-items .item-'+ data['iid'] +' td:nth-child(7)').html(data['incsubtotal']);
            }
            else {
              $('#invoice-items-table').append(data['content']);
              var itemId = invoice.getItemIdFromInvoiceItemRow($('#invoice-items-table .invoice-item:last-child'));
              invoice.bindClickEventToActionButtonsForItem(itemId);
            }

            // Reset invoice item form
            invoice.resetItemForm();
            $('#edit-description').focus();

            // Set new totals
            $('.invoice-items .extotal').html(data['extotal']);
            $('.invoice-items .inctotal').html(data['inctotal']);
          }
        },
        'json'
      );
    },

    /**
     * Resets the invoice item form
     */
    resetItemForm: function() {
      $('#edit-iid').val('');
      $('#edit-description').val('');
      $('#edit-quantity').val('');
      $('#edit-price-without-vat').val('');
      $('#edit-price-with-vat').val('');
      $('#edit-vat').val(invoice.defaultVat);
      $('#button-save-item').val(invoice.defaultButtonSaveItemValue);
    },

    /**
     * Handler for retrieving the data of an invoice item to edit
     *
     * @param {object} event jQuery event
     */
    editItemHandler: function(event) {
      var itemId = event.data.itemId;

      $.get(invoice.getUrl('invoice/edit/item'),
        {iid: itemId,
          invoice_number: $('#edit-invoice-number').val()
        },
        function(data) {
          if (data['error'] != undefined && data['error'] != '') {
            alert(data['error']);
          }
          else {
            $('#edit-iid').val(itemId);
            $('#edit-description').val(data['description']);
            $('#edit-vat').val(data['vat']);
            $('#edit-quantity').val(data['quantity']);
            $('#edit-price-without-vat').val(data['exunitcost']);
            $('#edit-price-with-vat').val(data['incunitcost']);
            $('#button-save-item').val(data['actionvalue']);
          }
        },
        'json'
      );
    },

    /**
     * Handler for deleting the given invoice item ID
     *
     * @param {object} event jQuery event
     */
    deleteItemHandler: function(event) {
      var itemId = event.data.itemId;

      if (confirm(Drupal.t('Are you sure you want to delete this invoice item?'))) {
        $.get(invoice.getUrl('invoice/delete/item'),
          {iid: itemId,
            invoice_number: $('#edit-invoice-number').val()
          },
          function(data) {
            if (data['error'] != undefined && data['error'] != '') {
              alert(data['error']);
            }
            else {
              $('.invoice-items .item-'+ itemId).remove();
              $('.invoice-items .extotal').html(data['extotal']);
              $('.invoice-items .inctotal').html(data['inctotal']);
            }
          },
          'json'
        );
      }
    },

    /**
     * Returns the invoice item ID from an invoice item row
     *
     * @param {mixed} element DOM element | jQuery selector
     * @return integer
     */
    getItemIdFromInvoiceItemRow: function(element) {
      var itemId = null;

      var elementClasses = $(element).attr('class').split(' ');
      $.each(elementClasses, function(index, value) {
        if (value.match('item-')) {
          itemId = parseInt(value.replace('item-', ''), 10);
          return false;
        }
      });

      return itemId;
    },

    /**
     * Returns a full URL for a Drupal path
     *
     * @param {string} path Relative Drupal path
     */
    getUrl: function(path) {
      var protocol = window.location.href.split('://')[0];
      var url = protocol + '://' + Drupal.settings['invoice']['host'] + Drupal.settings['basePath'];
      if (!Drupal.settings['invoice']['clean_urls']) {
        url += '?q=';
      }
      url += path;
      return url;
    }
  }


  invoice.init();
});