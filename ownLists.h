#include <LittleFS.h> //gehoert seit 2.0 zum core, den habe ich, also sollte es kein Thema sein

//die listen waren quatsch - ich hätte besser eine Liste von Objekten verwendet - aber jetzt wird die 
//Zeit langsam eng, daher lasse ich erst mal so
class RfidList; //forward
class StringList {
  private: 
    friend class RfidList;
    unsigned int max;
    String *strings; //nie gedanken darueber gemacht String scheint eine Klasse fuer Arduino zu sein
    unsigned int pos=0; //hinter letztem Eintrag
    char * fileName = NULL; //wird der char * als Literal übergeben, ist er im Flash und im RAM - wie lange ist er da?
                           //ah ja, string literale sind automatisch statisch - daher vermutlich auch das speichern im Ram und im Flash
                           //dennoch zur sicherheit kopie
  public:
    StringList(unsigned int max, const char * fileName = "")
    {
      this->max = max;
      unsigned len = strlen(fileName);
      if (len)
      {
        this->fileName = new char[len+1];
        strcpy(this->fileName,fileName);
      }    
      strings = new String[max];
    }
    ~StringList()
    {
      delete[] strings;
      delete [] fileName; //delete on Null should be safe
    }

    unsigned int getDelimiterPos() //eins hinter dem letzten Eintrags
    {
      return pos;
    }

    
    void clear()
    {
      for (int i = 0 ; i < pos ; ++i)
        strings[i] = "";
      pos = 0;
      return; 
    }
    void add(String s) //am ende einfuegen, ggf. platz frei machen 
    {
      if (pos == max) //voll
      {
        for (unsigned int i = 0; i < max-1; ++i)
          strings[i] = strings[i+1];
        pos--;  
      }
      strings[pos++] = s;
    }

    String * getList(unsigned int & anzahl)
    {
      anzahl = pos;
      return strings;  
    }
    void serialPrint()
    {
      Serial.println("--------------");
      for (unsigned int i = 0; i < pos ; ++i)
        Serial.println(strings[i]);
      Serial.println("--------------");
    }
    String htmlLines()
    {
      String result = "";
      for (unsigned int i = 0 ; i < pos ; ++i)
        result += strings[i] + "<br>\n";  
      return result; 
    }
    
    int getIndexOf(String s)
    {
      int index = -1;
      for (int i = 0 ; i < pos ; ++i)
      {
        if (strings[i] == s)
        {
            index = i;
            break;
        }
      }
      return index;
    }
    void deleteAt(int index)
    {
      if (index > -1 && index < max)
      {
        --pos;
        for (int i = index; i < pos;  ++i)
          strings[i] = strings[i+1];
        strings[pos] = "";
      }
    }
    //gibt den Index des gelöschten zurück
    int deleteEntry(String s)
    {
      int index = getIndexOf(s);
      deleteAt(index);
      return index;
    }

    //toCheck: muss erst gemounted werden? - denke nicht
    bool loadFromFile()
    {
      bool ret = true;
      if (fileName !="")
      {
         File dataFile = LittleFS.open(fileName, "r");
         int i=0; 
         if (dataFile)
         {
            while (dataFile.available() && i < max)
            {
               strings[i++] = dataFile.readStringUntil('\n'); 
            }
            pos = i;
            dataFile.close();
         }
         else
          ret = false;
      }
      return ret;
    }
    //Die Strings einfach der Reihe nach heraus 
    bool saveToFile()
    {
      bool ret = true;
      //Serial.println(String("schreibe in ") +fileName);
      if (fileName !="")
      {
         File dataFile = LittleFS.open(fileName, "w");
         if (dataFile)
         {
          for (int i = 0; i < pos ; ++i)
            dataFile.println(strings[i]); //scheint er so zu nehmen
          dataFile.close();
         }
         else 
          ret = false;
      }
      return ret;      
    }
};

class RfidList 
{
  private:
    unsigned int max;
    StringList *rfidL;
  public: 
    RfidList(int max, const char *fileName = "")
    {
      
      this->max = max;
      rfidL = new StringList(max,fileName); 
    }
    ~RfidList()
    {
      delete rfidL;
    }
    void clear()
    {
      rfidL->clear();
    }
    int indexOfRfid(String rfid)
    {
      int index = -1;
      int pos = rfidL->getDelimiterPos();
      for (int i = 0 ; i < pos ; ++i)
      {
        int pBar = (rfidL->strings)[i].indexOf('|');
        String lrfid = (rfidL->strings)[i].substring(0,pBar);
        if (lrfid == rfid)
        {
            index = i;
            break;
        }
      }
      return index;
    }  
    void getAt(int index, String &rfid, String &owner)
    {
        String &s = rfidL->strings[index];
        int p = s.indexOf('|');
        rfid = s.substring(0,p);
        owner = s.substring(p+1);
    }
    //nur hinzufuegen, wenn noch nicht da und liste nicht voll
    bool add(String rfid, String owner)
    {
      bool retVal = false;
      int pos = rfidL->getDelimiterPos();
      if (indexOfRfid(rfid) == -1  && pos < max)
      {
        rfidL->add(rfid+'|' + owner);
        retVal = true;    
      }
      return retVal;
    }
    
    void remove(String rfid)
    {
      int index = indexOfRfid(rfid);
      //Serial.println("in rfidlist Remove " + rfid + " at index " +i);
      rfidL->deleteAt(index);     
    }

    void removeAt(int index)
    {
      rfidL->deleteAt(index);      
    }

    int getIndexOf(String rfid,String owner)
    {
      return rfidL->getIndexOf(rfid+'|'+owner); 
    }
 
    unsigned int getDelimiterPos() //eins hinter dem letzten Eintrags
    {
      return rfidL->getDelimiterPos();
    }

 
    String htmlLines(String bText)//werde ich nicht mehr brauchen 
    {
      String result = "";
      int pos = rfidL->getDelimiterPos();      
      for (unsigned int i = 0 ; i < pos; ++i)
      {
        String &s = rfidL->strings[i];
        int p = s.indexOf('|');
        String rfid = s.substring(0,p);
        String owner = s.substring(p+1); 
        /*
        result += "<li><input type='checkbox' name='cb" + name+i+"'>" 
                  "<input type='text' name='rfid" + name+i+"' value='" +rfid+"' readonly>"
                  "<input type='text' name='owner" + name+i+"' value='" +owner+"'></li>\n";
        */
        result += "<li><button type='button'>" + bText + "</button><input type='text 'value='" +rfid+ "' readonly>"
                  "<input type='text' value='" +owner+ "'></li>\n";
        /* schade arrays gehen im Webserver nicht 
        result += "<input type='checkbox' name='cb"+name+"[]'>" 
                  "<input type='text' name='rfid"+name+"[]' value='" +rfidL->strings[i]+"' readonly>"
                  "<input type='text' name='owner"+name +"[]' value='" +ownerL->strings[i]+"'><br>\n";
        */
      }
      return result; 
    }

    void serialPrint()
    {
      rfidL->serialPrint();
    }

    bool loadFromFile()
    {
        bool rfid = rfidL->loadFromFile();
        return rfid;
    }

    bool saveToFile()
    {
        bool rfid = rfidL->saveToFile();
        return rfid;
    }
};
