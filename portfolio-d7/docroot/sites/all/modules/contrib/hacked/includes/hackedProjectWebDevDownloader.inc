<?php

/**
 * Downloads the dev version of a project.
 */
class hackedProjectWebDevDownloader extends hackedProjectWebDownloader {

  /**
   * Get download information for the dev release.
   *
   * @return array
   *   'module'    => The name of the project.
   *   'giturl'    => The upstream URL of the project.
   *   'branch'    => The development branch the snapshot was taken from.
   *   'timestamp' => When the project was originally downloaded.
   */
  function download_link() {
    $info = array();
    $project = &$this->project->project_info;

    // Get the branch we're on.
    $release = strtok($project['existing_version'],'-') . '-' . $project['existing_major'] . '.x-dev';
    $branch = $project['releases'][$release]['tag'];

    // Assign some information depending on the project type.
    if ($project['project_type'] == 'core') {
      // Special handling for core.
      $info['module'] = 'drupal';
    }
    else {
      // Contrib.
      $info['module'] = $project['name'];
    }

    // Assign all of the other information.
    $info['giturl'] = 'https://git.drupal.org/project/' . $project['name'] . '.git';
    $info['branch'] = $branch;
    $info['timestamp'] = $project['datestamp'];

    // Return it.
    return $info;
  }

  /**
   * Download the version of the project to compare.
   *
   * @return boolean
   *   TRUE if the download was successful; FALSE otherwise.
   */
  function download() {
    // Get detailed information about the download.
    $destination = $this->get_destination();
    $info = $this->download_link();
    $giturl = $info['giturl'];
    $branch = $info['branch'];
    $module = $info['module'];
    $timestamp = $info['timestamp'];

    // Return the path if the project version has already been downloaded to
    // the cache.
    if (file_exists($destination)) {
      return $destination;
    }

    // Clone the project repository and check out the appropriate branch.
    if (!$this->git_clone($giturl, $destination, $branch)) {
      watchdog('hacked', 'Could not clone project %project into path %path at branch %branch.', array(
        '%project' => $module,
        '%path' => $destination,
        '%branch' => $branch,
      ), WATCHDOG_ERROR);
      return FALSE;
    }

    // Attempt to checkout the last commit before timestamp on the local version.
    if (!$this->git_checkout($destination, $branch, $timestamp)) {
      watchdog('hacked', 'Could not check out latest commit before %timestamp in Git repository %repository.', array(
        '%timestamp' => $timestamp,
        '%repository' => $destination,
      ), WATCHDOG_ERROR);
      return FALSE;
    }

    // Add package information.
    $lines = array();
    $fp = fopen(drupal_get_path('module', $module) . "/$module.info", 'r');
    $found_generated = FALSE;
    while(!feof($fp))
    {
      $line = fgets($fp, 4096);
      if (strrpos($line, '; Information added by', -strlen($line)) !== false) {
        array_push($lines, PHP_EOL);
        $found_generated = TRUE;
      }

      if ($found_generated) {
        array_push($lines, $line);
      }
    }
    fclose($fp);

    if (count($lines)) {
      file_put_contents("$destination/$module.info", $lines, FILE_APPEND);
    }

    return TRUE;
  }

  /**
   * Clones a git repository in a temporary directory.
   *
   * @param $giturl
   *   URL of the project.
   * @param $path
   *   The location in which to place the checkout.
   * @param $branch
   *   The branch to check out.
   * @return
   *   Either FALSE on failure or the location of the checked out files.
   */
  function git_clone($giturl, $path, $branch) {

    // Get the Git command and location information.
    $git_cmd = $this->git_get_command();
    $dirname = dirname($path);
    $basename = basename($path);

    // Prepare the download location.
    file_prepare_directory($dirname, FILE_CREATE_DIRECTORY);
    file_prepare_directory($path, FILE_CREATE_DIRECTORY);

    // Perform the clone operation, and leave us in the appropriate branch.
    exec("cd $dirname; $git_cmd clone --branch $branch $giturl $basename", $output_lines, $return_value);

    // On success, return the path to the download, or FALSE otherwise.
    if ($return_value == 0) {
      return $path;
    }
    return FALSE;
  }

  /**
   * Checks out the last Git commit prior to the project's timestamp.
   *
   * @param $path
   *   The path of the Git repository where the checkout should happen.
   * @param $branch
   *   The desired branch within the repository containing the commit.
   * @param $timestamp
   *   The UNIX timestamp for the downloaded project.
   * @return
   *   Either FALSE on failure or the location of the checked out files.
   */
  function git_checkout($path, $branch, $timestamp) {

    // Fetch the Git command.
    $git_cmd = $this->git_get_command();

    // Point the working tree at the commit just before the timestamp.
    // This takes us to the point in time when the local project was downloaded.
    exec("cd $path; $git_cmd checkout $($git_cmd rev-list -n 1 --before='$timestamp' $branch)", $output_lines, $return_value);

    // On success, return the path to the download, or FALSE otherwise.
    if ($return_value == 0) {
      return $path;
    }
    return FALSE;
  }

  /**
   * Helper function to return the command to run git on the command line.
   *
   * @return string
   *   The command with which to run Git.
   */
  function git_get_command() {
    return variable_get('hacked_git_cmd', 'git');
  }

  /**
   * Returns the final destination of the unpacked project.
   *
   * @return string
   *   The unpacked project's path.
   */
  function get_final_destination() {
    // Simply return the original destination as Git's cloning already provides
    // an unpacked project.
    return $this->get_destination();
  }
}
