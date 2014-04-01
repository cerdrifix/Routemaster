var count = 0;
var distanceRadius = 5;

String.prototype.pad = function(_char, len, to) {
		if (!this || !_char || this.length >= len) {
				return this;
		}
		to = to || 0;

		var ret = this;

		var max = (len - this.length)/_char.length + 1;
		while (--max) {
				ret = (to) ? ret + _char : _char + ret;
		}

		return ret;
};

function parseDateTime(ticks) {
	var d = new Date(ticks);
	return d.getHours()* 60 + d.getMinutes();
}

function fetchBuses(latitude, longitude) {

	var fields = [
		"StopPointName",
		"StopCode1",
		"LineName",
		"DestinationText",
		"EstimatedTime"
	];

	var response;
	var req = new XMLHttpRequest();
	req.onload = requestOnLoad;

	var url = "http://countdown.api.tfl.gov.uk/interfaces/ura/instant_V1?" +
		"Circle=" + latitude + "," + longitude + "," + distanceRadius +
		"&ReturnList="+fields.join(',');

	console.log("url: " + url);
	
	req.open('GET', url, true);
	req.send(null);
}

function requestOnLoad(e) {
	var req = this;
	console.log("request readyState: "+req.readyState);
	if (req.readyState == 4) {
		if(req.status == 200) {

			response = req.responseText.split('\n').slice(1);

			var obj = {
				"0": response.length
			};

	  		console.log("fetched " + response.length + " bus(es)");

			var index = 1;

			for(i=0; i<response.length; i++) {
				var a = JSON.parse(response[i]).slice(1),
					o = [],
					stopPointName = a[0],
					stopCode = a[1],
					lineName = a[2],
					destinationText = a[3],
					d = new Date(),
					// estimatedTime = parseDateTime(a[4] - (d.getTime() + (d.getTimezoneOffset() * 60000))),
					estimatedTime = parseDateTime(a[4] - d.getTime()),
					time = estimatedTime == 0 ? "Now!" : estimatedTime == 1 ? estimatedTime + " min" : estimatedTime + " mins";

				obj[i+100] = lineName + " - " + time;
				obj[i+200] = "to " + destinationText;
			}

			Pebble.sendAppMessage(
				obj, 
				function(e) {
					// console.log('Success sending AppMessage for transactionId:' + e.data.transactionId);
				}, 
				function(e) {
					console.log('Failed sending AppMessage for transactionId:' + e.data.transactionId + '. Error: ' + e.data.error.message);
				}
			);

		} else {
			console.log("Errore: "+req.status);
			if(req.status == 416) {
				Pebble.sendAppMessage(
					{ 
						"0": 0 
					}, 
					function() {}, 
					function(e) {
						console.log('Failed sending AppMessage for transactionId:' + e.data.transactionId + '. Error: ' + e.data.error.message);
					}
				);
			}
		}
	}
};

function locationSuccess(pos) {
	var coordinates = pos.coords;
	console.log("location success: " + coordinates.latitude + "," + coordinates.longitude);
	fetchBuses(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
	console.warn('location error (' + err.code + '): ' + err.message);
	// Pebble.sendAppMessage({
	// 	"city":"Loc Unavailable",
	// 	"temperature":"N/A"
	// });
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 }; 


Pebble.addEventListener("ready", function(e) {
	console.log("connect!" + e.ready);
	console.log("geolocation: " + JSON.stringify(window.navigator.geolocation.watchPosition));
	locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
	console.log("locationWatcher: " + JSON.stringify(locationWatcher));
});
Pebble.addEventListener("appmessage", function(e) {
	if(!!e.payload.refresh) {
		window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
		console.log("refresh!");
	}
	else {
		console.log("other message!");
	}
});

Pebble.addEventListener("webviewclosed", function(e) {
	console.log("webview closed");
	console.log(e.type);
	console.log(e.response);
});