<?php

/**
 * @file
 * GraphAPI examples file.
 */

/**
 * Example function: creates a graph of user logins by day.
 */
function user_last_login_by_day($n=40) {
  $query = db_select('users');
  $query->addField('users', 'name');
  $query->addField('users', 'uid');
  $query->addField('users', 'created');
  $query->condition('uid', 0, '>');
  $query->orderBy('created', 'DESC');
  $query->range(0, $n);
  $result = $query->execute();
  $g = graphapi_new_graph();
  $now = time();
  $days = array();
  foreach ($result as $user) {
    $uid = $user->uid;
    $user_id = 'user_' . $uid;

    $day = intval(($now - $user->created) / (24 * 60 * 60));
    $day_id = 'data_' . $day;
    graphapi_set_node_title($g, $user_id, l($user->name, "user/" . $uid));
    graphapi_set_node_title($g, $day_id, "Day " . $day);
    graphapi_set_link_data($g, $user_id, $day_id, array('color' => '#F0F'));
  }
  $options = array(
    'width' => 400,
    'height' => 400,
    'item-width' => 50,
    'engine' => 'graph_phyz'
  );
  return theme('graphapi_dispatch', array('graph' => $g, 'config' => $options));
}
