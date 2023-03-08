//------------------------------------------------------------------------------------------
//die haupseite, sie wird im flash abgelegt (PROGMEM) R" heißt "roh", rawliteral( als trennzeichen
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
      function initWebSocket()
      { 
         websocket = new WebSocket(gateway);
         websocket.onopen = () =>
         {  
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
         }
      }
      
      window.addEventListener("load", () => 
      {
        initWebSocket();
        document.getElementById('bTestEintrag').addEventListener("click",() => 
        {
          let rfid = document.getElementById('testEintrag').value; 
          let owner = document.getElementById('testOwner').value; 
          websocket.send(JSON.stringify(
            {
              'action':'insert',
              'rfid':rfid,
              'owner':owner
            }));
        });
      }); 
    </script>
  </head> 
  <body style='font-family:Helvetica, sans-serif'> 
    <h1>Webserver for RFID config</h1>
    <p>Eintrag zum Test:</p>
    <form> 
      <input id='testEintrag' placeholder='RFIDid' type='text'>
      <input id='testOwner' placeholder='Owner' type='text'>
      <button type='button' id='bTestEintrag'>go</button>
    </form>
    <form method='post' action='/' name='rfid'>
    <p>Um neue RFIDs zu lesen bitte vor dem Sensor mit dem Teil wedeln, bevor weitere Aktionen erfolgen, bietet es sich an, nach und nach alle RFID-Tags einzulesen.<br>
    Mehr als 8 RFIDs pro Liste sind nicht möglich, kommt einer mehr, so wird der erste gekickt.</p>
    
    
    <h2>gelesene RFIDs</h2>
    <p>Beachte: Wenn man hier einen Haken setzen, dann wird das RFID-Tag in die Liste der authorisierten aufgenommen, dies muss dann noch gespeichert werden. 
    Das erste Feld ist die gelesene RFID, das zweite ist der einzutragende Besitzer, ohne diesen geht es nicht. </p>
    %NEW_RFID_LINES%
    <h2>authorisierte RFIDs</h2>
    <p>Das zweite Feld beinhaltet die Liste aller vorhandenen RFID-Tags. Wenn Sie einen Haken setzen, so wird das Tag 
    <strong>beim Speichern aus der Liste der authorisierten Tags entfernt.</strong> </p>
    %AUTH_RFID_LINES%
    <button type='submit'>Speichere auf Server</button>
    </form>
  </body></html>
)rawliteral";
