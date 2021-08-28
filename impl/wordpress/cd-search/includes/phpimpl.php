<?php
include 'search.php';
$s = new Search();
$r = $s->search("test", "kiddycharts", "kcnetwork", "/search", "params", 10);
$r = $s->search("test", "kiddycharts", "kcnetwork", "/suggest", "params", 10);
echo($r)
?>
