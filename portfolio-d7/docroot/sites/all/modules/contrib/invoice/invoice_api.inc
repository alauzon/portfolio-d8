<?php
/**
 * Returns the response formats that the API supports
 *
 * @return array
 */
function _invoice_api_get_supported_response_formats() {
    return array('json', 'pdf', 'html');
}

/**
 * Checks if the specified response format is supported by the API
 *
 * @param string $format
 */
function _invoice_api_check_response_format_support($format) {
    if (!in_array(strtolower($format), _invoice_api_get_supported_response_formats())) {
        _invoice_api_http_response_code(406);
        echo json_encode(array('code' => 406, 'message' => 'Not Acceptable'));
        exit;
    } else {
        switch (strtolower($format)) {
            case 'json':
                $contentType = 'application/json';
                break;
            case 'pdf':
                $contentType = 'application/pdf';
                break;
            case 'html':
            default:
                $contentType = 'text/html';
                break;
        }

        drupal_add_http_header('Content-type', $contentType);
    }
}

/**
 * Invoice API dispatcher
 */
function _invoice_api_dispatch() {
    $menuItem = menu_get_item();
    $pageCallback = isset($menuItem['page_callback']) ? $menuItem['page_callback'] : null;
    $pageArguments = isset($menuItem['page_arguments']) ? $menuItem['page_arguments'] : array();

    // Check for API route
    if (strpos($pageCallback, 'invoice_api') === 0) {
        $GLOBALS['invoice_api'] = true;

        if (isset($menuItem['page_arguments'][0])) {
            $format = $menuItem['page_arguments'][0];
            $pageArguments[0] = str_replace('invoice.', '', $format);
        } else {
            $format = null;
        }

        _invoice_api_check_response_format_support(str_replace('invoice.', '', $format));
        _invoice_api_authenticate();

        // Prepend request method
        array_unshift($pageArguments, strtoupper($_SERVER['REQUEST_METHOD']));

        call_user_func_array($pageCallback, $pageArguments);
        exit;
    }
}

/**
 * Authenticates through basic HTTP authentication
 */
function _invoice_api_authenticate() {
    /**
     * Remove any previous sessions to stay stateless
     */
    $sessionId = session_id();
    _drupal_session_destroy($sessionId);

    _invoice_api_check_user_pass();
    _invoice_api_check_allowed_ips();
}

/**
 * Verifies the username and password
 */
function _invoice_api_check_user_pass() {
    require_once DRUPAL_ROOT . '/' . variable_get('password_inc', 'includes/password.inc');

    $edit['name'] = isset($_SERVER['PHP_AUTH_USER']) ? $_SERVER['PHP_AUTH_USER'] : '';
    $edit['pass'] = isset($_SERVER['PHP_AUTH_PW']) ? $_SERVER['PHP_AUTH_PW'] : '';

    $users = user_load_multiple(array(), array('name' => $edit['name'], 'status' => 1));
    $account = reset($users);

    if (user_check_password($edit['pass'], $account)) {
        // Login the user
        $GLOBALS['user'] = $account;

        // Update the user table timestamp noting user has logged in.
        // This is also used to invalidate one-time login links.
        /*$GLOBALS['user']->login = REQUEST_TIME;
        db_update('users')
          ->fields(array('login' => $GLOBALS['user']->login))
          ->condition('uid', $GLOBALS['user']->uid)
          ->execute();

        // Regenerate the session ID to prevent against session fixation attacks.
        // This is called before hook_user in case one of those functions fails
        // or incorrectly does a redirect which would leave the old session in place.
        drupal_session_regenerate();*/
    } else {
        // Authentication failed
        _invoice_api_http_response_code(401);
        drupal_add_http_header('WWW-Authenticate', 'Basic Realm="Invoice API"');
        exit;
    }
}

/**
 * Check if the request is from an allowed IP addresses
 */
