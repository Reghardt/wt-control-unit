#include "ecp.h"

String receiveMessage(char ch)
{
  static int hashes = 0;
  static int dollars = 0;
  static String message = "";

  if(!(ch == 10 || ch == 13))
  {
    if(ch == '#')
    {
      hashes++;
      if(hashes > 3)
      {
        Serial.println("ERR 1"); //more than 3 hashes encountered
        hashes = 0;
        dollars = 0;
        message = "";
      }   
    }
    else 
    {
      if(hashes == 3)
      {
        if(ch == '$')
        {
          dollars++;
          if(dollars == 3)
          {
            const String tempMessage = message;
            hashes = 0;
            dollars = 0;
            message = "";
            return tempMessage;
          }
        }
        else
        {
          if((ch < 59 && ch > 47) || (ch < 91 && ch > 64) || ch == 46 || ch == 33)
          {
            if(message.length() < 50)
            {
              message += ch;
            }
            else
            {
              message = "";
              return "";
            }
          }
          else
          {
            const int charCode = ch;
            Serial.println("ERR un ex body ch: " + String(charCode)); //unexpected character: 
            hashes = 0;
            dollars = 0;
            message = "";
          }
        }
      }
      else
      {
        const int charCode = ch;
        Serial.println("ERR un ex start ch: " + String(charCode)); //unexpected start char: 
        hashes = 0;
        dollars = 0;
        message = "";
      }
    }
  }

  return "";
}


String getValueOfKey(String key, String message, String expectType)
{
  unsigned int index = message.indexOf(key + ':');
  String value = "";

  if(index >= 0)
  {
    index += 1 + key.length(); // add 1 to remove ':'
    while (message[index] != '!' && index < message.length())
    {
      auto ch = message[index];
      if(expectType == "num")
      {
        if(ch > 47 && ch < 58)
        {
          value += ch;
          index++;
        }
        else
        {
          return "";
        }
      }
      else
      {
        if(ch > 64 && ch < 91)
        {
          value += ch;
          index++;
        }
        else
        {
          return "";
        }
      }

    }

    if(index >= message.length())
    {
      return "";
    }
    else
    {
      return value;
    }
  }
  else
  {
    return "";
  }
}
