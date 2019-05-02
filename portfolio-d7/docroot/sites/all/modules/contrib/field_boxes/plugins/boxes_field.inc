<?php

    drupal_add_js(drupal_get_path('module',"field_boxes"). "/jquery.tablednd_0_5.js");
    drupal_add_js(drupal_get_path('module',"field_boxes"). "/field_boxes.js");
class boxes_field extends boxes_box {
  /*
   * we use fancy ajax on our options form and this brakes with the context ui
   */
  function use_multistep_create() {
    return TRUE;
  }

  public function options_defaults() {
    return array(
      'entity_type' => 'node',
      'entity_id' => '',
      'bundle' => 0,
      'field' => array(),
      'use_page' => 0,
      'exclude_from_content' => 0,
      'view_mode' => 'default',
    );

  }

  public function options_submit($form, &$form_state) {
    // if we an edit field then lets save the data to the entity
    /*
    if (isset($form['options']['entity']['field_settings']['edit'])) {
      $entity = $this->get_entity(TRUE);
      $entity_type = $this->options['entity_type'];
      list($id, $vid, $bundle) = entity_extract_ids($entity_type, $entity);
      entity_form_submit_build_entity($entity_type, $entity, $form, $form_state);
      field_attach_presave($entity_type, $entity);
      module_invoke_all('entity_presave', $entity, $entity_type);
      field_attach_update($entity_type, $entity, $form, $form_state);
      //clear the cache or we will not see anything on save
      entity_get_controller($entity_type)->resetCache(array($id));
    }
    */
  }