function _invoice_api_check_allowed_ips() {
    $allowedIps = variable_get('invoice_api_allowed_ips', '');

    if ('' != trim($allowedIps)) {
        // Trim ips
        $ips = explode(',', $allowedIps);
        foreach ($ips as $index => $ip) {
            $ips[$index] = trim($ip);
        }

        if (!in_array($_SERVER['REMOTE_ADDR'], $ips)) {
            // Authentication failed
            _invoice_api_http_response_code(403);
            echo json_encode(array('code' => 403, 'message' => 'IP address is not allowed'));
            exit;
        }
    }
}

/**
 * Checks if the authenticated username has access to the defined template name
 */
function _invoice_api_check_allowed_templates() {
    $templateAllowed = false;
    $data = _invoice_api_get_request_data();

    if (!isset($data['template']) || '' == trim($data['template'])) {
        _invoice_api_http_response_code(400);
        echo json_encode(array('code' => 400, 'message' => 'Template is required'));
        exit;
    }

    if (variable_get('invoice_api_root_username') == $GLOBALS['user']->name) {
        $templateAllowed = true;
    }

    if (true !== $templateAllowed) {
        $templates = _invoice_get_templates();
        foreach ($templates as $template) {
            $username = _invoice_get_variable($template, 'api_username', '');
            if ('' != trim($username) && $username == $GLOBALS['user']->name
                && strtolower($template) == strtolower($data['template'])
            ) {
                $templateAllowed = true;
                break;
            }
        }
    }

    if (true !== $templateAllowed) {
        _invoice_api_http_response_code(403);
        echo json_encode(array('code' => 403, 'message' => 'Permission denied for this template'));
        exit;
    }
}

/**
 * Returns the request data
 *
 * @staticvar null|array $data
 * @return    null|array
 */
function _invoice_api_get_request_data() {
    static $data = null;

    if (null === $data) {
        $data = json_decode(file_get_contents('php://input'), true);
    }

    return $data;
}

/**
 * GET / PUT / POST / DELETE an invoice
 *
 * @param string  $requestMethod
 * @param string  $format Response format
 * @param integer $invoiceId
 */
function invoice_api_invoice($requestMethod, $format, $invoiceId = null) {
    switch ($requestMethod) {
        case 'GET':
            if ($invoiceId > 0) {
                _invoice_api_invoice_get((int) $invoiceId, $format);
            } else {
                _invoice_api_invoice_get_list();
            }
            break;
        case 'POST':
            _invoice_api_invoice_post($format);
            break;
        case 'PUT':
            _invoice_api_invoice_put((int) $invoiceId);
            break;
        case 'DELETE':
            _invoice_api_http_response_code(501);
            echo json_encode(array('code' => 501, 'message' => 'Invoices may never be deleted,'
                . ' create a credit invoice instead.'));
            exit;
            break;
        default:
            // Method not allowed
            _invoice_api_http_response_code(405);
            echo json_encode(array('code' => 405, 'message' => 'Method Not Allowed'));
            exit;
    }
}

/**
 * Handles GET request for a list of invoices
 */
