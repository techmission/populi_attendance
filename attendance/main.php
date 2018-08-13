<?php

/* Bootstrap the codebase */

chdir(dirname(__FILE__));

require_once('controller.inc'); // add main business logic
bootstrap(); // add rest of code

// Render using Twig.
// See https://twig.sensiolabs.org/doc/1.x/api.html#twig-for-developers
global $TWIG;
// Debugging vars
global $VARS;

// will be used to render data later
$template_file = NULL;

// Access control: if not using the GUID, then render an access denied template.
if(!isset($_GET['key']) && $_GET['key'] != GUID) {
  $template = $TWIG->load('access_denied.html');
  echo $template->render(array('vars' => $VARS));
}

// Build the attendance data array and accompanying settings for rendering via Twig; uses data from $_GET
// Current setting is ?show_grades=1 or 0 to show the grades as well as submission dates
// Uses a different builder function depending on whether or not there is a Populi course id passed in $_GET
if(isset($_GET['course_id']) && is_numeric($_GET['course_id'])) {
  $twig_vars = build_single_course_attendance_data();
  $template_file = TEMPLATE_SINGLE_COURSE;
}
else {
  $twig_vars = build_attendance_data();
  $template_file = TEMPLATE_ATTENDANCE;
}

/* Add debugging variables to Twig template */
// Preformatted attendance array
$twig_vars['a_pre'] = print_r($twig_vars['a'], TRUE);
// Internal twig debugging variables
$twig_vars['vars'] = $VARS;

// Render the attendance data using the default template
$template = $TWIG->load($template_file);
echo $template->render($twig_vars);
?>
