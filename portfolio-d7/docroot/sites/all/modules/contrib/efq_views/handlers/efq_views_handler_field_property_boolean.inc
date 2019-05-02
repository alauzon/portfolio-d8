<?php
// $Id$

/**
 * Field handler for boolean properties.
 */
class efq_views_handler_field_property_boolean extends entity_views_handler_field_boolean {
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