function _invoice_api_invoice_get_list() {
    $options = array(
        'page' => isset($_GET['page']) ? $_GET['page'] : '', // Picked up by PagerDefault
        'max_results' => isset($_GET['max_results']) && $_GET['max_results'] > 0 ? $_GET['max_results'] : 15,
        'sort' => isset($_GET['sort']) && '' != trim($_GET['sort']) ?
            preg_replace('/[^a-zA-Z0-9\.]/', '', $_GET['sort']) : 'n.nid',
        'order' => isset($_GET['order']) && in_array($_GET['order'], array('asc', 'desc')) ? $_GET['order'] : 'desc',
        'template' => isset($_GET['template']) ? $_GET['template'] : '',
        'customer_number' => isset($_GET['customer_number']) ? $_GET['customer_number'] : '',
        'year' => isset($_GET['year']) ? $_GET['year'] : '',
        'month' => isset($_GET['month']) ? $_GET['month'] : '',
        'day' => isset($_GET['day']) ? $_GET['day'] : ''
    );

    $createDates = array();
    if ('' != $options['year'] || '' != $options['month'] || '' != $options['day']) {
        $year = '' != $options['year'] ? $options['year'] : date('Y');
        $month = '' != $options['month'] ? $options['month'] : date('m');
        $startDay = '' != $options['day'] ? $options['day'] : 1;
        $endDay = '' != $options['day'] ? $options['day'] : cal_days_in_month(CAL_GREGORIAN, $month, $year);

        $createStart = new DateTime();
        $createStart->setDate($year, $month, $startDay)->setTime(0, 0, 0);
        $createDates['start'] = $createStart;

        $createEnd = new DateTime();
        $createEnd->setDate($year, $month, $endDay)->setTime(23, 59, 59);
        $createDates['end'] = $createEnd;
    }

    try {
        $query = db_select('invoice_invoices', 'ii')->extend('PagerDefault')->limit($options['max_results']);
        $query->fields('ii', array('iid', 'nid', 'leading_zeros', 'prefix', 'pay_limit', 'pay_status',
            'description', 'uid'));
        $query->fields('c', array('customer_number', 'company_name', 'lastname', 'firstname'));
        $query->fields('n', array('created', 'changed'));

        $query->addExpression('it.name', 'template');
        $query->leftJoin('node', 'n', 'ii.nid = n.nid');
        $query->leftJoin('invoice_customers', 'c', 'ii.iid = c.invoice_id');
        $query->leftJoin('invoice_templates', 'it', 'ii.tid = it.tid');
        $query->groupBy('ii.iid')->orderBy($options['sort'], strtoupper($options['order']));

        $count_query = db_select('invoice_invoices', 'ii');
        $count_query->leftJoin('node', 'n', 'ii.nid = n.nid');
        $count_query->leftJoin('invoice_customers', 'c', 'ii.iid = c.invoice_id');
        $count_query->leftJoin('invoice_templates', 'it', 'ii.tid = it.tid');
        $count_query->addExpression('COUNT(*)');

        if ('' != $options['template']) {
            $query->condition('it.name', $options['template'], '=');
            $count_query->condition('it.name', $options['template'], '=');
        }

        if ('' != $options['customer_number']) {
            $query->condition('c.customer_number', $options['customer_number'], '=');
            $count_query->condition('c.customer_number', $options['customer_number'], '=');
        }

        if (2 == count($createDates)) {
            $query->condition('n.created', $createDates['start']->getTimestamp(), '>=');
            $query->condition('n.created', $createDates['end']->getTimestamp(), '<=');
            $count_query->condition('n.created', $createDates['start']->getTimestamp(), '>=');
            $count_query->condition('n.created', $createDates['end']->getTimestamp(), '<=');
        }

        $query->setCountQuery($count_query);
        $result = $query->execute();
    } catch(\Exception $e) {
        _invoice_api_http_response_code(500);
        echo json_encode(array('code' => 500, 'message' => $e->getMessage()));
        exit;
    }

    $rows = array();

    foreach ($result as $row) {

        // Set locale so money has the right format for the preferred culture
        $locale = _invoice_get_variable($row->template, 'locale');
        if ($locale) {
            setlocale(LC_MONETARY, $locale);
        }

        // Get invoice totals
        $totals = _invoice_get_invoice_totals($row->iid);

        // Add row
        $rows[] = array(
            'id' => (int) $row->iid,
            'number' => _invoice_get_formatted_invoice_number($row->iid, NULL, $row->created),
            'leading_zeros' => (int) $row->leading_zeros,
            'prefix' => $row->prefix,
            'template' => $row->template,
            'description' => $row->description,
            'extotal' => $totals['extotal'],
            'inctotal' => $totals['inctotal'],
            'vattotal' => $totals['vattotal'],
            'formatted_extotal' => _invoice_round_and_format_money($totals['extotal'], 2),
            'formatted_inctotal' => _invoice_round_and_format_money($totals['inctotal'], 2),
            'formatted_vattotal' => _invoice_round_and_format_money($totals['vattotal'], 2),
            'pay_status' => $row->pay_status,
            'pay_limit' => (int) $row->pay_limit,
            'uid' => (int) $row->uid,
            'created' => date('Y-m-d H:i:s', $row->created),
            'changed' => date('Y-m-d H:i:s', $row->changed),
            'customer' => array(
                'customer_number' => $row->customer_number,
                'company_name' => $row->company_name,
                'firstname' => $row->firstname,
                'lastname' => $row->lastname,
            )
        );
    }

    _invoice_api_http_response_code(200); // OK
    echo json_encode($rows);
}

