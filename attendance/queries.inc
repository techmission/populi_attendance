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
        $assignments = get_assignments_by_lesson($lessonid);
		// script_log(print_r($assignments, TRUE), LEVEL_DEBUG);
         // Step 5: For each assignment, get the submission info and add to array
        foreach($assignments as $assignmentid => $assignment) {
          $submission_info = get_assignment_submission_info($assignmentid, $assignment['type'], $personid);
          // add the array of assignment info and submission info together, preserving keys
          // https://stackoverflow.com/questions/3292044
          $assignments[$assignmentid] = $assignments[$assignmentid] + $submission_info;
        }
		//script_log(print_r($assignments, TRUE), LEVEL_DEBUG);
		// Testing the format on an assignment (debug only)
		//$assignments = array(array('type_fmt' => 'Assignment', 'name' => 'test', 'max_submission_date' => '2017-09-09 18:26:38', 'grade_letter' => 'A'));
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
  // pdo query (checked)
  $qry = 'SELECT personid FROM mv_students_1718 WHERE firstname = :firstname AND lastname = :lastname LIMIT 1';  
  $stmt = $DB->prepare($qry);
  // log_query($qry, array('firstname' => $firstname, 'lastname' => $lastname));
  $stmt->bindValue('firstname', $firstname, PDO::PARAM_STR);
  $stmt->bindValue('lastname', $lastname, PDO::PARAM_STR);
  $stmt->execute();
  // fetch a single datum
  $personid = $stmt->fetchColumn();
  return $personid;
}

/* Step 1: Get terms in which person was enrolled */
function get_terms_by_person($personid) {
  global $DB;
  // pdo query (checked)
  $terms = NULL;
  // exclude Summer 2018 term since it is not in the HCM submission
  $qry = <<<EOT
SELECT c.termid, c.term_name AS name FROM mv_courses_1718 c 
JOIN mv_enrollments_1718 e ON c.instanceid = e.instanceid JOIN mv_students_1718 s ON s.personid = e.studentid 
WHERE s.personid = :personid 
AND c.termid != 191581
GROUP BY c.termid 
ORDER BY c.term_start_date ASC
EOT;
  $stmt = $DB->prepare($qry);
  // log_query($qry, array('personid' => $personid));
  $stmt->bindValue('personid', $personid, PDO::PARAM_INT);
  $stmt->execute();
  // debug statement (note that this requires PHP to be upgraded to work properly)
  //$stmt->debugDumpParams();
  
  // fetch array with termids as keys and names as values, sorted chronologically
  $terms = $stmt->fetchAll(PDO::FETCH_UNIQUE);
  return $terms;
}

/* Step 2: For each term, get the courses */
function get_courses_by_term($personid, $termid) {
  global $DB;    
  // pdo query (checked)
  $courses = NULL;
  $qry = <<<EOT
SELECT c.instanceid, c.name FROM mv_courses_1718 c 
JOIN mv_enrollments_1718 e ON c.instanceid = e.instanceid JOIN mv_students_1718 s ON s.personid = e.studentid 
WHERE s.personid = :personid and c.termid = :termid 
GROUP BY e.instanceid 
ORDER BY c.name asc
EOT;
  $stmt = $DB->prepare($qry);
  // log_query($qry, array('personid' => $personid, 'termid' => $termid));
  $stmt->bindValue('personid', $personid, PDO::PARAM_INT);
  $stmt->bindValue('termid', $termid, PDO::PARAM_INT);
  $stmt->execute();
  // fetch array of courses with courseids as keys, with names, sorted by name
  $courses = $stmt->fetchAll(PDO::FETCH_UNIQUE);
  return $courses;
}

/* Step 3: For each course, get the lessons */
function get_lessons_by_course($courseid) {
  global $DB;
  $lessons = NULL;
  // pdo query (checked)
  $qry = <<<EOT
SELECT l.lessonid, l.name from course_lessons l join course_assignments c 
ON c.lessonid = l.lessonid 
WHERE l.instanceid = :courseid 
GROUP BY l.lessonid 
ORDER BY l.orderid ASC;
EOT;
  $stmt = $DB->prepare($qry);
  // log_query($qry, array('courseid' => $courseid));
  $stmt->bindValue('courseid', $courseid, PDO::PARAM_INT);
  $stmt->execute();
  // fetch array
  $lessons = $stmt->fetchAll(PDO::FETCH_UNIQUE);
  // array of lessons with lesson ids as keys, with names, sorted by order in course
  return $lessons;
}

