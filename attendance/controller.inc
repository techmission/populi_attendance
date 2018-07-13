<?php

/* File with the main business logic that builds the attendance page and the error page */

/* Includes all the rest of the needed files. */
function bootstrap() {
  global $TWIG;
  global $DB;

  // Add includes for this codebase
  require_once('../includes.inc');      // PDO and logging classes (not Populi at the moment)
  require_once('constants.inc');        // defines constants
  require_once('settings.inc');         // defines settings that can change - script wide
  require_once('queries.inc');          // the queries that build the attendance data array - main business logic

  // Add Twig.
  require_once('vendor/autoload.php');
  $loader = new Twig_Loader_Filesystem('templates');
  // Added auto_reload for debugging
  if(LOG_MODE == MODE_DEBUG) {
    $twig_cache_dir = FALSE;
  }
  else {
    $twig_cache_dir = 'twig_cache';
  }
  $TWIG = new Twig_Environment($loader, array('cache' => $twig_cache_dir, 'auto_reload' => TRUE));

  // Connect to database using PDO
  $DB = db_connect();

  // add a custom exception handler to make PDO exceptions go to the log, as well as error handlers for other errors
  // see https://phpdelusions.net/delusion/try-catch
  // see https://stackoverflow.com/questions/841500
  // see https://insomanic.me.uk/php-trick-catching-fatal-errors-e-error-with-a-custom-error-handler-cea2262697a2
  set_error_handler('error_to_exception');
  set_exception_handler('error_page_handler');
  register_shutdown_function('fatal_error_handler');
}

/* Function to build the attendance page */
function build_attendance_data() {
  $attendance_data = array();
  $personid = '';
  $output = '';
  if(!empty($_GET['firstname'] && $_GET['lastname'])) {
    $personid = get_personid($_GET['firstname'], $_GET['lastname']);
    // TODO: more robust checking in is_populi_id
    if(!is_populi_id($personid)) {
      build_error_page(ERROR_STUDENT_NOT_FOUND, array('firstname' => $_GET['firstname'], 'lastname' => $_GET['lastname'], 'personid' => $personid));
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
  if(LOG_MODE == MODE_DEFAULT) {
    $log_mode = 'Default';
  }
  else {
    $log_mode = 'Debug';
  }
  $twig_vars = array('error_msg' => $error_msg, 'log_mode' => $log_mode);
  if(isset($vars['error_info'])) {
    $twig_vars['error_info'] = $vars['error_info'];
	unset($vars['error_info']);
  }
  // get a rendered array of the vars as a variable to show
  $vars_dump = print_r($vars, TRUE);
  $twig_vars['vars_dump'] = $vars_dump;
  echo $template->render($twig_vars);
  script_log($error_msg, LEVEL_ERROR);
  exit(-1);
}
