Explanation
===========
Queries a local copy of the Populi database to render a record of the 
student's attendance in a given set of terms.

Creator
=======
Evan Donovan, for TechMission / City Vision University

License
=======
GNU GPL v2

Installation
============

1. Create a mirror of the Populi database & then create the supporting views (todo: document).
2. Configure the GUID you wish to use in the api_settings.inc file that you copy from api_settings.inc.dist. "SELECT UUID();" in MySQL.
3. Configure your database connection from connection.inc.
4. Run "composer install" when in the attendance directory to install the proper version of Twig.

Usage
=====

Will fetch an HTML page showing a student's attendance from a Populi database.

Example:

`wkhtmltopdf http://www.example.com/populi_attendance/attendance/activity_report.php?firstname=firstname&lastname=&key={GUID} 8-LNAME,FNAME.pdf`

(This requires wkhtmltopdf to be installed on Linux.)

Testing
=======
Use https://gist.github.com/k3n/1846220 to test code is correct.
