<?php
// $Id: tvi.query.inc,v 1.1 2009/12/04 01:40:56 awebb Exp $

//******************************************************************************
// TVI Database Interface
//******************************************************************************

/**
 * Return a default setting object.
 */
function tvi_default_settings($type = TVI_TYPE_TERM, $xid = 0) {
	$settings = new stdClass();
	
  $settings->type = $type;
  $settings->xid  = $xid;
  
  $settings->view_name = 0;
  $settings->display = TVI_DEFAULT_DISPLAY;
  $settings->status  = 0;
  
  $settings->is_default = TRUE;
		
	return $settings;	
}

//------------------------------------------------------------------------------

/**
 * Load a setting from the database or return a default, if the $return_default
 * flag is set.
 */
function tvi_load_settings($xid, $type = TVI_TYPE_TERM, $return_default = TRUE) {
	
  $settings = db_select('tvi_settings', 'tvi')
          ->fields('tvi')
          ->condition('tvi.type', $type)
          ->condition('tvi.xid', $xid)
          ->execute()
          ->fetchObject();
  
  if (!is_object($settings)) {
  	return ($return_default ? tvi_default_settings($type, $xid) : NULL);
  }
  
  $settings->is_default = FALSE;
  return $settings;
}

//------------------------------------------------------------------------------

/**
 * Save settings information for a taxonomy vocabulary or term to the database.
 */
function tvi_update_settings($settings) {
	
  if (is_array($settings)) {
   $settings = (object)$settings;
  }

  $count = db_select('tvi_settings', 'tvi')
        ->fields('tvi')
        ->condition('tvi.type', $settings->type)
        ->condition('tvi.xid', $settings->xid)
        ->countQuery()
        ->execute()
        ->fetchCol();

  if($count[0] != 0) {
    $result = db_update('tvi_settings')
        ->condition('type', $settings->type)
        ->condition('xid', $settings->xid)
        ->fields(array(
            'view_name' => $settings->view_name,
            'display' => $settings->display,
            'status' => $settings->status,
          ))
        ->execute();
  }
  else {
    $result = db_insert('tvi_settings')
        ->fields(array(
            'type' => $settings->type,
            'xid' => $settings->xid,
            'view_name' => $settings->view_name,
            'display' => $settings->display,
            'status' => $settings->status,
          ))
        ->execute();
  }
  return $result;
}

//------------------------------------------------------------------------------

function tvi_remove_settings($xid, $type = TVI_TYPE_TERM) {
  $return = db_delete('tvi_settings')
      ->condition('type', $settings->type)
      ->condition('xid', $settings->xid)
      ->execute();
}

//------------------------------------------------------------------------------