<?php

/* Wrapper function to create the PDO database */
function db_connect() {
  // Use the constants in the connection string.
  // Throw exceptions on errors for more robust handling.
  // See http://wiki.hashphp.org/PDO_Tutorial_for_MySQL_Developers
  $db = new PDO('mysql:host=' . DB_HOST . ';dbname=' . DB_NAME . ';charset=' . DB_CHARSET, DB_USERNAME, DB_PASSWORD, 
    array(PDO::ATTR_EMULATE_PREPARES => FALSE, PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION));
  return $db;
}

/* Function to handle the PDOExceptions with our logger 
   - https://phpdelusions.net/delusion/try-catch */
/* set with set_exception_handler('custom_exception_handler') */

function custom_exception_handler($ex) {
  header('HTTP/1.1 500 Internal Server Error', TRUE, 500);
  script_log($ex->getMessage(), LEVEL_ERROR);
  if(LOG_MODE == MODE_DEBUG) {
    echo '<pre>' . $ex->getMessage() . '</pre>';
  }
  else {
    echo '<p>A fatal error has occurred.</p>';
  }
}

/* Get the actual query that was run - for debugging
   from https://stackoverflow.com/questions/210564/getting-raw-sql-query-string-from-pdo-prepared-statements */
function log_query($query, $params) {
    $keys = array();

    // build a regular expression for each parameter
    foreach ($params as $key => $value) {
        if (is_string($key)) {
            $keys[] = '/:'.$key.'/';
        } else {
            $keys[] = '/[?]/';
        }
    }

    $query = preg_replace($keys, $params, $query, 1, $count);

    //if(LOG_MODE == MODE_DEBUG) {
      script_log($query, LEVEL_DEBUG);
    //}
}