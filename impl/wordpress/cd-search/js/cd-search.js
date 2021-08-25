(function($) {
    'use strict';
    $(window).on('load', function() {
        // console.log('custom-ajax-search : load');
        if (typeof(cdData) != 'undefined') {
            $(cdData.cssInputSelectors).autocomplete({
                appendTo: $(event.target).closest("form"),
                minLength: 3,
                source: customSearchSource,
                select: searchSelectedItem
            });
            function searchSelectedItem(event, ui) {
                // Copy the slected item into the search text box.
                $(event.target).val(ui.item.label);
                // Find the clostest (parent) form and submit it.
                $(event.target).closest("form").submit();
            }
            function customSearchSource(request, response) {
                // console.log('search: ' + request.term);
                // Send the HTTP POST data to our server, with the action and
                // search term (searchQuery).
                $.post(cdData.ajaxUrl, {
                        action: cdData.ajaxAction,
                        searchQuery: request.term
                    },
                    function(data, status) {
                        var responseData = null;
                        if (data) {
                            // console.log('good response: ' + data);
                            var responseData = JSON.parse(data);
                        } else {
                            // console.log('bad response: ' + data);
                        }
                        if (Array.isArray(responseData)) {
                            response(responseData);
                        } else {
                            response([]);
                        }
                    }
                );
            }
        }
    });
})(jQuery);
