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
 * Author: Cormac Guerin
 * Author URI: https://www.linkedin.com/in/cormacguerin
 * License: GPLv2 or later
 * License URI: https://www.gnu.org/licenses/gpl-2.0.html
 * Text Domain: cd-search
 *
 */

include 'includes/search.php';

const CD_ACTION_SEARCH = 'cd_search';
const CD_DEFAULt_CSS_SELECTOR = 'form[role="search"] input[type="search"]';
const CD_MAX_SEARCH_RESULT_COUNT = 5;

class CD_Search {

    function __construct() {
        add_action( 'init', [ $this, 'cd_init' ] );
        add_action( 'wp_enqueue_scripts', [ $this, 'cp_enqueue_scripts' ] );
    }

    function cd_init() {
        add_action('wp_ajax_nopriv_' . CD_ACTION_SEARCH, 'cd_search');
        add_action('wp_ajax_' . CD_ACTION_SEARCH, 'cd_search');
    }

    function cd_enqueue_scripts() {
        $base_url = get_stylesheet_directory_uri() . '/' . pathinfo(__FILE__, PATHINFO_FILENAME) . '/';
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
              'cssInputSelectors' => CD_DEFAULt_CSS_SELECTOR,
              'ajaxAction' => CD_ACTION_SEARCH,
              'ajaxUrl' => admin_url('admin-ajax.php'),
           )
        );
    }

    function cd_search() { 
        $s = new Search();
        $search_query = wp_strip_all_tags($_POST['searchQuery']);
        $r = array();
        // $r = $s->search($_POST['searchQuery'], "kiddycharts", "kcnetwork", "/search", "params", 20);
        $r = $s->search($search_query, "kiddycharts", "kcnetwork", "/suggest", "params", 10);
        echo json_encode($r);
        wp_die();
    }
}

if(class_exists('CD_Search')) {
    // instantiate the class.
    $search_plugin = new CD_Search();
}