  public function options_form(&$form_state) {
    //update options from form state
    foreach($this->options as $key => $value) {
      if (isset($form_state['values'][$key])) {
        $this->options[$key] = $form_state['values'][$key];
      }
    }

    $form = array();
    
    //Select entity type
    $entities_info = entity_get_info();
    foreach($entities_info as $key=>$info) {
      if($info['fieldable']) {
        $entity_type_options[$key] = $info['label'];
      }
    }
    $form['entity_type'] = array(
      '#type' => 'select',
      '#title' => 'Entity Type',
      '#options' => $entity_type_options,
      '#default_value' => $this->options['entity_type'],
      '#description' => "Select they type of item from which the field will be pulled.",
      '#ajax' => array(
        'callback' => 'field_boxes_bundle',
        'wrapper' => 'entity-wrapper',
        'method' => 'replace',
        'effect' => 'fade',
      ),
    );  

    //wrapper element for ajax callbacks
    $form['entity'] = array(
      '#type' => 'item',
      '#prefix' => '<div id="entity-wrapper">',
      '#suffix' => '</div>',
    );

    //the rest of the form is based on having an entity so if we do not have
    //one skip out
    if ($this->options['entity_type']) {
      $entity_type = $this->options['entity_type'];
      $entity_info = $entities_info[$entity_type];


      //if the bundle is not valid clear it out
      if(!in_array($this->options['bundle'], array_keys($entity_info['bundles']))) {
        $this->options['bundle'] = 0;
      }

      //We are going to go though all fields and see if they 
      //apply to the current entity type to build the field options
      //also we will grab all bundles for the bundle options
      $fields = field_info_fields();
      $instances= FALSE;
      if($this->options['bundle']) {
        $instances = _field_invoke_get_instances($this->options['entity_type'], $this->options['bundle']);
      }
      foreach($fields as $key =>$field) {
        if (isset($field['bundles'][$entity_type])) {
          $bundles = $field['bundles'][$entity_type];
          //build field options
          if (in_array($this->options['bundle'], $bundles) || !($this->options['bundle'])) { 
            $label = $instances  ? $instances[$key]['label'] : $key;
            $field_options[$key] = array($label, implode(', ', $bundles));
          }
          //allow for a bundle of any
          $bundle_options[0] = 'Any';
          foreach($field['bundles'][$entity_type] as $bundle) {
            $bundle_options[$bundle] = $bundle;
            
          }
        }
      }

      // Let the user pick the entity from which to pull
      // note if the entity type has a label we can use auto compleate
      // if it does not then the user has to enter the ids
      $form['entity']['id'] = array(
        '#type' => 'fieldset',
        '#title' => 'Select ' . $entity_info['label'],
        '#prefix' => '<div id = "entity-id-wrapper">',
        '#suffix' => '</div>',

      );
      // if we know how to extract the entity from the page 
      // offer an option to do just that
      // if we dont make sure that use_page is false
      if ($this->can_use_page()) {
        $form['entity']['id']['use_page'] = array(
          '#type' => 'checkbox',
          '#title' => 'Use '. $entity_info['label'] . " on current page",
          '#options' => array('1' => 'Use '. $entity_info['label'] . " from url", '0' => 'No'),
          '#default_value' => $this->options['use_page'],
          '#ajax' => array(
            'callback' => 'field_boxes_bundle',
            'replace with' => array(),
            'wrapper' => 'entity-wrapper',
            'method' => 'replace',
            'effect' => 'fade',
          ),
        );
      }
      else {
        $this->options['use_page'] = FALSE;
      }
      if(!$this->options['use_page']) {
        $form['entity']['id']['entity_id'] = array(
          '#type' => 'textfield',
          '#title' => $entity_info['label'] . " id",
          '#default_value' => $this->options['entity_id'],
          '#ajax' => array(
            'callback' => 'field_boxes_bundle',
            'replace with' => array('field_settings'),
            'wrapper' => 'field-settings-wrapper',
            'method' => 'replace',
            'effect' => 'fade',
          ),
        );
        // If we have a label in entity info then we can use our auto compleate
        if(isset($entity_info['entity keys']['label'])) {
          $form['entity']['id']['entity_id']['#title'] = $entity_info['label'];
          $form['entity']['id']['entity_id']['#autocomplete_path'] = 'entity-autocomplete/'.$entity_type;
        }
      }
      else {
        /*
         * not sure how to get these exclude from the core item
        $form['entity']['id']['exclude_from_content'] = array(
          '#type' => 'checkbox',
          '#title' => 'Exclude '. $entity_info['label'] . " from also apearing in content",
          '#options' => array('1' => 'Exclude '. $entity_info['label'] . " from also apearing in content", '0' => 'No'),
        );
        */
      }
      


      //if if our field is an option
      if(!in_array($this->options['field'], array_keys($field_options))) {
        //$this->options['field'] = '';
      }
      //Field set to hold all elements need for selecting the 
      //drupal field
      $form['entity']['field_set'] = array(
        '#type' => 'fieldset',
        '#title' => 'Select ' . $entity_info['label'] . ' Field:' . implode(", ",array_diff($this->options['field'], array(0))),
        '#collapsible' => TRUE,
        '#collapsed' => (bool)$this->options['field'],
      );

      // let the fields be filtered by bundle
      $form['entity']['field_set']['bundle'] = array(
        '#type' => 'select',
        '#title' => 'Filter on Bundle:',
        '#options' => $bundle_options,
        '#default_value' => $this->options['bundle'],
        '#ajax' => array(
          'callback' => 'field_boxes_bundle',
          'wrapper' => 'field-wrapper',
          'replace with' => array('field_set', 'field'),
          'method' => 'replace',
          'effect' => 'fade',
        ),
      );

      //insure that we have the correct order of currently selected
      //items
      $default_options = array();
      foreach($this->options['field'] as $field) {
        if (isset($field_options[$field])) {
          $default_options[$field] = $field_options[$field];
        }
      }
      $field_options = $default_options+$field_options;
      //Here is where the field is selected
      $form['entity']['field_set']['field'] = array(
        '#type' => 'tableselect',
        '#title' => 'Field',
        '#multiple' => TRUE,
        '#prefix' => '<div id="field-wrapper">',
        '#suffix' => '</div>',
        '#header' => array("Name", 'Used by'),
        '#options' => $field_options,
        '#default_value' => $this->options['field'],
        // we want to refresh field settings as they are based
        // on our value
        '#attributes' => array('class'=>array('field-boxes-table-order')),

        '#ajax' => array(
          'callback' => 'field_boxes_bundle',
          'replace with' => array('field_settings'),
          'wrapper' => 'field-settings-wrapper',
          'method' => 'replace',
          'effect' => 'fade',
        ),
      );
      // The view section lets us pick which mode to use
      // TODO: offer an option for a custom mode, and let the
      // user configure the display settings
      $mode_options = array("default"=> "Default");
      foreach($entity_info['view modes'] as $key => $mode) {
        $mode_options[$key] = $mode['label'];
      }
      $form['entity']['field_settings'] = array(
        '#type' => 'item',
        '#prefix' => '<div id="field-settings-wrapper">',
        '#suffix' => '</div>',
      );
      $form['entity']['field_settings']['view'] = array(
        '#type' => 'fieldset',
        '#title' => 'Display Settings:' .$mode_options[$this->options['view_mode']],
        '#collapsed' => TRUE,
        '#collapsible' => TRUE,
      );
      $form['entity']['field_settings']['view']['view_mode'] = array(
        '#type' => 'select',
        '#title' => 'View Mode',
        '#options' => $mode_options,
        '#default_value' => $this->options['view_mode'],
      );
      
     if((isset($form_state['triggering_element']['#ajax']['enable edit']) ||     
        !isset($form_state['triggering_element'])) && 
        ($entity =     $this->get_entity()) ) {    
          $form['entity']['field_settings']['edit'] = array( 
            '#type' => 'fieldset', 
            '#title' => "Edit {$this->options['field']} Field", 
            '#collapsed' => FALSE, 
            '#collapsible' => TRUE, 
            '#prefix' => '<div id="edit-wrapper">', 
            '#suffix' => '</div>',
          ); 
        $edit_form = array();     
        $edit_form_state = array(); 
        list($id, $vid, $bundle) = entity_extract_ids($entity_type, $entity);     
        entity_get_controller($entity_type)->resetCache(array($id));    
        //turn this off as it brakes on the media image
          //field_attach_load($entity_type, array($entity));  
          //field_attach_form($entity_type, $entity, $edit_form, $form_state, $langcode = NULL);  
        $form['entity']['field_settings']['edit'][$this->options['field']] = $edit_form[$this->options['field']]; 
      }      
      // add button to start editing      
      elseif ($this->get_entity()) {    
        $form['entity']['field_settings']['edit_start'] = array( 
          '#type' =>  'button', 
          '#value' => "Edit {$this->options['field']} Field", 
          '#prefix' => '<div id="edit-wrapper">', 
          '#suffix' => '</div>', 
          '#ajax' => array( 
            'callback' => 'field_boxes_bundle', 
            'replace with' => array('field_settings','edit'), 
            'enable edit' =>TRUE, 
            'wrapper' => 'edit-wrapper', 
            'method' => 'replace', 
            'effect' => 'fade',
          ),
        ); 
      } 
      else {    
        $form['entity']['field_settings']['edit_empty'] = array( 
          '#type' => 'item', 
          '#prefix' => '<div id="edit-wrapper">', 
          '#suffix' => '</div>',
        );   
      } 
    } 
    return $form; 
  }  

