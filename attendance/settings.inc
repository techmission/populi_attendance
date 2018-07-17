<?php

/* Script execution mode for logging, using the logging script from my Populi API framework */
/* Typically put at MODE_DEFAULT in production to log only errors not debugging */
/* MODE_DEBUG shows errors on screen as well as in log. */
define('LOG_MODE', MODE_DEBUG);

/* Set whether to debug Twig or not */
define('DEBUG_TWIG', FALSE);

/* Name of logfile */
define('LOGFILE', 'attendance.log');

/* Name of timezone for City Vision - Central Time 
   cf. https://stackoverflow.com/questions/4989209.
   This is the timezone used in Populi also.
*/
define('CUR_TZ', 'America/Chicago');