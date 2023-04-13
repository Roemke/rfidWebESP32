#include <LittleFS.h> //gehoert seit 2.0 zum core, den habe ich, also sollte es kein Thema sein

//die listen waren quatsch - ich hätte besser eine Liste von Objekten verwendet - aber jetzt wird die 
//Zeit langsam eng, daher lasse ich erst mal so - nein, ändere es, schreibe eine eigene Liste, stl geht nicht?
//bin mir noch nicht im klaren, ob ich irgendwo speicher-Probleme erzeuge
class Rfid {
  public: 
    String id = "";
    String owner = ""; 
    String extraData = "";
    bool valid = false;

    Rfid() = default;
    //irgendwo bekomme ich sonderzeichen herein, wende trim an
    Rfid(const String &i, const String &o)
    {
      id=i;
      owner=o;
      id.trim();
      owner.trim();
      extraData = "";
      valid = true;
    }

    Rfid(const String &i, const String &o, const String &eD)
    {
      id=i;
      owner = o;
      extraData = eD;
      id.trim();
      owner.trim();
      extraData.trim();
      valid = true;
    }
    //Aus String-Repräsentation erstellen 
    Rfid(const String &str, const char sep)
    {
      id= str;
      owner="";
      extraData = "";
      valid = true;
      String s = str;
      int index = s.indexOf(sep); 
      if (index != -1) // No space found
      {
        id = s.substring(0, index);
        s = s.substring(index+1);
        index = s.indexOf(sep); //der zweite
        if (index != -1)
        {
          owner = s.substring(0,index);
          extraData = s.substring(index+1);
        }
        else 
          owner=s;
      }
      id.trim();
      owner.trim();
      extraData.trim();
      
      //Serial.println("Created id: " + id + ",owner: " + owner + " and extraData: " + extraData);
    }
    String getAsString(const char sep = '|') const
    {
      return id + String(sep) + owner + String(sep) + extraData;
    }
    
    void setExtraData(byte * eD)
    {
      extraData = "";
      for (byte i = 0; i < 16; i++) 
      {
        extraData += (eD[i] < 0x10) ? "0" : "";
        extraData += String(eD[i],HEX);
      }
      extraData.toUpperCase();
      extraData.trim();
    }

    void generateExtraData()
    {
      auto randchar = []() -> char
      {
        const char charset[] =
        "0123456789"
        "ABCDEF";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
      };
      extraData="";
      for (int i = 0 ; i < 32; ++i)
        extraData += randchar(); 
    }
    void extraDataToByteArray(byte * buffer)
    {
      int max = extraData.length() / 2;
      if (max > 16)
        max = 16; 
      const char * extra = extraData.c_str();
      byte b;
      Serial.println("In extraDataToByteArray:");
      for (int i = 0 ; i < max; ++i)
      {
        sscanf( extra + 2*i, "%2hhx", &b); //hh means a byte not an int
        *(buffer + i) = b;
      }  
    }


    void setOwner(byte * o)
    {
      owner = "";
      for (byte i = 0; i < 32; i++) 
      {
        owner += !iscntrl(o[i]) ? (char) o[i] : ' ';
      }
      owner.trim();
    }
    void ownerToByteArray(byte * buffer)
    {
      int max = owner.length();
      if (max > 32)
        max = 32;  
      for (int i = 0 ; i < max; ++i)
        buffer[i] = (byte) owner[i];
    }
    bool operator==(const Rfid & rhs) const
    {
      return (id == rhs.id) && (owner == rhs.owner) && (extraData == rhs.extraData);
    }
    
    operator String() const 
    {
      return getAsString();
    }
    
};
String operator+=(String &s, const Rfid & rhs)
{
  s += rhs.getAsString();
}
String operator+(const String &s, const Rfid & rhs)
{
  return s + rhs.getAsString();
}
String operator+(const Rfid &lhs, const String & s)
{
  return lhs.getAsString() + s;
}


