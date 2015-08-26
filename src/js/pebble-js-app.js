var xhrRequest = function (url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        callback(this.responseText);
    };

    xhr.open(type, url);
    xhr.send();
};

function fetchWeather(position) {
    var url =
        'https://query.yahooapis.com/v1/public/yql?q=select city, woeid from geo.placefinder where text="' +
        position.coords.latitude + ',' + position.coords.longitude + '" and gflags="R"&format=json';

    xhrRequest(url, 'GET', function(responseText) {
        var json = JSON.parse(responseText);
        var location = json.query.results.Result.city;
        var woeid = json.query.results.Result.woeid;

        fetchWeatherByWoeid(location, woeid);
    });
}

function fetchWeatherByWoeid(location, woeid) {
    var url =
        'https://query.yahooapis.com/v1/public/yql?q=select item.condition, item.forecast from weather.forecast where woeid=' +
        woeid + ' and u="c"&format=json';

    xhrRequest(url, 'GET', function(responseText) {
        var json = JSON.parse(responseText);
        var now = parseInt(json.query.results.channel[0].item.condition.code);
        var next =  parseInt(json.query.results.channel[0].item.forecast.code);
        sendToPebble(now, next);
    });
}

function locationSuccess(position) {
    fetchWeather(position);
}

function locationError(err) {
    console.log('Error requesting location: ' + err);
    sendToPebble(null, null);
}

function sendToPebble(now,next) {
    var dictionary = {};

    if (now !== null && now !== null) {
        dictionary = {
            'KEY_NOW' : now,
            'KEY_NEXT': next
        };
    }

    Pebble.sendAppMessage(dictionary,
        function(e) {
            console.log('Info successfully sent to Pebble');
        },
        function(e) {
            console.log('Error sending info to Pebble!');
        }
    );
}

Pebble.addEventListener('ready', function(e) {
    getWeather();
});

Pebble.addEventListener('appmessage', function(e) {
    getWeather();
});

function getWeather() {
    navigator.geolocation.getCurrentPosition(
        locationSuccess,
        locationError,
        {timeout: 15000, maximumAge: 60000}
    );
}
