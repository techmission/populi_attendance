<?php

/* Script execution mode for logging, using the logging script from my Populi API framework */
/* Typically put at MODE_DEFAULT in production to log only errors not debugging */
define('LOG_MODE', MODE_DEBUG);

/* Set whether to debug Twig or not */
define('DEBUG_TWIG', TRUE);

/* Name of logfile */
define('LOGFILE', 'attendance.log');
