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
  // Add internationalization and set locale, for date formatting by timezone (requires Twig extensions)
  $TWIG->addExtension(new Twig_Extensions_Extension_Intl());
  // Add date extension for date differences
  $TWIG->addExtension(new Twig_Extensions_Extension_Date());
  // English (United States)
  Locale::setDefault('en-US');

  // Connect to database using PDO
  $DB = db_connect();

  // add a custom exception handler to make PDO exceptions go to the log, as well as error handlers for other errors
  // see https://phpdelusions.net/delusion/try-catch
  // see https://stackoverflow.com/questions/841500
  // see https://insomanic.me.uk/php-trick-catching-fatal-errors-e-error-with-a-custom-error-handler-cea2262697a2
  set_error_handler('error_page_handler');
  set_exception_handler('custom_exception_handler');
  register_shutdown_function('fatal_error_handler');
}

/* Function to build the attendance page */
function build_attendance_data() {
  $attendance_data = array();
  $personid = NULL;
  $courseid = NULL;
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
  
  // If a course id is specified in the query string, only load attendance data for that course
  if(isset($_GET['course_id']) && is_numeric($_GET['course_id'])) {
    $attendance_data = get_attendance_data($personid, $_GET['firstname'], $_GET['lastname'], $_GET['course_id']);
  }
  else {
    $attendance_data = get_attendance_data($personid, $_GET['firstname'], $_GET['lastname']);
  }

  // check whether to show the grades or not based on query string parameter
  if(isset($_GET['show_grades']) && $_GET['show_grades'] == TRUE) {
    $show_grades = TRUE;
  }
  else {
    $show_grades = FALSE;
  }

  // check whether to show the grades or not based on query string parameter
  // only show the grade dates if the grades themselves are showing, to prevent an odd display
  if(isset($_GET['show_grade_dates']) && $_GET['show_grade_dates'] == TRUE && $show_grades == TRUE) {
    $show_grade_dates = TRUE;
  }
  else {
    $show_grade_dates = FALSE;
  }

  // show days later if requested
  if(isset($_GET['show_days_later']) && $show_grades == TRUE) {
    $show_days_later = TRUE;
  }
  else {
    $show_days_later = FALSE;
  }
  
  // return to entry-point PHP page if there is valid attendance data, passing Twig variables
  // set to show grades and/or show grade dates if those was passed in the query string
  
  // TODO: more robust checking in is_attendance_data_valid
  if(is_attendance_data_valid($attendance_data)) {
    $twig_vars = array('a' => $attendance_data, 'show_grades' => $show_grades, 'show_grade_dates' => $show_grade_dates, 'show_days_later' => $show_days_later);
    return $twig_vars;
  }
  // if not, return an error page
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
    if(is_array($vars['error_info'])) {
      if(isset($vars['error_info']['TWIG'])) {
        unset($vars['error_info']['TWIG']);
      }
      $vars['error_info'] = print_r($vars['error_info'], TRUE);
    }
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
