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
          $submission_info = get_assignment_submission_info($assignmentid, $assignment['type'], $personid);
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
  // pdo query (checked)
  $qry = 'SELECT personid FROM mv_students_1718 WHERE firstname = :firstname AND lastname = :lastname LIMIT 1';  
  $stmt = $DB->prepare($qry);
  log_query($qry, array('firstname' => $firstname, 'lastname' => $lastname));
  $stmt->bindValue(':firstname', $firstname, PDO::PARAM_STR);
  $stmt->bindValue(':lastname', $lastname, PDO::PARAM_STR);
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
  $qry = <<<EOT
SELECT c.termid, c.term_name AS name FROM mv_courses_1718 c 
JOIN mv_enrollments_1718 e ON c.instanceid = e.instanceid JOIN mv_students_1718 s ON s.personid = e.studentid 
WHERE s.personid = :personid 
GROUP BY c.termid 
ORDER BY c.term_start_date ASC
EOT;
  $stmt = $DB->prepare($qry);
  log_query($qry, array('personid' => $personid));
  $stmt->bindValue(':personid', $personid, PDO::PARAM_INT);
  $stmt->execute();
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
SELECT c.instanceid, c.name, FROM mv_courses_1718 c 
JOIN mv_enrollments_1718 e ON c.instanceid = e.instanceid JOIN mv_students_1718 s ON s.personid = e.studentid 
WHERE s.personid = :personid and c.termid = :termid 
GROUP BY e.instanceid 
ORDER BY c.name asc
EOT;
  $stmt = $DB->prepare($qry);
  log_query($qry, array('personid' => $personid, 'termid' => $termid));
  $stmt->bindValue(':personid', $personid, PDO::PARAM_INT);
  $stmt->bindValue(':termid', $termid, PDO::PARAM_INT);
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
  log_query($qry, array('courseid' => $courseid));
  $stmt->bindValue(':courseid', $courseid, PDO::PARAM_INT);
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
  $qry = <<<EOT
SELECT assignmentid, name, type, status, timedue FROM course_assignments 
WHERE lessonid = :lessonid AND status = 'ACTIVE' AND type NOT IN ('ATTENDANCE', 'NEITHER') 
ORDER BY orderid ASC, name ASC, timedue ASC
EOT;
  $stmt = $DB->prepare($qry);
  log_query($qry, array('lessonid' => $lessonid));
  $stmt->bindValue(':lessonid', $lessonid, PDO::PARAM_INT);
  $stmt->execute();
  // fetch array
  $results = $stmt->fetchAll(PDO::FETCH_UNIQUE);
  // array of assignments, with names, sorted by order in lesson
  return $assignments;
}

/* Step 5: For each assignment, get the submission info  */
function get_assignment_submission_info($assignmentid, $assignment_type, $personid) {
  global $DB;
  $submission_date_info = NULL;
  $grade_pct = NULL;
  // 1. get submission dates
  // if forum, get submission dates from discussion table
  if($assignment_type == 'DISCUSSION') {
    // pdo query (checked)
    $qry = <<<EOT
select f.topic_id, min(f.added_at) as min_added_time, max(f.added_at) as max_added_time from forum_topics f 
join forum_topic_comments fc on f.topic_id = fc.topic_id 
where f.assignment_id = :assignmentid and fc.added_by = :personid and fc.parent_comment_id = 0 group by f.assignment_id
EOT;
    $stmt = $DB->prepare($qry);
	log_query($qry, array('assignmentid' => $assignmentid, 'personid' => $personid));
    $stmt->bindValue(':assignmentid', $assignmentid, PDO::PARAM_INT);
    $stmt->bindValue(':personid', $personid, PDO::PARAM_INT);
    $stmt->execute();
    // fetch a single row
    $submission_date_info = $stmt->fetch(PDO::FETCH_ASSOC);
  }
  else {
    // query assignment_submissions materialized view
    $qry = <<<EOT
select min(av.added_time) as min_added_time, max(av.added_time) as max_added_time from mv_assignment_submissions_1718 av 
where av.assignment_id = :assignmentid and av.student_id = :personid and av.added_by = :personid group by av.assignment_id
EOT;
    $stmt = $DB->prepare($qry);
	log_query($qry, array('assignmentid' => $assignmentid, 'personid' => $personid));
    $stmt->bindValue(':assignmentid', $assignmentid, PDO::PARAM_INT);
    $stmt->bindValue(':personid', $personid, PDO::PARAM_INT);
    $stmt->execute();
    // fetch a single row
    $submission_date_info = $stmt->fetch(PDO::FETCH_ASSOC);
  }
  
  // 2. get the grade, if any
  $qry = "select g.grade from course_grades g where g.assignmentid = :assignmentid and g.studentid = :personid";
  log_query($qry, array('assignmentid' => $assignmentid, 'personid' => $personid));
  $stmt = $DB->prepare($qry);
  $stmt->bindValue(':assignmentid', $assignmentid, PDO::PARAM_INT);
  $stmt->bindValue(':personid', $personid, PDO::PARAM_INT);
  $stmt->execute();
  // fetch a single datum
  $grade_pct = $stmt->fetchColumn();
  
  // 3. Put in NULL if no date (use Twig to format dates - see https://twig.symfony.com/doc/1.x/filters/date.html)
  if(!is_array($submission_date_info) || $submission_date_info['max_added_time'] == NULL) {
    $submission_info['max_submission_date'] = NULL;
  }
  // 4. Format the grades
  if(is_numeric($grade_pct) && $grade_pct > 0) {
    $grade_letter = get_grade_letter($grade_pct);
  }
  //  Put in “--” if no submission, or no grade (for now, don’t differentiate between those cases)
  else {
    $grade_letter = "--";
  }
  $submission_info['grade_letter'] = $grade_letter;
  
  // 5. Format the assignment type
  $submission_info['type_fmt'] = format_assignment_type($assignment_type);

  // Build the array
  // this array will be merged back into the assignment array
  
  return $submission_info;  
}

/**
  Helper functions 
 */

/* Translate a grade into a letter grade */
function get_grade_letter($grade_pct) {
  // Some basic error checking
  if(!is_numeric($grade_pct) || $grade_pct < 0) {
    script_log($grade_pct . ' was not a number', LEVEL_ERROR);
    return NULL;
  }
  // Get the grading scale
  $qry = <<<EOT
select gp.letter, gp.min_points, gp.grade_points, gp.direct_equivalent, gp.fail from grade_points gp 
where gp.deleted_by = '' and gp.letter != "P" 
order by gp.min_points desc
EOT;
  $stmt = $DB->prepare($qry);
  script_log($qry, LEVEL_DEBUG);
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

