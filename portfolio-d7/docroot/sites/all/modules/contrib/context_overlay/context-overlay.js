(function ($) {

  // Adjust the overlay dimensions.
  Drupal.behaviors.myModule = {
    
    attach: function (context, settings) {
      $('#overlay:not(.mymodule-adjusted)', context).each(function() {
        if (settings.context_overlay){
          if (settings.context_overlay.width){
            $(this).css({
              'width'     : settings.context_overlay.width,
              'min-width' : settings.context_overlay.width
            });
          }
          if (settings.context_overlay.margin_top){
            $(this).css('margin-top', settings.context_overlay.margin_top);
          }
          if (settings.context_overlay.hide_title){
            $('#overlay-title').hide();
          }
          if (settings.context_overlay.hide_add_shortcut){  
            $('.add-or-remove-shortcuts', this).hide();  // hide "add short-cut" button
          }
          if (settings.context_overlay.hide_branding){
            $('#branding', this).hide();  // hide branding container
          }
          
        }
      }).addClass('mymodule-adjusted');
    }
    
  };

})(jQuery);
