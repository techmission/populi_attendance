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

// Access control: if not using the GUID, then render an access denied template.
if(!isset($_GET['key']) && $_GET['key'] != GUID) {
  $template = $TWIG->load('access_denied.html');
  echo $template->render(array('vars' => $VARS));
}

// Build the attendance data array and accompanying settings for rendering via Twig; uses data from $_GET
// Current setting is ?show_grades=1 or 0 to show the grades as well as submission dates
$twig_vars = build_attendance_data();

// Add debugging variables to twig
$twig_vars['a_pre'] = print_r($attendance_data, TRUE);
$twig_vars['vars'] = $VARS;

// Render the attendance data using the default template
$template = $TWIG->load(TEMPLATE_FILE);
echo $template->render($twig_vars);
?>
