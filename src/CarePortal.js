///////////////////////////////////DATA TO MANUALLY SET BEFORE BUILDING ///////////////////////
function GetEnteredBy()
{
  return "Pebble";
}  

function GetWebURL()
{
  // TODO: NEED TO SET THE WEB SITE NAME E.G https://yourwwebsitename.azurewebsites.net//api//v1//treatments//
  return 'https://yourwebsitename.azurewebsites.net//api//v1//treatments//';
}

function GetSecretKey()
{
  // TODO: Go to  http://www.sha1-online.com/ and enter your api-secret key and generate the hashed value e.g APISECRETKEY123 == a3bb4e29e4d74b0be1a2a4c360afc97a898782c5
  return "a3bb4e29e4d74b0be1a2a4c360afc97a898782c5"; 
}
function Units()
{
    return "mmols";
    //return "mg/dl";
}
///////////////////////////////////END DATA TO MANUALLY SET BEFORE BUILDING ///////////////////////


Pebble.addEventListener('ready',
  function(e) {
    console.log('JavaScript app ready and running!');
    Pebble.sendAppMessage({ BG_UNITS: Units()});
  }
);

Pebble.addEventListener('appmessage',
  function(e) {
    console.log('Treatment being sent');
    console.log(JSON.stringify(e.payload));
  
    var contents = MongoDBContents( e);
    PostTreatment(contents);
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


function isNumber(obj)
{ 
  return !isNaN(parseFloat(obj));
}

//https://ninedof.wordpress.com/2014/02/02/pebble-sdk-2-0-tutorial-6-appmessage-for-pebblekit-js/
function MongoDBContents(e)
{
    var name = e.payload.KEY_DATA; 
    var result =  e.payload.KEY_VALUE;
    var eventtype = e.payload.KEY_EVENTTYPE;
    var enteredby = GetEnteredBy();
    var duration = e.payload.DURATION;
    var percent = e.payload.PERCENT;
    var glucose = e.payload.GLUCOSE;
  
    var contents = {
      "enteredBy" : enteredby,
      "eventType" : eventtype,
    };
  
    if (name !== undefined && name !== null)
    {
//       if(isNumber(result))
//       {
//         contents[name.toLowerCase()] = parseFloat(result);
//       }
//       else
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
      contents = AddBGData(contents, glucose, Units());
    }
  
    return contents;
}


function PostTreatment(contents)
  {
    var weburl = GetWebURL();
    var secret_key = GetSecretKey();

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