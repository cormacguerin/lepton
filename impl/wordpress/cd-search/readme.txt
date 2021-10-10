==== cd-search ====
Contributors: cormac guerin
Tags: compdeep, giga.store, search, suggest, wordpress, ecommerce
Requires at least: 4.3
Tested up to: 5.7
Requires PHP: 5.6
Stable tag: trunk

Enable compdeep search API

== Description ==

This is a wordpress implementation for search and suggest API at compdeep.com

To use update your api key and scope in apikey.json

For more history, see [here](https://compdeep.com/search).

== Installation ==
Copy files into plugin directory and enable plugin.

You then need to add the searchbox[form] into the theme at the required position.

<div class="cd-search-container">
	<form method="post" class="cd-search">
	  <input id="cd-search" type="text" name="doing_form" value="" class="cd-search" target="_blank" action="cd-search.php"/>
	</form>
	<div id="cd-results" ></div>
</div>

= Actions & Filters =

== Screenshots ==

== Get Involved ==

contact cormac@compdeep.com

== Changelog ==

See the [release history](https://github.com/wordpress/two-factor/releases).
