<?php

if (isset($view->override_path)) {
  // inside live preview
  print htmlspecialchars($xml);
}
elseif (isset($options['using_views_api_mode']) && $options['using_views_api_mode']) {
  //TODO: Code below is not executed
  // We're in Views API mode.
  print "DOT file <xmp>" . $xml . '</xmp>';
}
else {
  // TODO: this could be a json callback
  //drupal_add_http_header("Content-Type", "$content_type; charset=utf-8");
  print "DOT file <xmp>" . $xml . '</xmp>';
  //exit;
}