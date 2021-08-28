(function($) {
    'use strict';
    $(window).on('load', function() {
		$('.cd-search').autocomplete({
			appendTo: $(event.target).closest("form"),
			minLength: 3,
			source: customSearchSource,
			select: searchSelectedItem
		});
		$('.cd-search').keypress(function(event){
			if(event.keyCode == 13) {
				event.preventDefault();
				/*
				 * TODO clean this
				 */
				var ui = {}
				ui.item = {}
				ui.item.label = this.value
				console.log(this.value)
				if (this.value) {
					searchSelectedItem(event, ui)
				}
			}
		});
		function searchSelectedItem(event, ui) {
			$.post(cdData.ajaxUrl, {
					action: cdData.ajaxAction,
					requestType: 'search',
					searchQuery: ui.item.label
				},
				function(data, status) {
					var responseData = null;
					if (data) {
						console.log(data);
						var results = document.getElementById('cd-results');
						results.innerHTML = data;
						console.log(results.innerHTML);
					}
				}
			);
		}
		function customSearchSource(request, response) {
			let query = request.term.trim()
			if (query) {
				$.post(cdData.ajaxUrl, {
						action: cdData.ajaxAction,
						requestType: 'suggest',
						searchQuery: request.term
						// searchQuery: request.term.replace(/ /g, ":")
					},
					function(data, status) {
						var responseData = null;
						if (data) {
							console.log(data)
							var responseData = JSON.parse(data);
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
