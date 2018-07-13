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

/* Get the actual query that was run - for debugging
   from https://stackoverflow.com/questions/210564/getting-raw-sql-query-string-from-pdo-prepared-statements */
/* TODO: get this to add quotes properly */
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

    if(LOG_MODE == MODE_DEBUG) {
	  $caller = get_caller();
      script_log($query . ' in: ' . $caller['function'] . ', line ' . $caller['line'], LEVEL_DEBUG);
    }
}