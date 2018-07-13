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

// Build the attendance data array; uses data from $_GET
$attendance_data = build_attendance_data();

$template = $TWIG->load(TEMPLATE_FILE);
echo $template->render(array('a' => $attendance_data, 'vars' => $VARS, 'a_pre' => print_r($attendance_data, TRUE)));
?>
