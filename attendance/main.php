<?php

/* Bootstrap the codebase */

chdir(dirname(__FILE__));

require_once('controller.inc'); // add main business logic
bootstrap();                    // add rest of code

// Build the attendance data array; uses data from $_GET
$attendance_data = build_attendance_data();

// Render using Twig.
// See https://twig.sensiolabs.org/doc/2.x/api.html#twig-for-developers
global $TWIG;
// Debugging vars
global $VARS;

$template_file = 'attendance.html';

$template = $TWIG->load(TEMPLATE_FILE);
echo $template->render(array('a' => $attendance_data, 'vars' => $VARS));
?>