  public function can_use_page() {
    return in_array($this->options['entity_type'], array('node', 'taxonomy_term'));
  }
  public function get_entity() {
    if ($this->options['use_page']) {
      // we should only have to run this once per page
      $entity = &drupal_static(__FUNCTION__);
      if(isset($entity)) {
        return $entity;
      }
      else {
        switch($this->options['entity_type']) {
          case 'node' :
            return menu_get_object($this->options['entity_type'], 1);
          break;
          case 'taxonomy_term' :
            return menu_get_object($this->options['entity_type'], 2);
          break;
        }
      }
    }
    else {
      $id = $this->options['entity_id'];
      if(!is_numeric($id)) {
        if (preg_match('/\[id:(?P<digit>\d+)\]/', $id, $matches)) {
          $id =  $matches['digit'];
        }
      }
      if(is_numeric($id)) {
        $entities = entity_load($this->options['entity_type'], array($id));
        return $entities[$id];
      }
    }
  }
  public function render() {
    context_set("boxes_fields", "exclude", array($this->options['field']));
    $entities_info = entity_get_info();
    $entity_info = $entities_info[$this->options['entity_type']];
    $entity = $this->get_entity();
    $render_array = array();
    if($entity) {
       foreach($this->options['field'] as $field) {
         if (isset($entity->{$field})) {
           // remove the weight so we can order them our selves
           $render_element = field_view_field($this->options['entity_type'], $entity, $field, $this->options['view_mode'] ? $this->options['view_mode'] : 'box');
           unset($render_element['#weight']);
           $render_array[] = $render_element;
         }
       }
    }
    return array(
      'delta' => $this->delta, // Crucial.
      'title' => $this->title,
      'subject' => $this->title,
      'content' => render($render_array),
      // I do not know why i have to render this
      //'content' => $render_array,
    );

  }
}
