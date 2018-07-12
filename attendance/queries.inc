<?php

/* Main business logic function: queries Salesforce and populates the attendance data array */
function get_attendance_data($personid, $firstname, $lastname) {
  $attendance_data = array();
  $attendance_data['firstname'] = $firstname;
  $attendance_data['lastname'] = $lastname;
  
  // Step 1: Get terms in which person was enrolled
  // This will be the base level of the attendance data array
  $terms = get_terms_by_person($personid);
  if(is_array($terms)) {
	$attendance_data['terms'] = $terms;
  }
  
  // Step 2: For each term, get the courses, and add to array
  foreach($attendance_data['terms'] as $termid => $term) {
	$courses = get_courses_by_term($personid, $termid);
    $attendance_data['terms'][$termid]['courses'] = $courses;
	//  Step 3: For each course, get the lessons, and add to array
	foreach($attendance_data['terms'][$termid]['courses'] as $courseid => $courses) {
	  $lessons = get_lessons_by_course($courseid);
	  $attendance_data['terms'][$termid]['courses'][$courseid]['lessons'] = $lessons;
	  // Step 4: For each lesson get the assignments and add to array
	  foreach($attendance_data['terms'][$termid]['courses'][$courseid]['lessons'] as $lessonid => $lessons) {
	    $assignments = get_assignments_by_lessons($lessonid);
		 // Step 5: For each assignment, get the submission info and add to array
	    foreach($assignments as $assignmentid => $assignment) {
	      $submission_info = get_assignment_submission_info($assignmentid, $personid);
		  // add the array of assignment info and submission info together, preserving keys
		  // https://stackoverflow.com/questions/3292044
		  $assignments[$assignmentid] = $assignments[$assignmentid] + $submission_info;
		}
	    $attendance_data['terms'][$termid]['courses'][$courseid]['lessons'][$lessonid]['assignments'] = $assignments;
	  }
	}
  }
  
  return $attendance_data;
}

/* Step 0: Get a Populi student ID based on firstname and lastname */
function get_personid($firstname, $lastname) {
  global $DB;
  $personid = NULL;
  // pdo query
  try {
	$qry = 'SELECT personid FROM mv_students_1718 WHERE firstname = :firstname AND lastname = :lastname';  
    $stmt = $DB->prepare($qry);
	$stmt->bindValue(':firstname', $firstname, PDO::PARAM_STR);
	$stmt->bindValue(':lastname', $firstname, PDO::PARAM_STR);
	$stmt->execute();
    $results = $stmt->fetchAll(PDO::FETCH_ASSOC);
  }
  catch(PDOException $ex) {
    script_log($ex->getMessage(), LEVEL_ERROR);
  }
  return $personid;
}

/* Step 1: Get terms in which person was enrolled */
function get_terms_by_person($personid) {
  global $DB;
  // pdo query
  try {
    $qry = <<<EOT
SELECT termid, term_name FROM mv_courses_1718 
JOIN mv_enrollments_1718 ON c.instanceid = e.instanceid JOIN mv_students_1718 ON s.personid = e.studentid 
WHERE personid = :personid 
GROUP BY termid
ORDER BY term_start_date ASC
EOT;
	$stmt = $DB->prepare($qry);
	$stmt->bindValue(':personid', $personid, PDO::PARAM_INT);
    $results = $stmt->fetchAll(PDO::FETCH_ASSOC);
  }
  catch(PDOException $ex) {
    script_log($ex->getMessage(), LEVEL_ERROR);
  }
  // array with termids as keys and names as values, sorted chronologically
  return $terms;
}

/* Step 2: For each term, get the courses */
function get_courses_by_term($personid, $termid) {
  global $DB;	
  // pdo query
  try {
    $qry = <<<EOT
SELECT name, term_name, start_date FROM mv_courses_1718
JOIN mv_enrollments_1718 ON c.instanceid = e.instanceid JOIN mv_students_1718 ON s.personid = e.studentid
WHERE personid = :personid AND termid = :termid
GROUP BY instanceid
ORDER BY name asc
EOT;
	$stmt = $DB->prepare($qry);
	$stmt->bindValue(':personid', $firstname, PDO::PARAM_INT);
	$stmt->bindValue(':termid', $firstname, PDO::PARAM_INT);
	$stmt->execute();
    $results = $stmt->fetchAll(PDO::FETCH_ASSOC);
  }
  catch(PDOException $ex) {
    script_log($ex->getMessage(), LEVEL_ERROR);
  }
  // array of courses with courseids as keys, sorted by name
  return $courses;
}

/* Step 3: For each course, get the lessons */
function get_lessons_by_course($courseid) {
  global $DB;	
  // pdo query
  try {
	$qry = <<<EOT
SELECT l.lessonid, l.name, l.orderid from course_lessons l join course_assignments c 
ON c.lessonid = l.lessonid
WHERE l.instanceid = :courseid
GROUP BY l.lessonid
ORDER BY l.orderid ASC;
EOT;
    $stmt = $DB->prepare($qry);
	$stmt->bindValue(':courseid', $courseid, PDO::PARAM_INT);
	$stmt->execute();
    $results = $stmt->fetchAll(PDO::FETCH_ASSOC);
  }
  catch(PDOException $ex) {
    script_log($ex->getMessage(), LEVEL_ERROR);
  }
  // array of lessons with lesson ids as keys, with names, sorted by order in course
  return $lessons;
}

/* Step 4: For each lesson get the assignments */
function get_assignments_by_lesson($lessonid) {
  global $DB;
  // pdo query
  try {
	$qry = <<<EOT
SELECT assignment_id, name, type, status, timedue FROM course_assignments
WHERE lessonid= :lessonid
ORDER BY orderid ASC, name ASC, timedue ASC
EOT;
    $stmt = $DB->prepare($qry);
	$stmt->bindValue(':lessonid', $lessonid, PDO::PARAM_INT);
    $results = $stmt->fetchAll(PDO::FETCH_ASSOC);
  }
  catch(PDOException $ex) {
    script_log($ex->getMessage(), LEVEL_ERROR);
  }	
  // array of assignments, with names, sorted by order in lesson
  return $assignments;
}

/* Step 5: For each assignment, get the submission info  */
function get_assignment_submission_info($assignmentid, $personid) {
  global $DB;
  // 1. get submission dates
  // if forum, get submission dates from discussion table
  if($assignment['type'] == 'DISCUSSION') {
    // pdo queries
	try {
      // TODO: add queries
	  $stmt = $db->query($qry);
      $results = $stmt->fetchAll(PDO::FETCH_ASSOC);
    }
    catch(PDOException $ex) {
      script_log($ex->getMessage(), LEVEL_ERROR);
    }
  }
  else {
    // pdo queries (from either my report, or probably directly to the assignment_submissions table)
	try {
      // TODO: add queries
	  $stmt = $db->query($qry);
      $results = $stmt->fetchAll(PDO::FETCH_ASSOC);
    }
    catch(PDOException $ex) {
      script_log($ex->getMessage(), LEVEL_ERROR);
    }
  }
  // 2. format the dates, or put in “--” if none
  // 3. Get the grades on the submissions (if there are submissions)
  //     Put in “--” if no submission, or no grade (for now, don’t differentiate between those cases)
  return $submission_info;  // this array will be merged back into the assignment array
}



/**
  Utility functions 
 */

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

/* Validates a Populi ID */
function is_populi_id($id) {
  if(is_numeric($id) && $id > 0) {
	return TRUE;
  }
  else {
	return FALSE;
  }
}

