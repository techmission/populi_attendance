<?php

/* Main business logic function: queries Salesforce and populates the attendance data array */
function get_attendance_data($student_id) {
  // Create empty shell of array
  // NOTE: Would be better as class but would take longer to code
  $attendance_data = build_attendance_data_array();

  return $attendance_data;
}

/* Step 0: Looks up a Populi student record based on either email 
   or first name + last name */
function get_personid($firstname, $lastname) {
  $personid = NULL;
  // pdo query
  return $personid;
}


/* Utility functions */

/* Validates whether data is in the correct format to render */
function is_attendance_data_valid($attendance_data) {
  if(is_array($attendance_data) && count($attendance_data) > 0) {
    return TRUE;
  }
  else {
    return FALSE;
  }
  // TODO: add more validation in here, such as checking for non-zero values
  // for overall data
}

/* Creates shell of attendance data array */
function build_attendance_data_array() {
  $attendance_data = array();
  $attendance_data['terms'] = array();
  return $attendance_data;
}

