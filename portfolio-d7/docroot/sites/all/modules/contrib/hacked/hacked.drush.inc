<?php
/**
 * @file
 * Hacked! drush support.
 */

if (function_exists('hacked_load_drush_dependencies')) {
  return;
}

/**
 * Allow Hacked! to be used without enabling the module.
 */
function hacked_load_drush_dependencies() {
  if (!module_exists('hacked')) {
    drush_log(dt('Loading Hacked! dependencies.'));
    require_once __DIR__ . '/hacked.module';
    require_once __DIR__ . '/includes/hackedFileGroup.inc';
    require_once __DIR__ . '/includes/hackedFileHasher.inc';
    require_once __DIR__ . '/includes/hackedFileIgnoreEndingsHasher.inc';
    require_once __DIR__ . '/includes/hackedFileIncludeEndingsHasher.inc';
    require_once __DIR__ . '/includes/hackedProject.inc';
    require_once __DIR__ . '/includes/hackedProjectWebDownloader.inc';
    require_once __DIR__ . '/includes/hackedProjectWebDevDownloader.inc';
    require_once __DIR__ . '/includes/hackedProjectWebCVSDownloader.inc';
    require_once __DIR__ . '/includes/hackedProjectWebFilesDownloader.inc';
    require_once __DIR__ . '/hacked.report.inc';
  }
}

/**
 * Implements hook_drush_help().
 */
function hacked_drush_help($section) {
  switch ($section) {
    case 'drush:hacked-list-projects':
      return dt('List projects and their hacked/unhacked status.');

    case 'drush:hacked-details':
      return dt('Show details of the files in one project, and the hacked/unhacked status of those files.');

    case 'drush:hacked-diff':
      return dt('Output a unified diff of the specified project.');

  }
}

/**
 * Implements hook_drush_command().
 */
function hacked_drush_command() {
  $items = array();

  $items['hacked-list-projects'] = array(
    'description' => dt('List all projects that can be analysed by Hacked!'),
    'options' => array(
      'force-rebuild' => dt('Rebuild the Hacked! report instead of getting a cached version.'),
    ),
    'aliases' => array('hlp'),
  );

  $items['hacked-lock-modified'] = array(
    'description' => dt('Lock all projects that Hacked! detects are modified so pm-updatecode will not change them.'),
  );

  $items['hacked-details'] = array(
    'description' => dt('Show the Hacked! report about a specific project.'),
    'arguments' => array(
      'project' => dt('The machine name of the project to report on.'),
    ),
    'options' => array(
      'include-unchanged' => dt('List unchanged files.'),
    ),
    'aliases' => array('hd'),
  );

  $items['hacked-diff'] = array(
    'description' => dt('Output a unified diff of the project specified.'),
    'arguments' => array(
      'project' => dt('The machine name of the project to report on.'),
    ),
    'options' => array(
      'diff-options' => dt('Command line options to pass through to the diff command.'),
    ),
  );

  return $items;
}

/**
 * Compute the report data for hacked.
 *
 * WARNING: This function can invoke a batch process and end your current page.
 * So you'll want to be very careful if you call this!
 */
function hacked_calculate_project_data_drush($projects, $force = FALSE, $redirect = NULL) {
  hacked_load_drush_dependencies();

  // Try to get the report form cache if we can.
  $cache = drush_cache_get('hacked:drush:full-report', HACKED_CACHE_TABLE);
  if (!empty($cache->data) && !$force) {
    return $cache->data;
  }

  // Enter a batch to build the report.
  $operations = array();
  foreach ($projects as $project) {
    $operations[] = array(
      'hacked_build_report_batch_drush',
      array($project['name']),
    );
  }

  $batch = array(
    'operations' => $operations,
    'finished' => 'hacked_build_report_batch_finished_drush',
    'file' => drupal_get_path('module', 'hacked') . '/hacked.report.inc',
    'title' => dt('Building report'),
  );

  drush_print('Rebuilding Hacked! report');
  batch_set($batch);
  $batch =& batch_get();
  $batch['progressive'] = FALSE;
  drush_backend_batch_process();
  drush_print('Done.');

  // Now we can get the data from the cache.
  $cache = drush_cache_get('hacked:drush:full-report', HACKED_CACHE_TABLE);
  if (!empty($cache->data)) {
    return $cache->data;
  }
}

/**
 * Batch callback to build the hacked report.
 */
