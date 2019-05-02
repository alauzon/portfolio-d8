<?php

/**
 * @file
 * Content field formatter functions for Image Matrix module.
 */

/**
 * Implements hook_field_formatter_info().
 */
function imagematrix_field_formatter_info() {
  $formatters = array(
    'imagematrix' => array(
      'label' => t('Image Matrix'),
      'field types' => array('image'),
      'settings' => array(
        'block_width' => 600,
        'block_height' => NULL,
        'block_images_count' => 7,
        'image_padding' => 3,
        'image_link' => NULL,
        'randomize_layout' => FALSE,
        'cache_layout' => FALSE,
        'custom_class' => NULL,
        'custom_rel' => NULL,
      ),
    ),
  );
  return $formatters;
}

/**
 * Implements hook_field_formatter_settings_form().
 */
function imagematrix_field_formatter_settings_form($field, $instance, $view_mode, $form, &$form_state) {

  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];
  $element = array();

  // If formatter settings form was called from the view, let's return only
  // "Link image to" form element, as in this case all settings are configured
  // from Views' style plugin options form.
  if ($view_mode != '_dummy') {

    $element['block_width'] = array(
      '#type' => 'textfield',
      '#title' => t('Width of one block'),
      '#default_value' => $settings['block_width'],
      '#required' => TRUE,
      '#element_validate' => array('element_validate_integer_positive'),
      '#field_suffix' => 'px',
      '#size' => 5,
    );

    $element['block_height'] = array(
      '#type' => 'textfield',
      '#title' => t('Height of one block'),
      '#description' => t('This value is optional. If you leave it empty, you will most probably <br>get blocks with different heights, but images will not be cropped.<br />If you specify it on the other hand, all blocks will have the same height<br />(which would be desirable for example when displaying them next to<br />each other), but images could (and most probably will) be partially cropped.'),
      '#default_value' => $settings['block_height'],
      '#field_suffix' => 'px',
      '#size' => 5,
    );

    $element['block_images_count'] = array(
      '#type' => 'select',
      '#title' => t('Number of images in one block'),
      '#options' => drupal_map_assoc(range(2, 8)),
      '#default_value' => $settings['block_images_count'],
      '#multiple' => FALSE,
    );

    $element['randomize_layout'] = array(
      '#type' => 'checkbox',
      '#title' => t('Randomize generated layout'),
      '#description' => t('Enabling this option will try to randomize each block layout each time<br />it is generated. This option is exclusive with "Cache generated layout".'),
      '#default_value' => $settings['randomize_layout'],
    );

    $element['cache_layout'] = array(
      '#type' => 'checkbox',
      '#title' => t('Cache generated layout'),
      '#description' => t('Enabling this option will save once generated layout to the cache,<br />which could slightly improve page loading time. On the other hand<br />it will also cause images to appear in exactly the same places on each<br />page load (which could be slightly randomized otherwise). This option<br />is exclusive with "Randomize generated layout".'),
      '#default_value' => $settings['cache_layout'],
    );

    $element['image_padding'] = array(
      '#type' => 'textfield',
      '#title' => t('Image padding'),
      '#description' => t('Padding around each image.'),
      '#default_value' => $settings['image_padding'],
      '#element_validate' => array('element_validate_integer'),
      '#field_suffix' => 'px',
      '#size' => 5,
    );

  }

  $link_types = array(
    'content' => t('Content'),
    'file' => t('File'),
  );
  $element['image_link'] = array(
    '#title' => t('Link image to'),
    '#type' => 'select',
    '#default_value' => $settings['image_link'],
    '#empty_option' => t('Nothing'),
    '#options' => $link_types,
  );

  $element['custom_class'] = array(
    '#type' => 'textfield',
    '#title' => t('Custom link class'),
    '#description' => t('Custom class to add to all image links.'),
    '#default_value' => $settings['custom_class'],
    '#states' => array(
      'invisible' => array(
        ':input[name="options[settings][image_link]"]' => array('value' => ''),
        ':input[name="fields[' . $field['field_name'] . '][settings_edit_form][settings][image_link]"]' => array('value' => ''),
      ),
    ),
  );

  $element['custom_rel'] = array(
    '#type' => 'textfield',
    '#title' => t('Custom link <em>rel</em> attribute'),
    '#description' => t('Custom <em>rel</em> attribute to add to all image links.'),
    '#default_value' => $settings['custom_rel'],
    '#states' => array(
      'invisible' => array(
        ':input[name="options[settings][image_link]"]' => array('value' => ''),
        ':input[name="fields[' . $field['field_name'] . '][settings_edit_form][settings][image_link]"]' => array('value' => ''),
      ),
    ),
  );

  return $element;
}

/**
 * Implements hook_field_formatter_settings_summary().
 */
