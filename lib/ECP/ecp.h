//Easy Comunications protocol

#pragma once
#ifndef ECP_H
#define ECP_H

#include <Arduino.h>

String receiveMessage(char ch);
String getValueOfKey(String key, String message, String expectType);



#endif