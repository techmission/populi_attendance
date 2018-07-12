<?php

/* File to define constants used in the codebase. */

/* Debug logging levels */
define('LOG_ALL', 0);    // log_to_screen()
define('LOG_SOME', 1);   // templates
define('LOG_ERRORS', 2); // errors only (production)
define('LOG_NONE', 3);   // silence

/* Error messages */
define('ERROR_STUDENT_NOT_FOUND', 'A student was not found based on the provided information.');
define('ERROR_INVALID_INPUT', 'The provided input could not be used to query Populi for a student id.');
define('ERROR_INVALID_DATA', 'The attendance data was in an incorrect format to render.');