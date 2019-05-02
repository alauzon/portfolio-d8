<?php
/**
 * @file
 * Contains \SiteAudit\Check\Codebase\Hacked.
 */

class SiteAuditCheckSecurityHacked extends SiteAuditCheckAbstract {
  /**
   * Implements \SiteAudit\Check\Abstract\getLabel().
   */
  public function getLabel() {
    return dt('Hacked');
  }

  /**
   * Implements \SiteAudit\Check\Abstract\getDescription().
   */
  public function getDescription() {
    return dt('Determine whether core or contrib modules have been modified.');
  }

  /**
   * Implements \SiteAudit\Check\Abstract\getResultFail().
   */
  public function getResultFail() {
    return dt('Unable to determine whether core or contrib modules have been modified!');
  }

  /**
   * Implements \SiteAudit\Check\Abstract\getResultInfo().
   */
  public function getResultInfo() {
    return dt('Hacked! module not available to drush, either as part of site to be audited or in drush commands.');
  }

  /**
   * Implements \SiteAudit\Check\Abstract\getResultPass().
   */
  public function getResultPass() {
    return dt('No modifications were found.');
  }

  /**
   * Implements \SiteAudit\Check\Abstract\getResultWarn().
   */
  public function getResultWarn() {
    $ret_val = dt('The following extension(s) have been modified:');

    if (drush_get_option('html')) {
      $ret_val = '<p>' . $ret_val . '</p>';
      $ret_val .= '<table class="table table-condensed">';
      $ret_val .= '<thead><tr><th>Name</th><th>Title</th><th>Version</th><th>Changed</th></thead>';
      $ret_val .= '<tbody>';
      foreach ($this->registry['hacked'] as $row) {
        $ret_val .= '<tr><td>' . implode('</td><td>', $row) . '</td></tr>';
      }
      $ret_val .= '</tbody>';
      $ret_val .= '</table>';
    }
    else {
      foreach ($this->registry['hacked'] as $row) {
        $ret_val .= PHP_EOL . str_repeat(' ', 6);
        $ret_val .= $row['project_name'] . ' v' . $row['project_version'];
        $ret_val .= ': ' . $row['lines_different'] . ' line(s)';
      }
    }
    return $ret_val;
  }

  /**
   * Implements \SiteAudit\Check\Abstract\getAction().
   */
  public function getAction() {}

  /**
   * Implements \SiteAudit\Check\Abstract\calculateScore().
   */
  public function calculateScore() {
    $result = drush_invoke_process('@self', 'hacked-list-projects', array(), array('--include-unchanged=0', '--strict=0'), FALSE);
    if ($result === FALSE) {
      return SiteAuditCheckAbstract::AUDIT_CHECK_SCORE_INFO;
    }

    $rows = array();
    foreach ($result['object'] as $info) {
      if ($info['counts']['different'] != 0) {
        $rows[] = array(
          'project_name' => $info['project_name'],
          'project_title' => $info['title'],
          'project_version' => $info['existing_version'],
          'lines_different' => $info['counts']['different'],
        );
      }
    }

    $this->registry['hacked'] = $result;
    if (empty($result['object'])) {
      return SiteAuditCheckAbstract::AUDIT_CHECK_SCORE_FAIL;
    }
    else {
      if (!empty($rows)) {
        $this->registry['hacked'] = $rows;
        return SiteAuditCheckAbstract::AUDIT_CHECK_SCORE_WARN;
      }
      else {
        return SiteAuditCheckAbstract::AUDIT_CHECK_SCORE_PASS;
      }
    }
  }
}