/**
 * Handles GET request for the specified invoice ID
 *
 * @param integer $invoiceId
 * @param string  $format Valid values: html, json or pdf
 */
function _invoice_api_invoice_get($invoiceId, $format) {
    if (0 === $invoiceId) {
        _invoice_api_http_response_code(400);
        echo json_encode(array('code' => 400, 'message' => 'Invoice ID invalid or missing'));
        exit;
    } else {
        $row = db_query("SELECT COUNT(iid) AS count, it.api_username FROM {invoice_invoices} ii
            JOIN {invoice_templates} it ON ii.tid = it.tid
            WHERE iid = :invoiceId
            LIMIT 1",
            array(':invoiceId' => $invoiceId))->fetchAssoc();

        if ($row['count'] < 1) {
            _invoice_api_http_response_code(404);
            echo json_encode(array('code' => 404, 'message' => 'Not Found'));
            exit;
        }

        if (
            $row['api_username'] !== $GLOBALS['user']->name
            && variable_get('invoice_api_root_username') !== $GLOBALS['user']->name
        ) {
            _invoice_api_http_response_code(403);
            echo json_encode(array('code' => 403, 'message' => 'No access to the template of this invoice'));
            exit;
        }
    }

    _invoice_api_http_response_code(200); // OK

    switch ($format) {
        case 'html':
            invoice_view_print($invoiceId);
            break;
        case 'pdf':
            invoice_view_pdf($invoiceId);
            break;
        case 'json':
        default:
            _invoice_api_view_json($invoiceId);
            break;
    }
}

/**
 * Handles POST request
 *
 * @param string $format Response format
 */
function _invoice_api_invoice_post($format) {
    _invoice_api_check_allowed_templates();
    $data = _invoice_api_get_request_data();

    $invoiceModel = new InvoiceRestModel();
    $invoiceModel->exchangeArray($data);

    $transaction = db_transaction();

    try {
        $node = new stdClass();
        $node->type = 'invoice';
        $node->title = null;
        $node->language = LANGUAGE_NONE;
        $node->invoice_number = null;
        $node->user_defined_invoice_number = '';

        $invoiceModel->mapToNode($node);

        // Set some default values
        node_object_prepare($node);

        // Prepare node for a submit
        $node = node_submit($node);

        // Save the node. A nid property is available after this call.
        node_save($node);
    } catch (Exception $e) {
        $transaction->rollback();
        watchdog_exception('invoice_api', $e);
        _invoice_api_http_response_code(500);
        exit;
    }

    _invoice_api_http_response_code(302); // Found / See other

    $uri = _invoice_get_transfer_protocol() . '://' . $_SERVER['HTTP_HOST']
        . '/invoice/api/invoice.' . $format . '/' . $node->invoice_number;

    drupal_add_http_header('Location', $uri);
}

/**
 * Handles PUT request
 *
 * @param integer $invoiceId
 */
function _invoice_api_invoice_put($invoiceId) {
    if (0 === $invoiceId) {
        _invoice_api_http_response_code(400);
        echo json_encode(array('code' => 400, 'message' => 'Invoice ID invalid or missing'));
        exit;
    } else {
        $count = db_query("SELECT COUNT(iid) FROM {invoice_invoices} WHERE iid = :invoiceId",
            array(':invoiceId' => $invoiceId))->fetchField();
        if ($count < 1) {
            _invoice_api_http_response_code(404);
            echo json_encode(array('code' => 404, 'message' => 'Not Found'));
            exit;
        }
    }

    _invoice_api_check_allowed_templates();
    $data = _invoice_api_get_request_data();

    $nid = db_query("SELECT nid FROM {invoice_invoices} WHERE iid = :invoiceId",
      array(':invoiceId' => $invoiceId))->fetchField();

    $invoiceModel = new InvoiceRestModel();
    $invoiceModel->exchangeArray($data);

    $transaction = db_transaction();

    try {
        $node = node_load($nid);

        $invoiceModel->mapToNode($node);

        // Set some default values
        node_object_prepare($node);
        if (!empty($node->menu) && isset($node->menu['link']['enabled'])) {
            // Calling menu_node_prepare(), menu_node_save()
            // will cause data loss unless the enabled flag is set.
            $node->menu['link']['enabled'] = (int) (bool) $node->menu['link']['mlid'];
        }

        $node->invoice_number = $invoiceId;
        $node->user_defined_invoice_number = '';

        // Prepare node for a submit
        $node = node_submit($node);

        // Save the node.
        node_save($node);
    } catch (Exception $e) {
        $transaction->rollback();
        watchdog_exception('invoice_api', $e);
        _invoice_api_http_response_code(500);
        exit;
    }

    _invoice_api_http_response_code(204); // No content
}

