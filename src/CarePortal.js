Pebble.addEventListener('ready',
  function(e) {
    console.log('JavaScript app ready and running!');
  }
);

Pebble.addEventListener('appmessage',
  function(e) {
    console.log('Treatement being sent');
  //  console.log(JSON.stringify(e.payload));
    
    var name = e.payload.KEY_DATA;
    var result =  e.payload.KEY_VALUE;
    
    PostTreatment(name, result);
  }
);




//https://ninedof.wordpress.com/2014/02/02/pebble-sdk-2-0-tutorial-6-appmessage-for-pebblekit-js/
function MongoDBContents(name, enteredby, value)
{
  var contents = {
    "enteredBy" : enteredby,
    "eventType" : "Note"

  };
  contents[name.toLowerCase()] = parseFloat(value);
  return contents;
}

function GetEnteredBy()
{
  return "Pebble";
}  

function GetWebURL()
{
  // TODO: NEED TO SET THE WEB SITE NAME E.G https://yourwwebsitename.azurewebsites.net//api//v1//treatments//
  return 'https://websiteaddress.azurewebsites.net//api//v1//treatments//';
}

function GetSecretKey()
{
  // TODO: Go to  http://www.sha1-online.com/ and enter your api-secret key and generate the hashed value e.g APISECRETKEY123 == a3bb4e29e4d74b0be1a2a4c360afc97a898782c5
  return "a3bb4e29e4d74b0be1a2a4c360afc97a898782c5"; 

}

function PostTreatment(name, value)
  {
    var weburl = GetWebURL();
    var secret_key = GetSecretKey();
    var enteredby = GetEnteredBy();
    var contents = MongoDBContents(name, enteredby, value);

    console.log('Posting Treatment log');
    console.log(JSON.stringify(contents));
    var http = new XMLHttpRequest();
    http.open("POST", weburl, true);  
    
    http.setRequestHeader("API-SECRET", secret_key);    
    http.setRequestHeader("Content-type", 'application/json');
    http.setRequestHeader('Accept', 'application/json');
    
   http.send(JSON.stringify(contents));
 }