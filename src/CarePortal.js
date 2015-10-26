Pebble.addEventListener('ready',
  function(e) {
    console.log('JavaScript app ready and running!');
  }
);

Pebble.addEventListener('appmessage',
  function(e) {
    console.log('Treatement being sent');
    console.log(JSON.stringify(e.payload));
    
    var name = e.payload.KEY_DATA;
    var result =  e.payload.KEY_VALUE;
    var eventtype = e.payload.KEY_EVENTTYPE;
 
    PostTreatment(name, eventtype, result);
  }
);


function isNumber(obj)
{ 
  return !isNaN(parseFloat(obj));
}

//https://ninedof.wordpress.com/2014/02/02/pebble-sdk-2-0-tutorial-6-appmessage-for-pebblekit-js/
function MongoDBContents(name, enteredby, eventtype,  value)
{
  var contents = {
    "enteredBy" : enteredby,
    "eventType" : eventtype,

  };

  if(isNumber(value))
  {
    contents[name.toLowerCase()] = parseFloat(value);
  }
  else
    contents[name.toLowerCase()] = value;
  return contents;
}

function GetEnteredBy()
{
  return "Pebble";
}  

function GetWebURL()
{
  // TODO: NEED TO SET THE WEB SITE NAME E.G https://yourwwebsitename.azurewebsites.net//api//v1//treatments//
  return 'https://yourwebsite.azurewebsites.net//api//v1//treatments//';
}

function GetSecretKey()
{
  // TODO: Go to  http://www.sha1-online.com/ and enter your api-secret key and generate the hashed value e.g APISECRETKEY123 == a3bb4e29e4d74b0be1a2a4c360afc97a898782c5
  return "ad8b6f4c9ec736c45081ec96bd056e34dc2b509e"; 

}

function PostTreatment(name, eventtype, value)
  {
    var weburl = GetWebURL();
    var secret_key = GetSecretKey();
    var enteredby = GetEnteredBy();
    var contents = MongoDBContents(name, enteredby, eventtype, value);

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
            Pebble.sendAppMessage({ SUCCESS: "Message send successfully to website."});

        }
    };
  
    http.send(JSON.stringify(contents));
  }