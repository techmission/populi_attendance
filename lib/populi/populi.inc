<?php
/* Base Populi API class - based on the example at 
   https://support.populiweb.com/hc/en-us/articles/223798767 */
class PopuliCore {
  //Set to the correct URL for your college
  protected $api_url = 'https://' . API_SITE . '.populiweb.com/api/index.php'; 
  
  // Access token needed to perform API tasks - if NULL, must call login() before calling doTask()
  private $api_token = NULL;
  
  /* Function to login to Populi (must be called first unless access token is available */
  public function login($username = NULL, $password = NULL) {
    // Use the defaults from the settings file if not passed in
	if($username == NULL || $password == NULL) {
	  $username = API_USERNAME;
	  $password = API_PASSWORD;
	}
    $params = 'username=' . urlencode($username) . '&password=' . urlencode($password);
    
    // Place the results into an XML string. Use cURL since file_get_contents is unreliable.
    $curl = curl_init();
    curl_setopt($curl, CURLOPT_POST, 1);
    curl_setopt($curl, CURLOPT_POSTFIELDS, $params);
    curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1);
    curl_setopt($curl, CURLOPT_URL, $this->api_url);
    $response = curl_exec($curl);
    curl_close($curl);
    
    if( $response !== false) {
      // Use SimpleXML to put results into Simple XML object (requires PHP5)
      $xml = new SimpleXMLElement($response);
      if(isset($xml->access_key)){
        $this->api_token = (string)$xml->access_key;
      }
      else{
        throw new PopuliException("Valid username/password required.", 'AUTHENTICATION_ERROR');
      }
    }
    else{
      throw new PopuliException("Could not connect to Populi via API.", 'CONNECTION_ERROR');
    }
  }
  
  /* Unsets the API token to close the connection on the script side. */
  public function logout(){
    $this->api_token = NULL;
  }
  
  // Parse the response into a SimpleXML object by default, and return.
  // For certain tasks, though, (like downloadFile), the raw response is needed. 
  // In that case, set return_raw to true.
  public function doTask( $task, $params = array(), $return_raw = false ){
    if( !$this->api_token ){
      throw new Exception("Token not available - call login before trying to perform a task.");
    }
    
    $post = 'task=' . urlencode($task) . '&access_key=' . $this->api_token;
    
    foreach($params as $param => $value){
      if(is_array($value) ){
        foreach($value as $array_value){
          $post .= '&' . $param . '[]=' . urlencode($array_value);
        }
      }
      else{
        $post .= "&$param=" . urlencode($value);
      }
    }
    
    // Place the results into an XML string. Use cURL since file_get_contents is unreliable.
    $curl = curl_init();
    curl_setopt($curl, CURLOPT_POST, 1);
    curl_setopt($curl, CURLOPT_POSTFIELDS, $post);
    curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1);
    curl_setopt($curl, CURLOPT_URL, $this->api_url);
    $response = curl_exec($curl);
    curl_close($curl);
    
    if( $curl !== false ){
      if( $return_raw ){
        return $response;
      }
      else{
        // Use Simple XML to put results into Simple XML object (requires PHP5)
        try{
          $xml = new SimpleXMLElement($response);
        }
        catch(Exception $e){
          echo htmlentities($response) . '<br><br><br>';
          throw new PopuliException('Problem parsing the XML response: ' . $e->getMessage(), 'PARSE_ERROR');
        }
        
        if( $xml->getName() == 'response' ){
          return $xml;
        }
        else if( $xml->getName() == 'error' ){
          throw new PopuliException((string)$xml->message, (string)$xml->code);
        }
        else{
          // Received invalid XML document - response or error should always be the root element
          throw new PopuliException('Problem parsing the XML response: invalid root element.', 'PARSE_ERROR');
        }
      }
    }
    else{
      throw new PopuliException('Could not connect to Populi.', 'CONNECTION_ERROR');
    }
  }
}
	    
class PopuliException extends Exception{
  /**************************************************************************************************
   *  We have our own variable since we don't feel like using numeric error codes
   *  Should be one of:
   *    AUTHENTICATION_ERROR - Couldn't login to the API (bad username/password)
   *    BAD_PARAMETER - You called a task using parameters it didn't like
   *    CONNECTION_ERROR - Thrown if we can't connect to Populi 
   *    PARSE_ERROR - The XML document returned from Populi could not be parsed
   *    LOCKED_OUT - Your user account is blocked (too many failed login attempts?)
   *    OTHER_ERROR - Default generic error
   *    PERMISSIONS_ERROR - You aren't allowed to call that task with those parameters
   *    UNKNOWN_TASK - You tried to call an API task that doesn't exist
  ****************************************************************************************************/
  public $populi_code = null;
  
  public function __construct($message, $populi_code = 'OTHER_ERROR'){
    parent::__construct($message);
    $this->populi_code = $populi_code;
  }
  
  public function getPopuliCode(){
    return $this->populi_code;
  }
}
