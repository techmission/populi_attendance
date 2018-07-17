<?php

/* Define general error messages */
define('ERROR_EXCEPTION', 'A fatal exception has occurred.');
define('ERROR_PHP_FATAL_ERROR', 'A fatal PHP error has occurred.');
define('ERROR_PHP_WARNING', 'A PHP warning has occurred.');
define('ERROR_PHP_OTHER', 'A PHP error has occurred.');

/* Logging function */
function script_log($message, $level = LEVEL_DEBUG) {
  // Open log file for writing, in append mode
  // See https://stackoverflow.com/questions/24972424
  if($level == LEVEL_DEBUG) {
    $level_name = 'Debug';
  }
  else if($level == LEVEL_ERROR) {
    $level_name = 'Error';
  }
  // Log all messages in debug mode, or only errors if in default mode.
  if(LOG_MODE == MODE_DEBUG || (LOG_MODE == MODE_DEFAULT && $level == LEVEL_ERROR)) {
    $log_line = CURTIME . ' [' . $level_name . '] : ' . $message . PHP_EOL;
    // FIXME: replace with a logging class that uses SQL.
    $result = file_put_contents(LOG_DIRECTORY . '/' . LOGFILE, $log_line, FILE_APPEND | LOCK_EX);
  }
  // Result is the number of bytes logged, or FALSE on failure.
  return $result;
}

/* Function to handle the PDOExceptions with our logger & Twig error page
   - https://phpdelusions.net/delusion/try-catch */
/* set with set_exception_handler('custom_exception_handler') */

function custom_exception_handler($ex) {
  global $TWIG;
  header('HTTP/1.1 500 Internal Server Error', TRUE, 500);
  script_log($ex->getMessage(), LEVEL_ERROR);
  build_error_page(ERROR_EXCEPTION, array('error_info' => $ex));
}

/* set with set_exception_handler('error_page_handler') */
function error_page_handler ($code, $message, $file, $line, $context = NULL) {
  global $TWIG;
  if($code === E_ERROR || $code === E_WARNING) {
    header('HTTP/1.1 500 Internal Server Error', TRUE, 500);
    // Log the error
    script_log($message . ' in ' . $file . ' on ' . $line, LEVEL_ERROR);
    // Build the error page (get_defined_vars() is an array of the variables in the function)
    $error_code_msg = get_error_code_msg($code);
    build_error_page($error_code_msg, array('error_info' => get_defined_vars()));
  }
  // Otherwise, use regular error handler
  else {
    return FALSE;
  }
}


/* shutdown function to catch fatal errors - this gets called on script exit, even if fatal error */
/* https://insomanic.me.uk/php-trick-catching-fatal-errors-e-error-with-a-custom-error-handler-cea2262697a2 */
function fatal_error_handler() {
  // Get last error
  $last_error = error_get_last();
  if ($last_error['type'] === E_ERROR) {
    // fatal error
    error_page_handler(E_ERROR, $last_error['message'], $last_error['file'], $last_error['line']);
  }
}

/**
 * Gets the caller of the function where this function is called from
 * @param string what to return? 
 *  (Leave empty to get all, or specify: "class", "function", "line", "class", etc.) - options see: http://php.net/manual/en/function.debug-backtrace.php
 * https://stackoverflow.com/questions/190421/caller-function-in-php-5
 */
function get_caller($what = NULL)
{
    $trace = debug_backtrace();
    $previous_call = $trace[2]; // 0 is this call, 1 is call in previous function, 2 is caller of that function

    if(isset($what))
    {
        return $previous_call[$what];
    }
    else
    {
        return $previous_call;
    }   
}

function get_error_code_msg($error_code) {
	if($error_code == E_ERROR) {
       $error_code_msg = ERROR_PHP_FATAL_ERROR;
	}
	else if($error_code = E_WARNING) {
		$error_code_msg = ERROR_PHP_WARNING;
	}
	else {
		$error_code_msg = ERROR_PHP_OTHER;
	}
	return $error_code_msg;
}