//------------------------------------------------------------------------------------------
//die haupseite, sie wird im flash abgelegt (wegen PROGMEM) R" heißt "roh", rawliteral( als trennzeichen
const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>           
<html lang='de'>
  <head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width'>
    <title>Webserver RFID</title>
    <style>
     form { 
       border: 1px solid black; 
       padding: 1em;
      }
      div {
        border: 1px solid red;
        padding: 1em;
      }
      ul {
        border: 1px solid blue;
        padding: 1em;
        list-style-type: none;
      }
      h1 {
        font-size: 125%%; /* im Template keine Prozentangababen moeglich :-), leitet template ein..., %% - yes */
      }
      h2 {
        font-size: 110%%;
      }
    </style>
    <script>
      var gateway = `ws://${window.location.hostname}/ws`;
      var websocket;
      function htmlToElement(html) 
      {
        var template = document.createElement('template');
        html = html.trim(); // Never return a text node of whitespace as the result
        template.innerHTML = html;
        return template.content.firstChild;
      }

      
      function addClicked(e)
      {
        let li = e.currentTarget.parentNode; //current: listener is attached, also button
        let okList = document.getElementById("iULOk");
        let anzahl = okList.querySelectorAll("li").length;
        
        //if (anzahl < %RFID_MAX%) /* geht das mit Templates?, ja - nett */
        { //nur wenn es passt, obwohl auch der server die pruefung vornimmt - rueckmeldung
            let inputs = Array.from(li.querySelectorAll("input"));
            let rfid = inputs[0].value;
            let owner = inputs[1].value;
            let sendObject = {"action":"addAuthorized","rfid":rfid,"owner":owner};
            websocket.send(JSON.stringify(sendObject));
        }
      }
      function removeClicked(e)
      {
        let li = e.currentTarget.parentNode;
        let newList = document.getElementById("iULNew");
        let anzahl = newList.querySelectorAll("li").length; 
        let inputs = Array.from(li.querySelectorAll("input"));
        let rfid = inputs[0].value;
        let owner = inputs[1].value;
        let sendObject = {"action":"removeAuthorized","rfid":rfid,"owner":owner};
        websocket.send(JSON.stringify(sendObject));
     }


      function initWebSocket()
      { 
         websocket = new WebSocket(gateway);
         websocket.onopen = () =>
         {  
            websocket.send(JSON.stringify({'action':'getRfidsNew'}));
            websocket.send(JSON.stringify({'action':'getRfidsOk'}));
            websocket.send(JSON.stringify({'action':'keepWebServerAlive','time':0}));

            console.log('Connection opened');
            
         } 
         websocket.onclose = () => 
         {
            console.log('Connection closed');
            setTimeout(initWebSocket, 2000); //wenn stress versuche reconnect - ich glaube das funktioniert nicht sauber
         }
         websocket.onmessage = (e) =>
         {
            console.log(`Received a notification from ${e.origin}`);
            console.log(e);
            console.log(e.data);
            let data =JSON.parse(e.data);
            console.log(data);
            let newList = document.getElementById("iULNew");
            let okList = document.getElementById("iULOk");
            let newNode;
            let remNode;
            switch (data.action)
            {
              case "message":
                document.getElementById("iMessage").innerHTML = data.text;
                break;
              case "addNew":
                if (data.removeFirst)
                {
                  newList.firstElementChild.remove();  
                }
                //let pos = newList.children.length;
                newNode = htmlToElement("<li><button type='button'>add</button> " + 
                      "<input type='text' value='" +data.rfid+"' readonly>"+
                      "<input type='text' value='" +data.owner+"'></li>");
                newList.append(newNode);
                newNode.children[0].addEventListener("click",addClicked);//der button                                 
                /*
                newList.innerHTML +=  "<li><input type='checkbox' name='cb"+data.name+pos+"[]'>" + 
                      "<input type='text' name='rfid"+data.name+pos+"[]' value='" +data.rfid+"' readonly>"+
                      "<input type='text' name='owner"+data.name+pos +"[]' value='" +data.owner+"'></li>\n";            
                */
                break;
              case "removeNew":
                remNode = newList.querySelector("input[value='" +data.rfid +"']");
                if (remNode)
                  remNode.parentElement.remove();
                break;
              case  "addAuthorized":
                //zur authorized (okList) hinzu aus der newList entfernen
                remNode = newList.querySelector("input[value='" +data.rfid +"']");
                if (remNode)
                  remNode.parentElement.remove();
                
                newNode = htmlToElement("<li><button type='button'>remove</button> " + 
                      "<input type='text' value='" +data.rfid+"' readonly>"+
                      "<input type='text' value='" +data.owner+"'></li>");
                okList.append(newNode);
                newNode.children[0].addEventListener("click",removeClicked);//der button   
                break;
              case "removeAuthorized":
                remNode = okList.querySelector("input[value='" +data.rfid +"']");
                if (remNode)
                  remNode.parentElement.remove();
                break;
              case "clearNew":
                while( newList.firstChild ){
                 //newList.removeChild( newList.firstChild );
                 newList.firstChild.remove();
                }
                break;
              case "newOkList": //am Anfang und zum Testen
                while( okList.firstChild )
                {
                 //newList.removeChild( newList.firstChild );
                 okList.firstChild.remove();
                }
                data.data.forEach( entry => 
                {
                  newNode = htmlToElement("<li><button type='button'>remove</button> " + 
                      "<input type='text' value='" +entry.rfid+"' readonly>"+
                      "<input type='text' value='" +entry.owner+"'></li>");
                  
                  newNode.children[0].addEventListener("click",removeClicked);//der button                                 
                  okList.append(newNode);
                });
                break;
              case "newNewList": //am Anfang
                while( newList.firstChild )
                {
                 newList.removeChild( newList.firstChild );
                }
                data.data.forEach( entry => 
                {
                  newNode = htmlToElement("<li><button type='button'>add</button> " + 
                      "<input type='text' value='" +entry.rfid+"' readonly>"+
                      "<input type='text' value='" +entry.owner+"'></li>");
                  
                  newNode.children[0].addEventListener("click",addClicked);//der button                                 
                  newList.append(newNode);
                });
                break;
              default:
                console.log("unknown action");
                break;
            }  
         }
      }
      
      window.addEventListener("load", () => 
      {
        initWebSocket();
        document.getElementById('bTestEintrag').addEventListener("click",() => 
        {
          let rfid = document.getElementById('testEintrag').value; 
          let owner = document.getElementById('testOwner').value;
          //websocket.send(rfid+'|'+owner);
          websocket.send(JSON.stringify(
          {
              'action':'addNew',
              'rfid':rfid,
              'owner':owner
          }));
        });
        document.getElementById('bClearNew').addEventListener("click",() => 
        {
          websocket.send(JSON.stringify(
          {
              'action':'clearNew'
          }));
        });
        document.getElementById('bWiFiStop').addEventListener("click",() =>
        {
           websocket.send(JSON.stringify({'action':'keepWebServerAlive','time':3000}));//in drei Sekunden
        });
        /*heraus genommen 
        document.getElementById('bSaveToServer').addEventListener("click",() =>
        {
            let okList = document.getElementById("iULOk");
            let inputs = Array.from(okList.querySelectorAll("li > input"));//direkter nachfahre des li
            let objectArray = [];
            for (let i = 0; i < inputs.length ; i+=2)
            {
              let rfid = inputs[i].value;
              let owner = inputs[i+1].value;
              objectArray.push({"rfid":rfid,"owner":owner});
            }
            let sendObject = {
                "action":"addAuthorizedAll",
                "rfidArray":objectArray };
                
            websocket.send(JSON.stringify(sendObject));     
            console.log(objectArray);
        });
        */
      }); 
    </script>
  </head> 
  <body style='font-family:Helvetica, sans-serif'> 
    <h1>Webserver for RFID config <button type="button" id="bWiFiStop">Stop WiFi, save energy</button></h1>
    <p>Eintrag zum Test (nicht sinnvoll im normalen Betrieb):</p>
    <form> 
      <input id='testEintrag' placeholder='RFIDid' type='text'>
      <input id='testOwner' placeholder='Owner' type='text'>
      <button type='button' id='bTestEintrag'>go</button>
    </form>
    <div id="iMessage">
    </div>
    <form> <!-- method='post' action='/' name='rfid'> wollte mal mit post uebertragen, lasse das -->
    <p>Um neue RFIDs zu lesen bitte vor dem Sensor mit dem Teil wedeln und dann den Besitzer einzutragen, bevor weitere Aktionen erfolgen.
    <br>
    Mehr als 8 RFIDs pro Liste sind nicht möglich, kommt einer mehr, so wird der erste gekickt.</p>
    <h2>gelesene RFIDs <button id="bClearNew" type="button">Clear (ESP)</button> </h2>
    <p>Beachte: Wenn man in der Liste klickt, dann wird das RFID-Tag in die Liste der authorisierten aufgenommen. 
    Das erste Feld ist die gelesene RFID, das zweite ist der einzutragende Besitzer, diesen Einzutragen ist sinnvoll. </p>
    <p>Die Liste ist nicht perstistent, bei einem Neustart des ESP ist sie leer.</p>
    <ul id="iULNew">
    </ul>
    <h2>authorisierte RFIDs</h2>
    <p>Das zweite Feld beinhaltet die Liste aller vorhandenen RFID-Tags. Wenn Sie klicken, so wird das Tag entfernt und in die Liste der
    gelesenen eingefuegt. Dies wird sofort auf dem ESP gespeichert, auch im Dateisystem</p>
    <ul id="iULOk">
    </ul>
    </form>
  </body></html>
)rawliteral";