function hacked_build_report_batch_drush($project_name, &$context) {
  hacked_load_drush_dependencies();
  if (!isset($context['results']['report'])) {
    $context['results']['report'] = array();
  }

  $project = new hackedProject($project_name);
  $context['results']['report'][$project_name] = $project->compute_report();
  $context['message'] = dt('Finished processing: @name', array(
    '@name' => $project->title(),
  ));
}

/**
 * Completion callback for the report batch.
 */
function hacked_build_report_batch_finished_drush($success, $results, $operations) {
  if ($success) {
    // Sort the results.
    usort($results['report'], '_hacked_project_report_sort_by_status');
    // Store them.
    drush_cache_set('hacked:drush:full-report', $results['report'], HACKED_CACHE_TABLE, strtotime('+1 day'));
  }
}

/**
 * Validation callback for drush hacked_list_projects.
 */
function drush_hacked_list_projects_validate() {
  if (!function_exists('update_get_available')) {
    return drush_set_error('HACKED_UPDATE_DISABLED', 'Cannot list projects without the Drupal core update module enabled.');
  }
}

/**
 * Command callback for drush hacked_list_projects.
 */
function drush_hacked_list_projects() {
  hacked_load_drush_dependencies();
  // Get the data.
  module_load_include('inc', 'update', 'update.report');
  if ($available = update_get_available(TRUE)) {
    module_load_include('inc', 'update', 'update.compare');
    $data = update_calculate_project_data($available);
    $force_rebuild = drush_get_option('force-rebuild', FALSE);
    $projects = hacked_calculate_project_data_drush($data, $force_rebuild);

    // Now print the data using drush:
    $rows[] = array(
      dt('Title'),
      dt('Name'),
      dt('Version'),
      dt('Status'),
      dt('Changed'),
      dt('Deleted'),
    );

    foreach ($projects as $project) {
      $row = array(
        $project['title'],
        $project['name'],
        $project['existing_version'],
      );

      // Now add the status:
      switch ($project['status']) {
        case HACKED_STATUS_UNHACKED:
          $row[] = dt('Unchanged');
          break;

        case HACKED_STATUS_HACKED:
          $row[] = dt('Changed');
          break;

        case HACKED_STATUS_UNCHECKED:
        default:
          $row[] = dt('Unchecked');
          break;

      }

      $row[] = $project['counts']['different'];
      $row[] = $project['counts']['missing'];

      $rows[] = $row;
    }
    drush_print_table($rows, TRUE);

    return $projects;
  }
}

/**
 * Lock modified files so that pm-updatecode won't touch them.
 */
function drush_hacked_lock_modified() {
  $drupal_root = drush_get_context('DRUSH_DRUPAL_ROOT');
  module_load_include('inc', 'update', 'update.report');
  if (isset($drupal_root) && ($available = update_get_available(TRUE))) {
    module_load_include('inc', 'update', 'update.compare');
    $data = update_calculate_project_data($available);
    $projects = hacked_calculate_project_data_drush($data, TRUE);

    foreach ($projects as $project) {
      $row = array(
        empty($project['title']) ? $project['name'] : $project['title'],
        $project['name'],
        $project['existing_version'],
      );

      // Lock the file if it is not already locked.
      switch ($project['status']) {
        case HACKED_STATUS_HACKED:
          $project_ob = NULL;
          $project_ob = hacked_project_load($project['project_name']);
          $lockfile = $project_ob->file_get_location('local', '.drush-lock-update');
          if (!file_exists($lockfile)) {
            drush_op('file_put_contents', $lockfile, dt('Locked: modified.'));
            drush_print(dt('Locked: @project', array(
              '@project' => $project['name'],
            )));
          }
          else {
            drush_print(dt('@project is modified and already locked', array(
              '@project' => $project['name'],
            )));
          }
          break;

        case HACKED_STATUS_UNHACKED:
        case HACKED_STATUS_UNCHECKED:
        default:
          break;

      }
    }
  }
}

/**
 * Add a --lock-modified flag to pm-updatecode.
 */
function drush_hacked_pre_pm_updatecode() {
  if (drush_get_option('lock-modified')) {
    drush_print(dt('Hacked! is checking for modified projects...'));
    drush_hacked_lock_modified();
    drush_print(dt('Hacked! modification check complete.'));
  }
}

/**
 * Add --lock-modified to the pm-updatecode and pm-update help.
 */
