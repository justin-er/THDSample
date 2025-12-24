/**
 * Add gobals here
 */
var seconds 	= null;
var otaTimerVar =  null;
var wifiConnectInterval = null;

/**
 * Initialize functions here.
 */
$(document).ready(function(){
	// Load critical data immediately
	getSSID();
	
	// Stagger other requests to avoid connection queuing
	// (max_open_sockets = 7, server uses 3 internally = 4 available)
	setTimeout(function() {
		getUpdateStatus();
	}, 100);
	
	setTimeout(function() {
		startDHTSensorInterval();  // This calls getDHTSensorValues() immediately
	}, 200);
	
	setTimeout(function() {
		startLocalTimeInterval();  // This calls getLocalTime() immediately
	}, 300);
	
	setTimeout(function() {
		getConnectInfo();
	}, 400);
	
	$("#connect_wifi").on("click", function(){
		checkCredentials();
	}); 
	$("#disconnect_wifi").on("click", function(){
		disconnectWifi();
	}); 
});   

/**
 * Gets file name and size for display on the web page.
 */        
function getFileInfo() 
{
    var x = document.getElementById("selected_file");
    var file = x.files[0];

    document.getElementById("file_info").innerHTML = "<h4>File: " + file.name + "<br>" + "Size: " + file.size + " bytes</h4>";
}

/**
 * Handles the firmware update.
 * Sends raw binary data instead of FormData to avoid multipart parsing issues.
 */
function updateFirmware() 
{
    var fileSelect = document.getElementById("selected_file");
    
    if (fileSelect.files && fileSelect.files.length == 1) 
	{
        var file = fileSelect.files[0];
        document.getElementById("ota_update_status").innerHTML = "Uploading " + file.name + ", Firmware Update in Progress...";

        // Read file as ArrayBuffer and send as raw binary
        var reader = new FileReader();
        reader.onload = function(e) {
            var arrayBuffer = e.target.result;
            
            // Http Request - send raw binary
            var request = new XMLHttpRequest();
            
            request.upload.addEventListener("progress", updateProgress);
            request.open('POST', "/OTAupdate");
            request.setRequestHeader("Content-Type", "application/octet-stream");
            request.responseType = "blob";
            request.send(arrayBuffer);
        };
        
        reader.onerror = function() {
            document.getElementById("ota_update_status").innerHTML = "!!! File Read Error !!!";
        };
        
        reader.readAsArrayBuffer(file);
    } 
	else 
	{
        window.alert('Select A File First')
    }
}

/**
 * Progress on transfers from the server to the client (downloads).
 */
function updateProgress(oEvent) 
{
    if (oEvent.lengthComputable) 
	{
        getUpdateStatus();
    } 
	else 
	{
        window.alert('total size is unknown')
    }
}

/**
 * Posts the firmware udpate status.
 * Changed to asynchronous to prevent blocking page load.
 */
function getUpdateStatus() 
{
    $.ajax({
        url: '/OTAstatus',
        method: 'POST',
        data: 'ota_update_status',
        dataType: 'json',
        success: function(response) {
            document.getElementById("latest_firmware").innerHTML = response.compile_date + " - " + response.compile_time

            // If flashing was complete it will return a 1, else -1
            // A return of 0 is just for information on the Latest Firmware request
            if (response.ota_update_status == 1) 
            {
                // Set the countdown timer time
                seconds = 10;
                // Start the countdown timer
                otaRebootTimer();
            } 
            else if (response.ota_update_status == -1)
            {
                document.getElementById("ota_update_status").innerHTML = "!!! Upload Error !!!";
            }
        },
        error: function() {
            // Silently fail - OTA status is not critical for page load
        }
    });
}

/**
 * Displays the reboot countdown.
 */
function otaRebootTimer() 
{	
    document.getElementById("ota_update_status").innerHTML = "OTA Firmware Update Complete. This page will close shortly, Rebooting in: " + seconds;

    if (--seconds == 0) 
	{
        clearTimeout(otaTimerVar);
        window.location.reload();
    } 
	else 
	{
        otaTimerVar = setTimeout(otaRebootTimer, 1000);
    }
}

/**
 * Gets DHT22 sensor temperature and humidity values for display on the web page.
 */
function getDHTSensorValues()
{
	$.getJSON('/dhtSensor.json', function(data) {
		$("#temperature_reading").text(data["temp"]);
		$("#humidity_reading").text(data["humidity"]);
	});
}

/**
 * Sets the interval for getting the updated DHT22 sensor values.
 * Also fetches sensor data immediately on page load.
 */
function startDHTSensorInterval()
{
	// Fetch sensor data immediately
	getDHTSensorValues();
	// Then set up interval for updates every 5 seconds
	setInterval(getDHTSensorValues, 5000);    
}

