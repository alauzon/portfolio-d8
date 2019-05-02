
(function ($) {
  Drupal.behaviors.field_boxes = { 
    attach: function(context, settings) {
    $(".field-boxes-table-order").tableDnD();
    }
  };
})(jQuery);
