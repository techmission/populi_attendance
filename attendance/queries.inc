<?php

/* Main business logic function: queries Populi and populates the attendance data array */
/* Delegates to two different functions based on whether a single course is requested or not */
function get_attendance_data($personid, $firstname, $lastname, $course_id = NULL) {
  $attendance_data = array();
  
  // Add personid and course_id (for debugging)
  $attendance_data['personid'] = $personid;
  $attendance_data['course_id'] = $course_id;
  
  // Add firstname and lastname (used in heading)
  $attendance_data['firstname'] = $firstname;
  $attendance_data['lastname'] = $lastname;
  
  if($course_id == NULL) {
    get_attendance_data_all_terms($attendance_data);
  }
  else {
    get_attendance_data_by_course($attendance_data, $personid, $course_id);
  }
  
  return $attendance_data;
}

/* Build the attendance data array for all terms -
   passed by reference so it can modify the array from the calling function */
function get_attendance_data_all_terms(&$attendance_data) {
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
}

/* Build the attendance data array for a single course -
   passed by reference so it can modify the array from the calling function */
function get_attendance_data_by_course(&$attendance_data, $personid, $course_id) {
  // To simplify basing template off existing template, will create a "dummy" structure that matches
  // Step 1: Get the information on the course itself, including the name
  $attendance_data['terms'][0]['courses'] = get_course_by_id($personid, $course_id);

  // Step 2: Get the lessons in the course
  $lessons = get_lessons_by_course($course_id);
  $attendance_data['terms'][0]['courses'][$course_id]['lessons'] = $lessons;  

  // Step 3: For each lesson get the assignments and add to array
  foreach($attendance_data['terms'][0]['courses'][$course_id]['lessons'] as $lessonid => $lessons) {
    $assignments = get_assignments_by_lesson($lessonid);
    // script_log(print_r($assignments, TRUE), LEVEL_DEBUG);
    // Step 4: For each assignment, get the submission info and add to array
    foreach($assignments as $assignmentid => $assignment) {
      $submission_info = get_assignment_submission_info($assignmentid, $assignment['type'], $personid);
      // add the array of assignment info and submission info together, preserving keys
      // https://stackoverflow.com/questions/3292044
      $assignments[$assignmentid] = $assignments[$assignmentid] + $submission_info;
    }
    $attendance_data['terms'][$termid]['courses'][$courseid]['lessons'][$lessonid]['assignments'] = $assignments;
  }
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

/* Step 1: Get terms in which person was enrolled (after end of add/drop) */
function get_terms_by_person($personid) {
  global $DB;
  // pdo query (checked)
  $terms = NULL;
  // exclude Summer 2018 term since it is not in the HCM submission
  $qry = <<<EOT
SELECT c.termid, c.term_name AS name FROM mv_courses_1718 c 
JOIN mv_enrollments_1718 e ON c.instanceid = e.instanceid JOIN mv_students_1718 s ON s.personid = e.studentid 
JOIN course_time_tracking ct ON ct.student_id = s.personid AND ct.course_instance_id = e.instanceid AND ct.owner_type = 'COURSE' 
WHERE s.personid = :personid 
AND c.termid != 191581
AND ct.last_update_time > c.add_drop_time 
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

/* Step 2: For each term, get the courses (after end of add/drop) */
function get_courses_by_term($personid, $termid) {
  global $DB;    
  // pdo query (checked)
  $courses = NULL;
  $qry = <<<EOT
SELECT c.instanceid, c.name FROM mv_courses_1718 c 
JOIN mv_enrollments_1718 e ON c.instanceid = e.instanceid JOIN mv_students_1718 s ON s.personid = e.studentid 
JOIN course_time_tracking ct ON ct.student_id = s.personid AND ct.course_instance_id = e.instanceid AND ct.owner_type = 'COURSE' 
WHERE s.personid = :personid and c.termid = :termid 
AND ct.last_update_time > c.add_drop_time 
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

/* Step 1 in single course version: Get the course by its id */
function get_course_by_id($personid, $course_id) {
  global $DB;    
  // pdo query (TODO: test)
  $courses = NULL;
  $qry = <<<EOT
SELECT ci.instanceid, co.name
FROM course_instances ci
JOIN course_offering_catalog_courses co ON ci.instanceid = co.course_offering_id
JOIN course_students cs ON cs.instanceid = ci.instanceid
JOIN people p ON p.personid = cs.studentid
WHERE p.personid = :personid AND ci.instanceid = :course_id
EOT;
  $stmt = $DB->prepare($qry);
  // log_query($qry, array('personid' => $personid, 'termid' => $termid));
  $stmt->bindValue('personid', $personid, PDO::PARAM_INT);
  $stmt->bindValue('course_id', $course_id, PDO::PARAM_INT);
  $stmt->execute();
  // fetch array of courses with courseids as keys, with names, sorted by name
  // TODO: see if I should only fetch first one
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
  // count only top-level forum posts that are not edits and are not deleted
  if($assignment_type == 'DISCUSSION') {
    // 1a. Do default query off forums table
    $qry = <<<EOT
select f.topic_id, min(fc.added_at) as min_added_time, max(fc.added_at) as max_added_time from forum_topics f 
join forum_topic_comments fc on f.topic_id = fc.topic_id 
where f.assignment_id = :assignmentid and fc.added_by = :personid and fc.parent_comment_id = 0 
and fc.deleted_at = '' and fc.edited_comment_id = 0
group by f.assignment_id
EOT;
    $stmt = $DB->prepare($qry);
    //log_query($qry, array('assignmentid' => $assignmentid, 'personid' => $personid));
    $stmt->bindValue('assignmentid', $assignmentid, PDO::PARAM_INT);
    $stmt->bindValue('personid', $personid, PDO::PARAM_INT);
    $stmt->execute();
    // fetch a single row
    $submission_date_info = $stmt->fetch(PDO::FETCH_ASSOC);
    // 1b. try looking in assignment submissions if that did not work
    if(!is_array($submission_date_info) || empty($submission_date_info['min_added_time'])) {
      $qry = <<<EOT
select min(a.added_time) as min_added_time, max(a.added_time) as max_added_time from assignment_submissions a
join course_assignments ca on ca.assignmentid = a.assignment_id
join course_grades g on g.assignmentid = a.assignment_id and g.studentid = a.student_id
and a.student_id = a.added_by
and a.assignment_id = :assignmentid
and a.student_id = :personid
and g.grade > 0
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
  $submission_info['min_submission_date'] = format_datetime($submission_date_info['min_added_time']);
  $submission_info['max_submission_date'] = format_datetime($submission_date_info['max_added_time']);
  
  // 3. get the grade and grade date, if any, and set by reference in the submission info array
  $has_grade = get_grade_info($assignmentid, $personid, $submission_info);

  // 4. Format the assignment type
  $submission_info['type_fmt'] = format_assignment_type($assignment_type);

  //  return the submission_info array: this array will be merged back into the assignment array  
  return $submission_info;  
}

/* Get grade info (pass submission_info array by reference to set variables */
function get_grade_info($assignmentid, $personid, &$submission_info) {
  global $DB;
  
  // Set null values for variables used later in submission info on grades
  $grade_letter = NULL; // letter grade
  $grade_date = NULL;   // date graded

  // If the assignment has a submission date, then look up grade info
  if($submission_info['min_submission_date'] != NULL || $submission_info['max_submission_date'] != NULL) {
      $qry = "select g.grade as pct, g.updated_at from course_grades g where g.assignmentid = :assignmentid and g.studentid = :personid";
      // log_query($qry, array('assignmentid' => $assignmentid, 'personid' => $personid));
      $stmt = $DB->prepare($qry);
      $stmt->bindValue('assignmentid', $assignmentid, PDO::PARAM_INT);
      $stmt->bindValue('personid', $personid, PDO::PARAM_INT);
      $stmt->execute();
      // fetch a single row (need to be able to differentiate 0's from no grade at all)
      $grade_result = $stmt->fetch(PDO::FETCH_ASSOC);
      // script_log(print_r($grade_result, TRUE), LEVEL_DEBUG);

      // Process the result:
      if(is_array($grade_result) && count($grade_result) == 1) {
        // Get a letter grade (including for F's), if there is a grade percent
        if($grade_result['pct'] >= 0) {
          $grade_letter = get_grade_letter($grade_result['pct']);
        }
        // Format the date graded
        $grade_date = format_datetime($grade_result['updated_at']);
      }
      //  Put in “--” if no grade (ungraded by professor) - for error checking on our part
      // This should only be OK on courses that are unfinalized
      else {
        $grade_letter = "--";
      }
  }
  // If no submission, indicate a 0 for that
  // Note that it appears that all assignments, even unsubmitted, get a 0 entered at the end of the term, at least in our school,
  //  so this is a presumption that non-submits are 0's
  else {
    $grade_letter = "Non-Submitted (F)";
    // In this case, grade date remains NULL - will need to handle that in template by a conditional.
  }
  
  $submission_info['grade_percent'] = $grade_result['pct'];
  $submission_info['grade_letter'] = $grade_letter;
  // PHP DateTime object
  $submission_info['grade_date'] = $grade_date;
  
  if($grade_letter != "--") {
    $has_grade = TRUE;
  }
  else {
    $has_grade = FALSE;
  }

  // Note that the calling array has been modified by reference; this return value is just a flag
  return $has_grade;
}

/**
  Helper functions 
 */

/** 
 Turns a text string in a date format into a DateTime object in the proper timezone
 This object will then be rendered in the Populi Twig template 
*/
function format_datetime($date_string) {
  $dt = NULL;
  
  // Create DateTime object in order to set timezone
  // Need to define as UTC first since that's what Populi stores as.
  $dt = new DateTime($date_string, new DateTimeZone('UTC'));
  // Set to settings-defined timezone.
  // Note that this needs to be changed here even though display layer requests to show in same TimeZone
  $dt->setTimeZone(new DateTimeZone(CUR_TZ));

  return $dt;
}

/* Translate a percent grade into a letter grade using the Populi-defined grade scale */
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
    if($grade_pct >= $result['min_points']) {
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

/* Validates whether data is in the correct format to render - stub so far */
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

/* Validates a Populi ID - stub so far, may not really be needed */
function is_populi_id($id) {
  if(is_numeric($id) && $id > 0) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}

