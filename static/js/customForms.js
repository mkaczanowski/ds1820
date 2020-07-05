$(document).on("pageinit",function() {
    var settings;

    $.ajax({
        type: "POST",
        url: "http://" + window.location.host + "/get_settings",
        data : {"type" : "all"},
        dataType : "json",
        success: function(data) {
            $("#host").val(data.host);
            $("#port").val(data.port);
            $("#recipient").val(data.email_recipient);
            $("#scale").val(parseInt(data.temperature_scale));
            $('#scale').slider('refresh');

            $("#range1a").val(data.sensors_range.min);
            $("#range1b").val(data.sensors_range.max).slider('refresh');

            $("#range1c").val(data.email_send_rate_secs).slider('refresh');
            $("#range1d").val(data.db_update_rate_secs).slider('refresh');

            settings = data;
        }
    }); 


    $("#settings").submit(function() {
        var post = {};
        settings.host = $("#host").val();
        settings.port = parseInt($("#port").val());
        settings.email_recipient = $("#recipient").val();
        settings.temperature_scale = parseInt($("#scale").val());
        settings.sensors_range.min = parseInt($("#range1a").val());
        settings.sensors_range.max = parseInt($("#range1b").val());
        settings.email_send_rate_secs = parseInt($("#range1c").val());
        settings.db_update_rate_secs = parseInt($("#range1d").val());

        $.ajax({
            type: "POST",
            url: "http://" + window.location.host + "/set_settings",
            data: {"data": JSON.stringify(settings)},
            dataType : "json",
            success: function(data)
            {
                window.location.href = "/index.html";
            }
        });

        return false; // avoid to execute the actual submit of the form.
    });
});