/* Step 4: For each lesson get the assignments */
function get_assignments_by_lesson($lessonid) {
  global $DB;
  // pdo query (checked)
  // exclude deleted assignments and ones that have no submission associated with them
  // also exclude the "required books" assignments
  $qry = <<<EOT
SELECT assignmentid, name, type, status, timedue FROM course_assignments 
WHERE lessonid = :lessonid 
AND status = 'ACTIVE' AND type NOT IN ('ATTENDANCE', 'NEITHER') 
AND name NOT LIKE '%Required Books%'
ORDER BY orderid ASC, name ASC, timedue ASC
EOT;
  $stmt = $DB->prepare($qry);
  // log_query($qry, array('lessonid' => $lessonid));
  $stmt->bindValue('lessonid', $lessonid, PDO::PARAM_INT);
  $stmt->execute();
  // fetch array
  $assignments = $stmt->fetchAll(PDO::FETCH_UNIQUE);
  // array of assignments, with names, sorted by order in lesson
  return $assignments;
}

/* Step 5: For each assignment, get the submission info  */
function get_assignment_submission_info($assignmentid, $assignment_type, $personid) {
  global $DB;
  $submission_date_info = NULL;
  $grade_pct = NULL;
  $submission_info = array();

  // 1. Get submission dates
  // if forum, get submission dates from discussion table
  if($assignment_type == 'DISCUSSION') {
    // pdo query (checked)
    $qry = <<<EOT
select f.topic_id, min(fc.added_at) as min_added_time, max(fc.added_at) as max_added_time from forum_topics f 
join forum_topic_comments fc on f.topic_id = fc.topic_id 
where f.assignment_id = :assignmentid and fc.added_by = :personid and fc.parent_comment_id = 0 group by f.assignment_id
EOT;
    $stmt = $DB->prepare($qry);
    //log_query($qry, array('assignmentid' => $assignmentid, 'personid' => $personid));
    $stmt->bindValue('assignmentid', $assignmentid, PDO::PARAM_INT);
    $stmt->bindValue('personid', $personid, PDO::PARAM_INT);
    $stmt->execute();
    // fetch a single row
    $submission_date_info = $stmt->fetch(PDO::FETCH_ASSOC);
  }
  // if test, query the tests table for when a test was taken
  // Add group by for test id, in case they took it multiple times
  else if($assignment_type == 'TEST') {
    $qry = <<<EOT
select t.testid, min(ati.end_time) as min_added_time, max(ati.end_time) as max_added_time from assignment_test_instances ati 
join assignment_tests t on t.testid = ati.testid 
where ati.grade_coef != 0
and t.assignmentid = :assignmentid
and ati.studentid = :personid
group by t.testid
EOT;
    $stmt = $DB->prepare($qry);
    // log_query($qry, array('assignmentid' => $assignmentid, 'personid' => $personid));
    $stmt->bindValue('assignmentid', $assignmentid, PDO::PARAM_INT);
    $stmt->bindValue('personid', $personid, PDO::PARAM_INT);
    $stmt->execute();
    // fetch a single row
    $submission_date_info = $stmt->fetch(PDO::FETCH_ASSOC);
  }
  // otherwise, query the assignment_submissions table:
  // need to show only submissions by the student, where there was a file submitted
  else {
    $qry = <<<EOT
select min(a.added_time) as min_added_time, max(a.added_time) as max_added_time from assignment_submissions a 
where a.assignment_id = :assignmentid and a.student_id = :personid and a.added_by = :personid 
and a.file_id != '' 
group by a.assignment_id
EOT;
    $stmt = $DB->prepare($qry);
    //log_query($qry, array('assignmentid' => $assignmentid, 'personid' => $personid));
    $stmt->bindValue('assignmentid', $assignmentid, PDO::PARAM_INT);
    $stmt->bindValue('personid', $personid, PDO::PARAM_INT);
    $stmt->execute();
    // fetch a single row
    $submission_date_info = $stmt->fetch(PDO::FETCH_ASSOC);
  }
  
  // 2. Prepare date objects for minimum and maximum dates
  // Put in NULL if no date (use Twig to format dates - see https://twig.symfony.com/doc/1.x/filters/date.html)
  $submission_info['min_submission_date'] = get_submission_date($submission_date_info, 'min');
  $submission_info['max_submission_date'] = get_submission_date($submission_date_info, 'max');
  
  // 3. get the grade, if any, and set by reference in the submission info array
  $has_grade = get_grade_info($assignmentid, $personid, $submission_info);

  // 4. Format the assignment type
  $submission_info['type_fmt'] = format_assignment_type($assignment_type);

  //  return the submission_info array: this array will be merged back into the assignment array  
  return $submission_info;  
}

