(function ($) {
Drupal.behaviors.mec_lang_link = {
  attach: function (context, settings) {
    
    var settings = settings || Drupal.settings;
    $('#edit-mec-lang-link-select').change(function() {
      document.location.href = this.options[this.selectedIndex].value;
    });
    
    if (settings.mec_lang_link) {
      var flags = settings.mec_lang_link.jsWidget.languageicons;
      if (flags) {
      $.each(flags, function(index, value) {
        $('#edit-mec-lang-link-select option[value=' + index + ']').attr('title', value);
      });
      }
      var msddSettings = settings.mec_lang_link.jsWidget;
      
      $('#edit-mec-lang-link-select').msDropDown({
        visibleRows: msddSettings.visibleRows,
        rowHeight: msddSettings.rowHeight,
        animStyle: msddSettings.animStyle
      });
    }
  }
};
})(jQuery);
