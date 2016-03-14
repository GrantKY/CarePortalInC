// Removed the auto codes and replace with configuration screen  below
var insulin_increment = 0.5;

// adding configuration screen for the units, secret key, web URL and Pebble name
Pebble.addEventListener("showConfiguration", function(e) {
                        console.log("Showing Configuration", JSON.stringify(e));
  Pebble.openURL('http://cgminthecloud.github.io/PebbleCareportal/config_1.html');
                        });

Pebble.addEventListener("webviewclosed", function(e) {
                        var opts = JSON.parse(decodeURIComponent(e.response));
                        console.log("CLOSE CONFIG OPTIONS = " + JSON.stringify(opts));
                        // store configuration in local storage
                        localStorage.setItem('portalPebble1', JSON.stringify(opts));    
                        var transactionid = Pebble.sendAppMessage({ BG_UNITS: opts.units, INSULIN_INCREMENT: insulin_increment * 100},
                                            function(e) {
                                                         console.log('Successfully delivered message with transactionId='+ e.data.transactionId);
                                                         },
                                             function(e) {
                                                         console.log('Unable to deliver message with transactionId='+ e.data.transactionId + ' Error is: ' + e.error.message);
                                            });
                        console.log("transactionid: " + transactionid);
                        });

Pebble.addEventListener('ready',
  function(e) {
    console.log('JavaScript app ready and running!');
    var opts = [ ].slice.call(arguments).pop( );
    opts = JSON.parse(localStorage.getItem('portalPebble1'));  
    var transactionid = Pebble.sendAppMessage({ BG_UNITS: opts.units, INSULIN_INCREMENT: insulin_increment * 100},
          function(e) {
                        console.log('Successfully delivered message with transactionId='+ e.data.transactionId);
                      },
          function(e) {
                        console.log('Unable to deliver message with transactionId='+ e.data.transactionId + ' Error is: ' + e.error.message);
                      });
    console.log("transactionid: " + transactionid);
  }
);

Pebble.addEventListener('appmessage',
  function(e) {
    console.log('Treatment being sent');
    console.log(JSON.stringify(e.payload));
  
    // check for configuration data
    var message;
    //get options from configuration window

    var opts = [ ].slice.call(arguments).pop( );
    opts = JSON.parse(localStorage.getItem('portalPebble1'));

    console.log(opts);
	  // check if endpoint exists
    if (!opts.endpoint) {
        // endpoint doesn't exist, return no endpoint to watch
		// " " (space) shows these are init values, not bad or null values
        message = {
          endpoint: " ",
          pebblename: " ",
          secretAPI: " ",
          units: " ",
          hashAPI: " ",
        };
        
        console.log("NO ENDPOINT JS message", JSON.stringify(message));
        Pebble.sendAppMessage(JSON.stringify(message));
        return;   
    }
    
    var contents = MongoDBContents(e, opts.pebblename, opts.units);
    PostTreatment(contents, opts.endpoint, opts.hashAPI);
  }
);

function AddTempBasalDetails(contents, duration, percent)
{
  if(isNumber(duration))
  {
    contents.duration = parseFloat(duration);
  }
  
  if(isNumber(percent))
  {
    contents.percent = parseFloat(percent);
  }
  
  return contents;  
}

function AddBGData(contents, currentglucose, bg_units)
{
  if(isNumber(currentglucose))
  {
    contents.glucose = parseFloat(currentglucose);
    contents.units = bg_units;
    contents.glucoseType = "Finger"; 
  }
  
  return contents;  
}

function AddComboBolous(contents, enteredinsulin, splitnow, splitext)
{
  
  if(isNumber(enteredinsulin))
  {
     var upfrontinsulin = 0.0;
     if(parseFloat(splitnow) !== 0)
     {
        console.log("splitnow" + splitnow);
        upfrontinsulin = (parseFloat(splitnow) / 100) * parseFloat(enteredinsulin);   
        
     }
    
      contents.insulin = upfrontinsulin.toFixed(3);
      contents.relative = (parseFloat(enteredinsulin) - upfrontinsulin).toFixed(3);
      contents.enteredinsulin = enteredinsulin;
      contents.splitNow = splitnow;
      contents.splitExt = splitext; 
  }
  
  return contents;  
}


function isNumber(obj)
{ 
  return !isNaN(parseFloat(obj));
}

//https://ninedof.wordpress.com/2014/02/02/pebble-sdk-2-0-tutorial-6-appmessage-for-pebblekit-js/
function MongoDBContents(e, enteredBy, units)
{
    var name = e.payload.KEY_DATA; 
    var result =  e.payload.KEY_VALUE;
    var eventtype = e.payload.KEY_EVENTTYPE;
    var duration = e.payload.DURATION;
    var percent = e.payload.PERCENT;
    var glucose = e.payload.GLUCOSE;
    var insulin = e.payload.INSULIN;
    var splitnow = e.payload.SPLITNOW;
    var splitext = e.payload.SPLITEXT;
    var profile = e.payload.PROFILE;
	
    var contents = {
      "enteredBy" : enteredBy,
      "eventType" : eventtype,
    };
  
    if (name !== undefined && name !== null)
    {
        contents[name.toLowerCase()] = result;
    }  

//  Add Temp Basal Info
    if (duration !== undefined && duration !== null)
    {
      contents = AddTempBasalDetails(contents, duration, percent);
    }
  
     // Add Glucose Level info
    if (glucose !== undefined && glucose !== null)
    {
      contents = AddBGData(contents, glucose, units);
    }
    
  // Combo Bolus
  
    if(splitnow !== undefined && splitnow !== null)
    {
         contents = AddComboBolous(contents, insulin, splitnow, splitext);
    }
  
    // Profile Switch
	if(profile !== undefined && profile !== null)
    {
         contents.profile = profile;
    }
    return contents;
}


function PostTreatment(contents, endpoint, hashAPI) {
    var weburl = endpoint;
    var secret_key = hashAPI;

    console.log('Posting Treatment log');
    console.log(JSON.stringify(contents));
    var http = new XMLHttpRequest();
    http.open("POST", weburl, true);  
    
    http.setRequestHeader("API-SECRET", secret_key);    
    http.setRequestHeader("Content-type", 'application/json');
    http.setRequestHeader('Accept', 'application/json');

    http.onload = function () 
    {
        // do something to response
        console.log("http.onload - ----Status:", http.status);
  
        if ( http.status != 200)
        {
            console.log("ERROR --------------------");
            Pebble.sendAppMessage({ ERROR: "Error not able to connect to web site"});
          }
        else
        {
          console.log("SUCCESS --------------------");
            Pebble.sendAppMessage({ SUCCESS: "Message send successfully to website."});

        }
    };
  
    http.send(JSON.stringify(contents));
  }
