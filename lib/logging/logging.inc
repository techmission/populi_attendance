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