function imagematrix_field_formatter_settings_summary($field, $instance, $view_mode) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];

  $summary = array();

  $summary[] = t('Block width: !block_width px', array('!block_width' => $settings['block_width']));

  if (!empty($settings['block_heigth'])) {
    $summary[] = t('Block height: !block_height px', array('!block_height' => $settings['block_height']));
  }

  $summary[] = t('Images in one block: !block_images_count', array('!block_images_count' => $settings['block_images_count']));

  $image_padding = $settings['image_padding'] ? $settings['image_padding'] : 0;
  $summary[] = t('Image padding: !image_padding px', array('!image_padding' => $image_padding));

  $link_types = array(
    'content' => t('Images linked to content'),
    'file' => t('Images linked to file'),
  );
  // Display this setting only if image is linked.
  if (isset($link_types[$settings['image_link']])) {
    $summary[] = $link_types[$settings['image_link']];
  }
  else {
    $summary[] = t('Images without links');
  }

  $cache_layout = !empty($settings['cache_layout']) ? 'yes' : 'no';
  $summary[] =  t('Cache layout: !value', array('!value' => $cache_layout));

  if (!empty($settings['custom_class'])) {
    $summary[] =  t('Custom link class');
  }

  if (!empty($settings['custom_rel'])) {
    $summary[] =  t('Custom link <em>rel</em> attribute');
  }

  return implode('<br />', $summary);
}

/**
 * Implements hook_field_formatter_view().
 */
function imagematrix_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {

  // Check if the formatter involves a link.
  if ($display['settings']['image_link'] == 'content') {
    $uri = entity_uri($entity_type, $entity);
  }
  elseif ($display['settings']['image_link'] == 'file') {
    $link_file = TRUE;
  }

  // If formatter was called from the view, let's just return $items array,
  // as in such case we don't want to do any calculations here, as we don't
  // have all images available to arrange them properly. The only thing to
  // do here would be to add links to the content the image comes from
  // if we need them later.
  if (isset($display['views_view'])) {
    $element = array();

    foreach ($items as $delta => $item) {
      if (isset($link_file)) {
        $uri = array(
          'path' => file_create_url($item['uri']),
          'options' => array(),
        );
      }
      $content_uri = entity_uri($entity_type, $entity);
      $item['links'] = array(
        'content' => $content_uri['path'],
        'file' => file_create_url($item['uri']),
      );

      $item['custom_class'] = $display['settings']['custom_class'];
      $item['custom_rel'] = $display['settings']['custom_rel'];

      $element[$delta] = array(
        '#theme' => 'image_formatter',
        '#item' => $item,
        '#path' => isset($link_file) ? $uri : '',
      );

    }
    return $element;
  }

  $settings = $display['settings'];

  // Settings for MagazineLayout class.
  $mag_options = array(
    'block_width' => $settings['block_width'],
    'block_height' => $settings['block_height'],
    'block_images_count' => $settings['block_images_count'],
    'image_padding' => $settings['image_padding'],
    'randomize_layout' => $settings['randomize_layout'],
  );

  // Before initializing MagazineLayout class, let's check
  // if we don't already have block definitions stored in cache.
  if (!empty($settings['cache_layout'])) {
    $fids = array();
    foreach ($items as $item) {
      $fids[] = $item['fid'];
    }
    $cid_elements = $mag_options + array('image_link' => $settings['image_link']) + $fids;

    // Number of $cid_elements could get quite long (longer than 255 chars
    // allowed for cids), so we cannot use it directly, let's then sha1 it.
    $cid = 'imagematrix:' . sha1(implode(':', $cid_elements));

    $cached = cache_get($cid, 'cache_views');
    if (is_object($cached) && isset($cached->data)) {
      $blocks = $cached->data;
    }
  }
  // Block definitions were not found in cache,
  // so we need to regenerate them unfortunately.
  if (empty($settings['cache_layout']) || empty($cached->data)) {

    // Initialize MagazineLayout class.
    $mag = new MagazineLayout($mag_options);

    foreach ($items as $id => $item) {

      // Prepare array of image attributes to be passed
      // to MagazineLayout class, and, well, pass them.
      $image_attributes = array(
        'fid'     => $item['fid'],
        'path'    => drupal_realpath($item['uri']),
        'width'   => $item['width'],
        'height'  => $item['height'],
        'title'   => $item['title'],
        'alt'     => $item['alt'],
      );

      // Check if the formatter involves a link.
      if (isset($link_file)) {
        $uri = array(
          'path' => file_create_url($item['uri']),
          'options' => array(),
        );
      }
      if (!empty($uri)) {
        $image_attributes['uri'] = $uri['path'];
      }

      $mag->addImage($image_attributes);

    }

    // Get array of already processed blocks from MagazineLayout class.
    $blocks = $mag->getProcessedBlocks();

    // Save blocks to cache.
    if (!empty($settings['cache_layout'])) {
      cache_set($cid, $blocks, 'cache_views');
    }

  }

  $element = array(
    '#settings' => $settings,
  );
  if (!empty($blocks)) {
    foreach ($blocks as $block_id => $images) {
      $element[$block_id] = array(
        '#theme' => 'field_imagematrix_block',
        '#images' => $images,
        '#settings' => $settings,
      );
    }
  }

  return $element;
}
