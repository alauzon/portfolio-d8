<?php

/**
 * Field handler for textual properties.
 */
class efq_views_handler_field_property_text extends entity_views_handler_field_text {
  /**
   * Called to determine what to tell the clicksorter.
   */
  function click_sort($order) {
    $this->query->query->propertyOrderBy($this->real_field, $order);
  }

  /**
   * Override the parent's query method, since it doesn't need to do anything.
   */
  function query() {}
}
