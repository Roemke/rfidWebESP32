class RfidList; //forward
class StringList {
  private: 
    friend class RfidList;
    unsigned int max;
    String *strings; //nie gedanken darueber gemacht String scheint eine Klasse fuer Arduino zu sein
    unsigned int pos=0; //hinter letztem Eintrag

  public:
    StringList(unsigned int max)
    {
      this->max = max;
      strings = new String[max];
    }
    ~StringList()
    {
      delete[] strings;
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
      if (index > -1 && index < max-1)
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

};

class RfidList 
{
  private:
    unsigned int max;
    StringList *rfidL; //alternativ template klasse
    StringList *ownerL;
  
  public: 
    RfidList(int max)
    {
      this->max = max;
      rfidL = new StringList(max);
      ownerL = new StringList(max);
    }
    ~RfidList()
    {
      delete rfidL;
      delete ownerL;
    }
  
    //nur hinzufuegen, wenn noch nicht da
    bool add(String rfid, String owner)
    {
      bool retVal = false;
      if (rfidL->getIndexOf(rfid) == -1)
      {
        rfidL->add(rfid);
        ownerL->add(owner);
        retVal = true;    
      }
      return retVal;
    }
    
    void deleteRfid(String rfid)
    {
      int index = rfidL->deleteEntry(rfid);
      ownerL->deleteAt(index);
    }

    int getIndexOf(String rfid)
    {
      return rfidL->getIndexOf(rfid); 
    }
 
    unsigned int getDelimiterPos() //eins hinter dem letzten Eintrags
    {
      return rfidL->getDelimiterPos();
    }

 
    String htmlLines(String name)
    {
      String result = "";
      int pos = rfidL->getDelimiterPos();
      for (unsigned int i = 0 ; i < pos; ++i)
      {
        result += "<input type='checkbox' name='cb" + name+i+"'>" 
                  "<input type='text' name='rfid" + name+i+"' value='" +rfidL->strings[i]+"' readonly>"
                  "<input type='text' name='owner" + name+i+"' value='" +ownerL->strings[i]+"'><br>\n";
        
        /* schade arrays gehen im Webserver nicht 
        result += "<input type='checkbox' name='cb"+name+"[]'>" 
                  "<input type='text' name='rfid"+name+"[]' value='" +rfidL->strings[i]+"' readonly>"
                  "<input type='text' name='owner"+name +"[]' value='" +ownerL->strings[i]+"'><br>\n";
        */
      }
      return result; 
    }        
};