/**
 * Clears the connection status interval.
 */
function stopWifiConnectStatusInterval()
{
	if (wifiConnectInterval != null)
	{
		clearInterval(wifiConnectInterval);
		wifiConnectInterval = null;
	}
}

/**
 * Gets the WiFi connection status.
 * Changed to asynchronous to prevent blocking.
 */
function getWifiConnectStatus()
{
	$.ajax({
		url: '/wifiConnectStatus',
		method: 'POST',
		data: 'wifi_connect_status',
		dataType: 'json',
		success: function(response) {
			document.getElementById("wifi_connect_status").innerHTML = "Connecting...";
			
			if (response.wifi_connect_status == 2)
			{
				document.getElementById("wifi_connect_status").innerHTML = "<h4 class='rd'>Failed to Connect. Please check your AP credentials and compatibility</h4>";
				stopWifiConnectStatusInterval();
			}
			else if (response.wifi_connect_status == 3)
			{
				document.getElementById("wifi_connect_status").innerHTML = "<h4 class='gr'>Connection Success!</h4>";
				stopWifiConnectStatusInterval();
				getConnectInfo();
			}
		},
		error: function() {
			// Silently fail - connection status check will retry on next interval
		}
	});
}

/**
 * Starts the interval for checking the connection status.
 */
function startWifiConnectStatusInterval()
{
	wifiConnectInterval = setInterval(getWifiConnectStatus, 2800);
}

/**
 * Connect WiFi function called using the SSID and password entered into the text fields.
 */
function connectWifi()
{
	// Get the SSID and password
	selectedSSID = $("#connect_ssid").val();
	pwd = $("#connect_pass").val();
	
	$.ajax({
		url: '/wifiConnect.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		headers: {'my-connect-ssid': selectedSSID, 'my-connect-pwd': pwd},
		data: {'timestamp': Date.now()}
	});
	
	startWifiConnectStatusInterval();
}

/**
 * Checks credentials on connect_wifi button click.
 */
function checkCredentials()
{
	errorList = "";
	credsOk = true;
	
	selectedSSID = $("#connect_ssid").val();
	pwd = $("#connect_pass").val();
	
	if (selectedSSID == "")
	{
		errorList += "<h4 class='rd'>SSID cannot be empty!</h4>";
		credsOk = false;
	}
	if (pwd == "")
	{
		errorList += "<h4 class='rd'>Password cannot be empty!</h4>";
		credsOk = false;
	}
	
	if (credsOk == false)
	{
		$("#wifi_connect_credentials_errors").html(errorList);
	}
	else
	{
		$("#wifi_connect_credentials_errors").html("");
		connectWifi();    
	}
}

/**
 * Shows the WiFi password if the box is checked.
 */
function showPassword()
{
	var x = document.getElementById("connect_pass");
	if (x.type === "password")
	{
		x.type = "text";
	}
	else
	{
		x.type = "password";
	}
}

/**
 * Gets the connection information for displaying on the web page.
 */
function getConnectInfo()
{
	$.getJSON('/wifiConnectInfo.json', function(data)
	{
		$("#connected_ap_label").html("Connected to: ");
		$("#connected_ap").text(data["ap"]);
		
		$("#ip_address_label").html("IP Address: ");
		$("#wifi_connect_ip").text(data["ip"]);
		
		$("#netmask_label").html("Netmask: ");
		$("#wifi_connect_netmask").text(data["netmask"]);
		
		$("#gateway_label").html("Gateway: ");
		$("#wifi_connect_gw").text(data["gw"]);
		
		document.getElementById('disconnect_wifi').style.display = 'block';
	});
}

/**
 * Disconnects Wifi once the disconnect button is pressed and reloads the web page.
 */
function disconnectWifi()
{
	$.ajax({
		url: '/wifiDisconnect.json',
		dataType: 'json',
		method: 'DELETE',
		cache: false,
		data: { 'timestamp': Date.now() }
	});
	// Update the web page
	setTimeout("location.reload(true);", 2000);
}

/**
 * Sets the interval for displaying local time.
 * Also fetches time immediately on page load.
 */
function startLocalTimeInterval()
{
	// Fetch time immediately
	getLocalTime();
	// Then set up interval for updates every 10 seconds
	setInterval(getLocalTime, 10000);
}

/**
 * Gets the local time.
 * @note connect the ESP32 to the internet and the time will be updated.
 */
function getLocalTime()
{
	$.getJSON('/localTime.json', function(data) {
		$("#local_time").text(data["time"]);
	});
}

/**
 * Gets the ESP32's access point SSID for displaying on the web page.
 */
function getSSID()
{
	$.getJSON('/apSSID.json', function(data) {
		$("#ap_ssid").text(data["ssid"]);
	});
}







    










    


