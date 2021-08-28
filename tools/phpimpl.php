<?php
include 'search.php';
$s = new Search();
$r = $s->search("test", "kiddycharts", "kcnetwork", "/search", "params", 20);
$r = $s->search("test", "kiddycharts", "kcnetwork", "/suggest", "params", 20);
echo($r)
?>
