<?php

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
  build_error_page(ERROR_EXCEPTION, array('exception' => $ex));
}

/* Turn fatal errors to exceptions - https://stackoverflow.com/questions/841500 */
class ErrorOrWarningException extends Exception
{
    protected $_Context = null;
    public function getContext()
    {
        return $this->_Context;
    }
    public function setContext( $value )
    {
        $this->_Context = $value;
    }

    public function __construct( $code, $message, $file, $line, $context )
    {
        parent::__construct( $message, $code );

        $this->file = $file;
        $this->line = $line;
        $this->setContext( $context );
    }
}

/* set with set_exception_handler('error_to_exception') */
function error_to_exception( $code, $message, $file, $line, $context ) {
  if($code == E_ERROR || $code == E_WARNING) {
    throw new ErrorOrWarningException( $code, $message, $file, $line, $context );
  }
  else {
    return FALSE;
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