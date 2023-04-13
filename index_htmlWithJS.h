//------------------------------------------------------------------------------------------
//die haupseite, sie wird im flash abgelegt (wegen PROGMEM) R" heißt "roh", rawliteral( als trennzeichen
const char index_html[] PROGMEM = R"rawliteral(<!doctype html> 
<!-- seite wird vom esp generiert, zum testen mal als html gespeichert, geht mit chrome beachte aber Aederungen wg Prozent, s.u. -->          
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
      li input  {
        width: 15em;
      }
      input:nth-of-type(2){
        width: 25em;
      }
      
      #divNachrichten {
        border: 1px solid blue;
        padding: 1em;
        margin-top:  1em;
        margin-left: 1em;
      }
      ul {
        border: 1px solid blue;
        padding: 1em;
        list-style-type: none;
      }
      h1 {
        font-size: 135%%; /* im Template keine Prozentangababen moeglich :-), leistet template ein..., %% - yes */
        padding: 0em 1em;
      }
      h2 {
        font-size: 110%%;
      }
      .hinweis {
        border: 2px solid darkgreen;
        padding: 2em;
      }
      #controlContainer {
          display: grid;
          position: sticky;
          top: 0em;
          background-color: #fafaff;
          /*  grid-template-rows: 20%% 20%% 20%% 20%% 20%% ; sowieso automatisch */
           grid-template-columns: 30%% 1fr minmax(min-content,8em) 10%%;
           grid-auto-flow: column;
           padding-bottom: 1em; 
           /*justify-items: center; */             
      }
      #controlContainer h1 {
        display: inline-block;
        grid-row-start: 1; /* beachte rasterlinien, nicht zeilen, oben unten ist linie */ 
        grid-row-end  : 7; 
      }
      #controlContainer h2 {
          font-size: 120%%;
      }
      #controlContainer button {
        grid-column-start: 3;
        margin-top: 0.5em;
      }
      #controlContainer label {
        grid-column-start: 3;
        margin-top: 0.5em;
      }
      #status {
        padding: 1em; 
        grid-row: 1 / 7 ; /* kurzform */ 
        grid-column-start:4; 
      }
      #sSymbol {
        font-size:200%%;
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
            let extraData = inputs[1].value; 
            let owner = inputs[2].value;
            let sendObject = {"action":"addAuthorized","rfid":rfid,"extraData":extraData,"owner":owner};
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
        let extraData = inputs[1].value; 
        let owner = inputs[2].value;
        let sendObject = {"action":"removeAuthorized","rfid":rfid,"extraData":extraData,"owner":owner};
        websocket.send(JSON.stringify(sendObject));
     }


      function initWebSocket()
      { 
         websocket = new WebSocket(gateway);
         websocket.onopen = () =>
         {  //hmm, die folgenden get Geschchen müssten doch unnoetig sein, der Server sendet das am Anfang 
            
            websocket.send(JSON.stringify({'action':'getStartmeldungen'}));
            websocket.send(JSON.stringify({'action':'getRfidsNew'}));
            websocket.send(JSON.stringify({'action':'getRfidsOk'}));
            websocket.send(JSON.stringify({'action':'getStatus'}));
            
            //----------------------------
            
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
            //console.log(data);
            let newList = document.getElementById("iULNew");
            let okList = document.getElementById("iULOk");
            let newNode;
            let remNode;
            switch (data.action)
            {
              case "message":
                document.getElementById("iMessage").innerHTML += data.text + "<br>";
                break;
              case "addNew":
                if (data.removeFirst)
                {
                  newList.firstElementChild.remove();  
                }
                //let pos = newList.children.length;
                newNode = htmlToElement("<li><button type='button'>add</button> " + 
                      "<input type='text' value='" +data.rfid+"' readonly>"+
                      "<input type='text' value='" +data.extraData +"' readonly>" +
                      "<input type='text' maxlength='32' value='" +data.owner+"'></li>");
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
                      "<input type='text' value='" +data.extraData+"' readonly>"+
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
                      "<input type='text' value='" +entry.extraData+"' readonly>"+
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
                      "<input type='text' value='" +entry.extraData+"' readonly>"+
                      "<input type='text' maxlength='32' value='" +entry.owner+"'></li>");
                  
                  newNode.children[0].addEventListener("click",addClicked);//der button                                 
                  newList.append(newNode);
                });
                break;
              case "setStatus":
                let status = data['schlossOpen'];
                let text = document.getElementById('sText');
                let symbol = document.getElementById('sSymbol');
                if (status == "1") 
                {
                  text.innerHTML = "geöffnet";
                  symbol.innerHTML = "&#128275;";
                }
                else
                {
                  text.innerHTML = "geschlossen";
                  symbol.innerHTML = "&#128274;";                  
                }
                break;
              case "mailEnabled":
                let iMail = document.getElementById('iMail');                
                iMail.checked = (data['mailEnabled'] == "1");
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
          let extraData = document.getElementById('testExtraData').value;
          let owner = document.getElementById('testOwner').value;
          
          //websocket.send(rfid+'|'+owner);
          websocket.send(JSON.stringify(
          {
              'action':'addNew',
              'rfid':rfid,
              'extraData':extraData,
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
        document.getElementById('bReboot').addEventListener("click",() =>
        {
           websocket.send(JSON.stringify({'action':'rebootESP'}));
        });
        document.getElementById('bReset').addEventListener("click",() =>
        {
           websocket.send(JSON.stringify({'action':'resetMFC'}));
        });
        document.getElementById('bStatus').addEventListener("click",() =>
        {
           websocket.send(JSON.stringify({'action':'aendereStatus'}));
        });
        document.getElementById('bSchalte').addEventListener("click",() =>
        {
           websocket.send(JSON.stringify({'action':'schalte'}));
        });
        
        var iMail = document.getElementById('iMail');
        iMail.addEventListener("change",() =>
        {
           websocket.send(JSON.stringify({'action':'mailEnabled','mailEnabled':iMail.checked}));
        });
        document.getElementById('bClear').addEventListener("click",() => 
        { 
          document.getElementById('iMessage').innerHTML = "";
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
    <div id='controlContainer'>
        <h1>Webserver RFID config </h1>
        <button type="button" id="bWiFiStop">Stop WiFi</button>
        <button type="button" id="bReboot">reboot Esp</button>
        <button type="button" id="bReset">reset MFC</button>
        <button type="button" id="bStatus">ändere Status</button>
        <button type="button" id="bSchalte">Schalte</button>      
        <label for="iMail">Mail senden <input type="checkbox" id="iMail"></label>      
        <div id = 'status'>
          <h2>Status</h2>
          <span id="sText">geschlossen</span>
          <span id="sSymbol">&#128274;</span> 
        </div>
    </div>
    <form> <!-- method='post' action='/' name='rfid'> wollte mal mit post uebertragen, lasse das -->
    <h2>gelesene / neue  RFIDs <button id="bClearNew" type="button">Clear (ESP)</button> </h2>
    <ul id="iULNew">
    </ul>
    <h2>authorisierte RFIDs</h2>
    <ul id="iULOk">
    </ul>
    </form>
    <form> 
     <span>Eintrag zum Test:</span>
      <input id='testEintrag' placeholder='RFIDid' type='text'>
      <input id='testExtraData' placeholder='extraData' maxlength='32' type='text'>
      <input id='testOwner' placeholder='Owner' type='text' maxlength='32'>
      <button type='button' id='bTestEintrag'>go</button>
    </form>
    <div id='divNachrichten'>
     Nachrichten <button id='bClear' type='button'>(clear)</button>: 
     <p id="iMessage">
     </p>
    </div>
    <h2>Erl&auml;uterungen</h2>
    <div>
    <p>
    Die RFIDs werden in zwei Listen verwaltet, den neuen und den akzeptierten / authorisierten RFIDs. 
    Erst ist die RFID aufgeführt, dann die Bezeichnung, z.B. Besitzerin.</p>
    Mehr als 8 RFIDs pro Liste sind nicht möglich, kommt eine mehr, so wird die erste gekickt, im Fall der akzeptierten Rfids
    wird keine Änderung vorgenommen.</p>
    <p>
    Die Liste der neuen wird durch einen Testeintrag und durch Wedeln mit einem Tag modifiziert, sie 
    ist nicht persistent. Ist eine neue ID hinzugefügt werden, so sollte eine Bezeichnung vergeben werden.
    Klickt man in der Liste der neuen auf Add, so wird die ID in die Liste der akzeptieren übernommen.</p>
    <p>Wird ein RFID im Testeintrag hinzugefügt, dass vorhanden ist, so wird das Relais geschaltet.
    <p>Die zweite Liste beinhaltet alle akzeptierten / authorisierten RFID-Tags. Wenn Sie klicken, so wird das Tag entfernt und in die Liste der
    gelesenen/neuen eingefuegt. Jede Änderung dieser Liste wird sofort auf dem ESP gespeichert, auch im Dateisystem.</p>
    
    <div class = "hinweis">Ein Update der Firmware / des Sketches kann &uuml;ber OTA erfolgen.
      <ol>
        <li>Bin-Datei erzeugen, in der IDE Sketch -> Kompilierte ... exportieren oder Strg Alt s </li>
        <li>Upload der Firmware über %UPDATE_LINK% (findet sich im Sketchordner) </li>
      </ol>
      Das sollte auch fuer das Dateisystem statt Firmware gehen, ist aber unnoetig, da ich mir damit die 
      gespeicherten RFIDs loesche. Da sonst nichts dort liegt - egal. (Gibt wohl auch Probleme mit der Arduino-Ide das Dateisystem als bin 
      zu erzeugen, wenn kein USB.
    </div>
  </body></html>
)rawliteral";
