<?php
/**
 * CD Search
 *
 * @package     CD_Search
 * @author      Plugin Contributors
 * @copyright   2021 Plugin Contributors
 * @license     GPL-2.0-or-later
 *
 * @wordpress-plugin
 * Plugin Name: CD Search
 * Description: Search plugin by compdeep.com & giga.store
 * Plugin URI: https://www.compdeep.com/
 * Version: 0.1
 * Author: Compdeep
 * Author URI: https://www.linkedin.com/in/cormacguerin
 * License: GPLv2 or later
 * License URI: https://www.gnu.org/licenses/gpl-2.0.html
 * Text Domain: cd-search
 *
 */

if(!defined('ABSPATH')) {
	die; // Die if accessed directly.
}

include(plugin_dir_path( __FILE__ ) . 'includes/search.php');

const CD_ACTION_SEARCH = 'cd_search';
const CD_DEFAULT_CSS_SELECTOR = 'form[role="search"] input[type="search"]';
const CD_MAX_SEARCH_RESULT_COUNT = 5;

class CD_Search {

	private static $instance = null;

    private function __construct() {
	
    }

    function cd_enqueue_scripts() {
        $base_url = '/wp-content/plugins/' . pathinfo(__FILE__, PATHINFO_FILENAME) . '/';
        $theme_version = wp_get_theme()->get('Version');
        wp_enqueue_style('cd-frontend', $base_url . 'css/cd-search.css', null, $theme_version);
        wp_enqueue_script('cd-frontend',
           $base_url . 'js/cd-search.js',
           array('jquery-ui-autocomplete'),
           $theme_version,
           true
        );
        wp_localize_script(
           'cd-frontend',
           'cdData', array(
              'cssInputSelectors' => CD_DEFAULT_CSS_SELECTOR,
              'ajaxAction' => CD_ACTION_SEARCH,
              'ajaxUrl' => admin_url('admin-ajax.php'),
           )
        );
    }

	function cd_search() {
		if (array_key_exists('requestType', $_POST)) {
			if ($_POST['requestType'] == 'suggest') {
				$s = new Search();
				$search_query = wp_strip_all_tags($_POST['searchQuery']);
				$r = array();
				$r = $s->search($search_query, "kiddycharts", "kcnetwork", "/suggest", "params", 10);
				print json_encode(json_decode($r)->{'suggestions'});
			} else if ($_POST['requestType'] == 'search') {
				$s = new Search();
				$search_query = wp_strip_all_tags($_POST['searchQuery']);
				$r = array();
				$r = $s->search($search_query, "kiddycharts", "kcnetwork", "/search", "params", 20);
				$items = json_decode($r)->{'items'};
				foreach($items as $item) {
					print '<div class="cd-result-card">';
					print '<div class="cd-result-title">';
					print '<a href="'.$item->{'url'}.'">';
					print $item->{'title'};
					print '</a></div>';
					print '<div class="cd-result-snippet">';
					print $item->{'snippet'};
					print '</div>';
					print '<a class="cd-result-url" href="'.$item->{'url'}.'">';
					print $item->{'url'};
					print '</a></div>';
				}
			}
			wp_die();
		}
	}

	function cd_init() {
        add_action('wp_ajax_nopriv_' . CD_ACTION_SEARCH, 'cd_search');
        add_action('wp_ajax_' . CD_ACTION_SEARCH, 'cd_search');
    }

	public static function getInstance() {

		if (self::$instance == null) {
			self::$instance = new CD_Search();
		}

		if (!empty($_POST)) {
			if (array_key_exists('doing_form',$_POST)) {
				self::$instance->cd_search();
			}
		}

        add_action('wp_ajax_nopriv_' . CD_ACTION_SEARCH, [self::$instance,'cd_search']);
        add_action('wp_ajax_' . CD_ACTION_SEARCH, [self::$instance,'cd_search']);
        add_action('wp_enqueue_scripts', [self::$instance,'cd_enqueue_scripts']);
		return self::$instance;
    }

}

CD_Search::getInstance();