/**
 * Displays the specified invoice in JSON format
 *
 * @param integer $invoiceId
 */
function _invoice_api_view_json($invoiceId) {
    $nid = db_query("SELECT nid FROM {invoice_invoices} WHERE iid = :invoiceId",
      array(':invoiceId' => $invoiceId))->fetchField();

    $invoiceModel = new InvoiceRestModel();
    $node = node_load($nid);
    $invoiceModel->mapFromNode($node);

    _invoice_api_http_response_code(200); // OK
    echo json_encode($invoiceModel->getArrayCopy());
}

/**
 * Sets HTTP response code and message
 *
 * @param  integer $code
 * @return integer
 */
function _invoice_api_http_response_code($code = NULL) {
    if ($code !== NULL) {
        switch ($code) {
            case 100: $text = 'Continue';
                break;
            case 101: $text = 'Switching Protocols';
                break;
            case 200: $text = 'OK';
                break;
            case 201: $text = 'Created';
                break;
            case 202: $text = 'Accepted';
                break;
            case 203: $text = 'Non-Authoritative Information';
                break;
            case 204: $text = 'No Content';
                break;
            case 205: $text = 'Reset Content';
                break;
            case 206: $text = 'Partial Content';
                break;
            case 300: $text = 'Multiple Choices';
                break;
            case 301: $text = 'Moved Permanently';
                break;
            case 302: $text = 'Moved Temporarily';
                break;
            case 303: $text = 'See Other';
                break;
            case 304: $text = 'Not Modified';
                break;
            case 305: $text = 'Use Proxy';
                break;
            case 400: $text = 'Bad Request';
                break;
            case 401: $text = 'Unauthorized';
                break;
            case 402: $text = 'Payment Required';
                break;
            case 403: $text = 'Forbidden';
                break;
            case 404: $text = 'Not Found';
                break;
            case 405: $text = 'Method Not Allowed';
                break;
            case 406: $text = 'Not Acceptable';
                break;
            case 407: $text = 'Proxy Authentication Required';
                break;
            case 408: $text = 'Request Time-out';
                break;
            case 409: $text = 'Conflict';
                break;
            case 410: $text = 'Gone';
                break;
            case 411: $text = 'Length Required';
                break;
            case 412: $text = 'Precondition Failed';
                break;
            case 413: $text = 'Request Entity Too Large';
                break;
            case 414: $text = 'Request-URI Too Large';
                break;
            case 415: $text = 'Unsupported Media Type';
                break;
            case 500: $text = 'Internal Server Error';
                break;
            case 501: $text = 'Not Implemented';
                break;
            case 502: $text = 'Bad Gateway';
                break;
            case 503: $text = 'Service Unavailable';
                break;
            case 504: $text = 'Gateway Time-out';
                break;
            case 505: $text = 'HTTP Version not supported';
                break;
            default:
                exit('Unknown http status code "' . htmlentities($code) . '"');
                break;
        }

        $protocol = (isset($_SERVER['SERVER_PROTOCOL']) ? $_SERVER['SERVER_PROTOCOL'] : 'HTTP/1.0');
        header($protocol . ' ' . $code . ' ' . $text);
        $GLOBALS['http_response_code'] = $code;
    } else {
        $code = (isset($GLOBALS['http_response_code']) ? $GLOBALS['http_response_code'] : 200);
    }

    return $code;
}