//class RfidList; //forward
template <typename T>class ObjectList {
  private: 
    unsigned int max;
    T *objects; 
    unsigned int pos=0; //hinter letztem Eintrag
    char * fileName = NULL; //wird der char * als Literal übergeben, ist er im Flash und im RAM - wie lange ist er da?
                           //ah ja, string literale sind automatisch statisch - daher vermutlich auch das speichern im Ram und im Flash
                           //dennoch zur sicherheit kopie
  public:
    ObjectList(unsigned int max, const char * fileName = "")
    {
      this->max = max;
      unsigned len = strlen(fileName);
      if (len)
      {
        this->fileName = new char[len+1];
        strcpy(this->fileName,fileName);
      }    
      objects = new T[max];
    }
    ~ObjectList()
    {
      delete[] objects;
      delete [] fileName; //delete on Null should be safe
    }

    unsigned int getDelimiterPos() //eins hinter dem letzten Eintrags
    {
      return pos;
    }
    void clear()
    {
      /*
      for (int i = 0 ; i < pos ; ++i)
      {
        objects[i] = Null;
      }*/
        
      pos = 0;
      return; 
    }
    void add(const T &o) //am ende einfuegen, ggf. platz frei machen 
    {
      if (pos == max) //voll
      {
        for (unsigned int i = 0; i < max-1; ++i)
          objects[i] = objects[i+1];
        pos--;  
      }
      objects[pos++] = o;
    }
    //nur hinzufuegen, wenn die id noch nicht da und liste nicht voll
    bool addNew(const T & o)
    {
      bool retVal = false;
      if (indexOfOnlyId(o) == -1  && pos < max)
      {
        add(o);
        retVal = true;    
      }
      return retVal;
    }
    //hmm, was ist mit index out of bounds ? Exception - habe zu lang kein c++ mehr programmiert ...
    T & getAt(int index)
    {
      return objects[index]; 
    }

    T * getList(unsigned int & anzahl)
    {
      anzahl = pos;
      return objects;  
    }
    void serialPrint()
    {
      Serial.println("--------------");
      for (unsigned int i = 0; i < pos ; ++i)
        Serial.println(String(objects[i]));
      Serial.println("--------------");
    }
    String htmlLines()
    {
      String result = "";
      for (unsigned int i = 0 ; i < pos ; ++i)
        result += objects[i] + "<br>";  //+ ueberladen
      return result;
    }
    
    int indexOf(const T & o)
    {
      int index = -1;
      for (int i = 0 ; i < pos ; ++i)
      {
        if (objects[i] == o)
        {
            index = i;
            break;
        }
      }
      return index;
    }
    //damit ist die Funktion nicht mehr generisch sondern sehr speziell für das rfid-Objekt 
    int indexOfOnlyId(const T & o)
    {
      int index = -1;
      for (int i = 0 ; i < pos ; ++i)
      {
        if (objects[i].id == o.id)
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
          objects[i] = objects[i+1];
      }
    }
    //gibt den Index des geloeschten zurück
    int deleteEntry(const T &o)
    {
      int index = indexOf(o);
      //Serial.print("Found to delete at ");
      //Serial.println( index);
      deleteAt(index);
      return index;
    }

    //toCheck: muss erst gemounted werden? - denke nicht
    bool loadFromFile()
    {
      Serial.println("Load from filesystem");
      bool ret = true;
      if (fileName !="")
      {
         File dataFile = LittleFS.open(fileName, "r");
         int i=0; 
         if (dataFile)
         {
            while (dataFile.available() && i < max)
            {
               objects[i++] = T(dataFile.readStringUntil('\n'),'|');
               //Serial.println("Add object"); 
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
            dataFile.println(objects[i].getAsString('|')); //scheint er so zu nehmen
          dataFile.close();
         }
         else 
          ret = false;
      }
      return ret;      
    }
};