/* Get grade info (pass submission_info array by reference to set variables */
function get_grade_info($assignmentid, $personid, &$submission_info) {
  if($submission_info['min_submission_date'] != NULL || $submission_info['max_submission_date'] != NULL) {
	  $qry = "select g.grade as pct from course_grades g where g.assignmentid = :assignmentid and g.studentid = :personid";
      // log_query($qry, array('assignmentid' => $assignmentid, 'personid' => $personid));
      $stmt = $DB->prepare($qry);
      $stmt->bindValue('assignmentid', $assignmentid, PDO::PARAM_INT);
      $stmt->bindValue('personid', $personid, PDO::PARAM_INT);
      $stmt->execute();
      // fetch a single row (need to be able to differentiate 0's from no grade at all)
      $grade_result = $stmt->fetch(PDO::FETCH_ASSOC);
      script_log(print_r($grade_result, TRUE), LEVEL_DEBUG);

      // Format the grades:
      // Get a letter grade (including for F's), if there is a grade percent
      if(is_array($grade_result) && count($grade_result) == 1 && $grade_result['pct'] >= 0) {
        $grade_letter = get_grade_letter($grade_result['pct']);
      }
      //  Put in “--” if no grade
	  // TODO: not sure if this actually works properly - ungraded assignments may always show as a 0 based on the query above
      else {
        $grade_letter = "--";
      }
  }
  else {
    $grade_letter = "--";
  }
  
  $submission_info['grade_percent'] = $grade_result['pct'];
  $submission_info['grade_letter'] = $grade_letter;
  
  if($grade_letter != "--") {
    $has_grade = TRUE;
  }
  else {
	$has_grade = FALSE;
  }
  
  return $has_grade;
}

/**
  Helper functions 
 */
 
function get_submission_date($submission_date_info, $date_type) {
  $dt = NULL;
  if($date_type == 'min') {
    $array_source = 'min_added_time';
  }
  else {
    $array_source = 'max_added_time';
  }
  
  // if there is no date to operate on, return this as NULL
  if(!is_array($submission_date_info) || $submission_date_info[$array_source] == NULL) {
    $dt = NULL;
  }
  else {
    // Create DateTime object in order to set timezone
    // Need to define as UTC first since that's what Populi stores as.
    $dt = new DateTime($submission_date_info[$array_source], new DateTimeZone('UTC'));
    // Set to settings-defined timezone.
    // Note that this needs to be changed here even though display layer requests to show in same TimeZone
    $dt->setTimeZone(new DateTimeZone(CUR_TZ));
  }
  return $dt;
}

/* Translate a grade into a letter grade */
function get_grade_letter($grade_pct) {
  global $DB;
  $grade_letter = NULL;
  // Get the grading scale
  $qry = <<<EOT
select gp.letter, gp.min_points, gp.grade_points, gp.direct_equivalent, gp.fail from grade_points gp 
where gp.deleted_by = '' and gp.letter != "P" 
order by gp.min_points desc
EOT;
  $stmt = $DB->prepare($qry);
  // script_log($qry, LEVEL_DEBUG);
  $stmt->execute();
  // fetch an array
  $results = $stmt->fetchAll(PDO::FETCH_ASSOC);
  foreach($results as $result) {
    // iterate through grade scale
    // if a match, end loop
    if($grade_pct > $result['min_points']) {
       $grade_letter = $result['letter'];
       break;
    }
	else if($grade_pct == 0) {
	  $grade_letter = 'F';
	}
  }
  return $grade_letter;
}

/* Format assignment type into a human-readable name */
function format_assignment_type($assignment_type) {
  $fmt_assignment_type = NULL;
  if(is_string($assignment_type)) {
    switch($assignment_type) {
      case 'ESSAY':
      case 'FILE_UPLOAD':
        $fmt_assignment_type = 'Assignment';
        break;
      case 'DISCUSSION':
        $fmt_assignment_type = 'Forum';
        break;
      case 'TEST':
        $fmt_assignment_type = 'Quiz/Exam';
        break;
      // Just leave empty if unrecognized type
      default:
        $fmt_assignment_type = '';
    }
  }
  return $fmt_assignment_type;
}

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

