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
            console.log(e.data);
            let data =JSON.parse(e.data);
            console.log(data);
            if (data.name == "new")
            {
              let newList = document.getElementById("iULNew");
              if (data.removeFirst)
              {
                newList.firstElementChild.remove();  
                //folgendes sehr unschön, muss nochmal ueber die Namensgebung nachdenken
                /*
                for (let i = 0; i < newList.children.length; ++i)
                {
                  newList.children[i].children[0].name='cb'+data.name+i+'[]';          
                  newList.children[i].children[1].name='rfid'+data.name+i+'[]';          
                  newList.children[i].children[2].name='owner'+data.name+i+'[]';          
                }*/ 
              }
              //let pos = newList.children.length;
              let newNode = htmlToElement("<li><button>add</button> " + 
                    "<input type='text' value='" +data.rfid+"' readonly>"+
                    "<input type='text' value='" +data.owner+"'></li>");
              newList.append(newNode);
              newNode.children[0].addEventListener("click",(e) => //der button
              { 
                let li = e.currentTarget.parentNode; //current: listener is attached, also button
                let liClone = li.cloneNode(true); //deep copy (ohne listener); 
                li.remove();
                let button = liClone.querySelector("button");
                button.remove();
                button = htmlToElement("<button type='button'>remove</button>"); //fehlt noch der eventlistener
                alert("missing eventhandler");
                liClone.prepend(button); 
                let okList = document.getElementById("iULOk");
                let anzahl = okList.querySelectorAll("li").length;
                
                if (anzahl < %RFID_MAX%) /* geht das mit Templates?, ja - nett */
                  okList.append(liClone);
              });
                                
              /*
              newList.innerHTML +=  "<li><input type='checkbox' name='cb"+data.name+pos+"[]'>" + 
                    "<input type='text' name='rfid"+data.name+pos+"[]' value='" +data.rfid+"' readonly>"+
                    "<input type='text' name='owner"+data.name+pos +"[]' value='" +data.owner+"'></li>\n";            
              */
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
          websocket.send(rfid+'|'+owner);
          /* 
          websocket.send(JSON.stringify(
            {
              'action':'insert',
              'rfid':rfid,
              'owner':owner
            })); //Gedanken gecancelt
          */
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
    <p>Beachte: Wenn man hier klickt, dann wird das RFID-Tag in die Liste der authorisierten aufgenommen, dies muss dann noch gespeichert werden. 
    Das erste Feld ist die gelesene RFID, das zweite ist der einzutragende Besitzer, ohne diesen geht es nicht. </p>
    <ul id="iULNew">
    %NEW_RFID_LINES%
    </ul>
    <h2>authorisierte RFIDs</h2>
    <p>Das zweite Feld beinhaltet die Liste aller vorhandenen RFID-Tags. Wenn Sie klicken, so wird das Tag entfernt, ist in der Liste der
    gelesenen noch platz kommt es dort hinzu. 
    <strong>Beim Speichern wird es aus der Liste der authorisierten Tags auf dem ESP entfernt.</strong> </p>
    <ul id="iULOk">
    %AUTH_RFID_LINES%
    </ul>
    <button type='submit'>Speichere auf Server</button>
    </form>
  </body></html>
)rawliteral";
