<?php

/* File with the main business logic that builds the attendance page and the error page */

/* Includes all the rest of the needed files. */
function bootstrap() {
  global $TWIG;
  global $DB;
  
  // Add Twig.
  // TODO: Confirm this is the proper location for Twig.
  require_once('vendor/autoload.php');
  $loader = new Twig_Loader_Filesystem('templates');
  // Added auto_reload for debugging
  $TWIG = new Twig_Environment($loader, array('cache' => 'twig_cache', 'auto_reload' => TRUE));
  
  // Add includes for this codebase
  require_once('../includes.inc');      // PDO and logging classes (not Populi at the moment)
  require_once('constants.inc');        // defines constants
  require_once('settings.inc');         // defines settings that can change - script wide
  require_once('queries.inc');          // the queries that build the attendance data array - main business logic
  
  // Connect to database using PDO
  $DB = db_connect();
  
  // add a custom exception handler to make PDO exceptions go to the log
  // see https://phpdelusions.net/delusion/try-catch
  set_exception_handler('custom_exception_handler');
}

/* Function to build the attendance page */
function build_attendance_data() {
  $attendance_data = array();
  $personid = '';
  $output = '';
  if(!empty($_GET['firstname'] && $_GET['LastName'])) {
    $personid = get_personid($_GET['firstname'], $_GET['lastname']);
    // TODO: more robust checking in is_populi_id
    if(!is_populi_id($personid)) {
      build_error_page(ERROR_CONTACT_NOT_FOUND, array('firstname' => $_GET['firstname'], 'lastname' => $_GET['lastname'], 'personid' => $personid));
    }
  }
  else {
    // sanitized version of $_GET array: http://php.net/manual/en/function.filter-var-array.php
    $get_array_sanitized = filter_var_array($_GET, FILTER_SANITIZE_STRING);
    build_error_page(ERROR_INVALID_INPUT, array('get_params' => $get_array_sanitized));
  }
  
  $attendance_data = get_attendance_data($personid, $_GET['firstname'], $_GET['lastname']);
  // TODO: more robust checking in is_attendance_data_valid
  if(is_attendance_data_valid($attendance_data)) {
    return $attendance_data;
  }
  else {
    build_error_page(ERROR_INVALID_DATA, array('attendance_data' => $attendance_data));
  }
}

/* Function to build the error page */
function build_error_page($error_msg, $vars) {
  global $TWIG;
  $template = $TWIG->load('error.html');
  // get a rendered array of the vars as a variable to show
  $vars_dump = print_r($vars, TRUE);
  echo $template->render(array('error_msg' => $error_msg, 'vars' => $vars, 'vars_dump' => $vars_dump, 'log_mode' => LOG_MODE));
  script_log($error_msg, LEVEL_ERROR);
  exit(-1);
}