function hacked_drush_help_alter(&$command) {
  if (($command['command'] == 'pm-updatecode') || ($command['command'] == 'pm-update')) {
    $command['sub-options']['--lock']['--lock-modified'] = dt('Lock any project that Hacked! determines is modified.');
  }
}

/**
 * Validate hook for the hacked_details drush command.
 */
function drush_hacked_details_validate($short_name = '') {
  return drush_hacked_drush_command_validate($short_name);
}

/**
 * Generic validator for hacked drush commands.
 *
 * @param string $short_name
 *   The name of the project in question.
 *
 * @return null|boolean
 *   Return FALSE if there's an error.
 */
function drush_hacked_drush_command_validate($short_name = '') {
  hacked_load_drush_dependencies();
  if (empty($short_name)) {
    return drush_set_error('HACKED_PROJECT_NOT_FOUND', dt('You must specify the name of a project.'));
  }

  $project = hacked_project_load($short_name);
  $project->identify_project();
  if (!$project->project_identified) {
    return drush_set_error('HACKED_PROJECT_NOT_FOUND', dt('Unable to find "@project".', array(
      '@project' => $short_name,
    )));
  }
}

/**
 * Command callback for drush hacked_details.
 */
function drush_hacked_details($short_name) {
  $project = hacked_project_load($short_name);
  $report = $project->compute_details();

  drush_print(dt('Details for project: @name', array('@name' => $project->title())));
  drush_print(dt('Total files: @total_files, files changed: @changed_files, deleted files: @deleted_files', array(
    '@total_files' => count($report['files']),
    '@changed_files' => $report['counts']['different'],
    '@deleted_files' => $report['counts']['missing'],
  )));
  drush_print('');

  drush_print(dt('Detailed results:'));
  // Sort the results:
  arsort($report['files']);

  $rows[] = array(
    dt('Status'),
    dt('File'),
  );
  $show_unchanged = drush_get_option('include-unchanged', FALSE);
  foreach ($report['files'] as $file => $status) {
    if (!$show_unchanged && $status == HACKED_STATUS_UNHACKED) {
      continue;
    }
    $row = array();

    // Now add the status:
    switch ($status) {
      case HACKED_STATUS_UNHACKED:
        $row[] = dt('Unchanged');
        break;

      case HACKED_STATUS_HACKED:
        $row[] = dt('Changed');
        break;

      case HACKED_STATUS_DELETED:
        $row[] = dt('Deleted');
        break;

      case HACKED_STATUS_UNCHECKED:
      default:
        $row[] = dt('Unchecked');
        break;

    }

    $row[] = $file;
    $rows[] = $row;
  }
  drush_print_table($rows, TRUE);
}

/**
 * Validation callback for drush hacked_diff.
 */
function drush_hacked_diff_validate($short_name = '') {
  return drush_hacked_drush_command_validate($short_name);
}

/**
 * Command callback for drush hacked_diff.
 */
function drush_hacked_diff($short_name) {
  $project = hacked_project_load($short_name);

  $local_location = $project->file_get_location('local', '');
  $clean_location = $project->file_get_location('remote', '');

  // If the hasher is our ignore line endings one, then ignore line endings.
  if (variable_get('hacked_selected_file_hasher', HACKED_DEFAULT_FILE_HASHER) == 'hacked_ignore_line_endings') {
    $default_options = '-uprb';
  }
  else {
    $default_options = '-upr';
  }

  $diff_options = drush_get_option('diff-options', $default_options);
  drush_shell_exec("diff $diff_options $clean_location $local_location");

  $lines = drush_shell_exec_output();
  $local_location_trim = dirname($local_location . '/dummy.file') . '/';
  $clean_location_trim = dirname($clean_location . '/dummy.file') . '/';
  foreach ($lines as $line) {
    if (strpos($line, '+++') === 0) {
      $line = str_replace($local_location_trim, '', $line);
    }
    if (strpos($line, '---') === 0) {
      $line = str_replace($clean_location_trim, '', $line);
    }
    if (strpos($line, 'diff -upr') === 0) {
      $line = str_replace($clean_location_trim, 'a/', $line);
      $line = str_replace($local_location_trim, 'b/', $line);
    }
    drush_print($line);
  }
}

/**
 * Implements hook_drush_command_alter().
 */
function hacked_drush_command_alter(&$command) {
  if ($command['command'] == 'audit_security') {
    $command['checks'][] = array(
      'name' => 'Hacked',
      'location' => __DIR__ . '/hacked.site_audit.inc',
    );
  }
}
