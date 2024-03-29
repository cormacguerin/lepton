<?php
include 'cds.php';

Class Search {

  public $apiKey;

  public function __construct() {
      $this->apiKey = json_decode(file_get_contents(
        "../apikey.json", true
      ), true);
  }

  function search($query, $database, $table, $endpoint, $filter, $pages) {

      $host = 'www.intradeep.com';
      $keyId = $this->apiKey['id'];
      $keyName = $this->apiKey['name'];
      $apiScope = $this->apiKey['scope'];
      $credential = $this->apiKey['credential'];
      $lang = 'en';
      $method = 'GET';

      $params = (object) [
        'query' => $query,
        'filter' => $filter,
        'database' => $database,
        'table' => $table
      ];

      $headers = (object) [
        'Content-Type' => 'application/json',
        'Accept-Charset' => 'UTF-8',
        'Date' => strtotime('now') * 1000,
        'Host' => $host,
      ];

      $headerSignature = genSignature($host,$keyId,$keyName,$apiScope,$credential,$method,$endpoint,$headers,$params);
      $headers->authorization = $headerSignature;

      $headersArr = array();
      $vars = get_object_vars( $headers );
      foreach($vars as $key=>$value) {
        $h = $key . ":" . $value;
        $headersArr[] = $h;
      }

      $url = "https://" . $host . $endpoint . "?" . http_build_query($params);
      echo($url);

      $curl = curl_init();
      curl_setopt($curl, CURLOPT_HTTPGET, 1);
      curl_setopt($curl, CURLOPT_URL, $url);
      curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1);
      curl_setopt($curl, CURLOPT_HTTPHEADER, $headersArr);
      $result = curl_exec($curl);
      curl_close($curl);
      return $result;
  } 

}
?>
