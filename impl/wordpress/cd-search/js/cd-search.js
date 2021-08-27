(function($) {
    'use strict';
    $(window).on('load', function() {
		$('.cd-search').autocomplete({
			appendTo: $(event.target).closest("form"),
			minLength: 3,
			source: customSearchSource,
			select: searchSelectedItem
		});
		function searchSelectedItem(event, ui) {
			// $(event.target).val(ui.item.label);
			$.post(cdData.ajaxUrl, {
					action: cdData.ajaxAction,
					requestType: 'search',
					searchQuery: ui.item.label
				},
				function(data, status) {
					var responseData = null;
					if (data) {
						console.log('good response: ' + data);
						// var responseData = JSON.parse(data);
						var results = document.getElementById('cd-results');
						results.innerHTML = data;
						console.log(results.innerHTML);
					} else {
						// console.log('bad response: ' + data);
					}
					if (Array.isArray(responseData)) {
						// response(responseData);
					}
				}
			);
		}
		function customSearchSource(request, response) {
			// console.log('search: ' + request.term);
			// Send the HTTP POST data to our server, with the action and
			// search term (searchQuery).
			let query = request.term.trim()
			if (query) {
				$.post(cdData.ajaxUrl, {
						action: cdData.ajaxAction,
						requestType: 'suggest',
						searchQuery: request.term
					},
					function(data, status) {
						var responseData = null;
						if (data) {
							// console.log('good response: ' + data);
							console.log(data)
							var responseData = JSON.parse(data);
						} else {
							// console.log('bad response: ' + data);
						}
						if (Array.isArray(responseData)) {
							response(responseData.map(item => item.replace(/:/g,' ')));
						} else {
							response([]);
						}
					}
				);
			}
		}
    });
})(jQuery);
