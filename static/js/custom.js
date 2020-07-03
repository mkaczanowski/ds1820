var refresh_rate_secs = 5;

$(document).on("pageinit",function() {
    reload();
    setInterval(reload, 1000 * refresh_rate_secs);
});

function reload() { 
	var settings = getSettings();
	settings.sensors.forEach(function(item, index) {
		getSensorInfo(settings, item, 5);
	});
}

function normalizeSensorName(name) {
	return name.split("/")[5].replace("-", "_");
}

function getSensorInfo(settings, sensor, limit){
    var source_sensor = sensor;
    var normalized_sensor = normalizeSensorName(source_sensor);

    $.ajax({
        type: "POST",
        url: "http://localhost:8888/get_temp",
        data : {"count" : limit, "sensor": source_sensor},
        dataType : "json",
        success: function(data) {
		var history = [];
		var elem = `
			<li id="li_{sensor}" data-cards-type='weather'>
				<h1>Sensor: {sensor}</h1>
				<ul class='detail'>
					<li><i id='sensor_{sensor}' class='icon-circle {icon}'></i>
						<span id='status_sensor_{sensor}'>{sensor_status}</span>
					</li>
					<li id='sensor_{sensor}_temp'>{sensor_temp}&deg;</li>
				</ul>
				<ul class='week' id='sensor_{sensor}_history'>
					{sensor_data}
				</ul>
			</li>`;

		var online = (Math.round(+new Date()/1000) - data[0].timestamp) < settings.db_update_rate_secs + refresh_rate_secs;

            	$.each(data, function(i){
            	     if(i == 0) return;
            	     var date = new Date(data[i].timestamp*1000);
            	     history.push("<li><span>"+date.getHours()+":"+date.getMinutes()+"</span><strong>"+data[i].temperature+"&deg;</strong></li>");
	    	});

		var rendered = elem
		    .replace(/\{sensor\}/g, normalized_sensor)
		    .replace(/\{sensor_temp\}/g, data[0].temperature)
		    .replace(/\{sensor_data\}/g, history.join(" "))
		    .replace(/\{icon\}/g, online ? "sunny" : "cold")
		    .replace(/\{sensor_status\}/g, "STATUS: " + (online ? "ON" : "OFF"));

		if ($("#li_" + normalized_sensor).length == 1) {
			$('#content ul #li_' + normalized_sensor).html(rendered);
		} else {
			$("#content ul").append(rendered);
		}
        }
    }); 
}

function getSettings(){
    var json_data;

    $.ajax({
        type: "POST",
        url: "http://localhost:8888/get_settings",
        data : {"t": "test"},
        dataType : "json",
	async: false,
        success: function(data) {   
		json_data = data;
        }
    }); 

    return json_data;
}
