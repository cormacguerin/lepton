<?php
function getSignedHeaders($headers) {
    sort($headers);
    return implode("\n", $headers);
}

function getQueryParameters($params) {
    return http_build_query($params);
}

function getSigningKey($key_secret, $key_datestamp, $key_name, $key_scope) {
    $kDate = hmac("LT" . $key_secret, $key_datestamp, true);
    $kName = hmac($kDate, $key_name, true);
    $kScope = hmac($kName, $key_scope, true);
    $kSigning = hmac($kScope, "lt_request", true);
    return $kSigning;
}

function hmac($key, $string, $bin) {
    return hash_hmac('sha256', $string, $key, $bin);
}

function genSignature($host,$keyId,$keyName,$apiScope,$secretKey,$method,$path,$headers,$params) {

    $headersArr = [];
    $headerKeys = [];
    foreach($headers as $key=>$value) {     
        $h = $key . ":" . $value;
        $headersArr[] = strtolower($h);
        $headerKeys[] = $key;
    }

    $datestamp = strtotime('now') * 1000;
    $credential = $keyId . '/' . $datestamp . '/' . $apiScope . '/lt_request';
    $signing_string = $method . "\n" . $host . "\n" . $path . "\n" . getQueryParameters($params) . "\n" . getSignedHeaders($headersArr) . "\n";

    $signing_key = getSigningKey($secretKey, $datestamp, $keyName, $apiScope);
    $signature = hmac($signing_key, $signing_string, false);

    /*
     * Utilized for debug purpose
     *
      $signing_key_hex = bin2hex($signing_key);
      $signing_string_hex = hash('sha256', $signing_string);
      echo "signing_key_hex\n";
      echo "$signing_key_hex\n";
      echo "signing_string_hex\n";
      echo "$signing_string_hex\n";
      echo "signing_key \n";
      echo $signing_key;
      echo "signing_string \n";
      echo $signing_string;
      echo "\n";
    */

    return 'LT-HMAC-SHA256 Credential=' . $credential . ',SignedHeaders=' . strtolower(implode(";", $headerKeys)) . ',Signature=' . $signature;
}
?>
