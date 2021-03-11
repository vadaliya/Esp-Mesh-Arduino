#include <Arduino.h>
#include "painlessMesh.h"

#define MESH_PREFIX "Esp-Mesh"
#define MESH_PASSWORD "messwithmesh"
#define MESH_PORT 5555

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

// Received message store in this variable
String message = {};

#define ButtonPin 18 // D18..Pin connected to button
#define LedPin 2     // D2...Pin Connected to LED

//Variable to check the state
int ledState = -1;        //this variable tracks the state of the button, low if not pressed, high if pressed
bool buttonState = LOW;    //this variable tracks the state of the LED, negative if off, positive if on
long lastDebounceTime = 0; // the last time the output pin was toggled
long debounceDelay = 500;  // the debounce time; increase if the output flickers

// User stub
void sendMessage(); // Prototype so PlatformIO doesn't complain

Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

void sendMessage()
{
    String msg = "Hi from Esp32-DOIT";
    msg += mesh.getNodeId();
    mesh.sendBroadcast(msg);
    taskSendMessage.setInterval(TASK_SECOND * 5);
}

// Needed for painless library
void receivedCallback(uint32_t from, String &msg)
{
    message = msg;
    Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId)
{
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback()
{
    Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset)
{
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void ledfunction()
{
    if (message == "on")
    {
        Serial.printf("Led is on\n");
        ledState = -ledState;
        digitalWrite(LedPin, HIGH);
        message = "";
    }

    else if (message == "off")
    {
        ledState = -ledState;
        Serial.printf("Led is off\n");
        digitalWrite(LedPin, LOW);
        message = "";
    }
}

void ButtonFunction();

Task taskSendStatus(TASK_SECOND * 1, TASK_FOREVER, &ButtonFunction);

void ButtonFunction()
{

    buttonState = digitalRead(ButtonPin);

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        if ((buttonState == HIGH) && (ledState < 0))
        {
            digitalWrite(LedPin, HIGH);  //turn LED on
            ledState = -ledState;        //now the LED is on, we need to change the state
            lastDebounceTime = millis(); //set the current time

            Serial.printf("Led is on");

            String msg = "on";
            msg += mesh.getNodeId();
            mesh.sendBroadcast(msg);
        }
        else if ((buttonState == HIGH) && (ledState > 0))
        {

            digitalWrite(LedPin, LOW);   //turn LED off
            ledState = -ledState;        //now the LED is off, we need to change the state
            lastDebounceTime = millis(); //set the current time

            Serial.printf("Led is off");

            String msg = "off";
            msg += mesh.getNodeId();
            mesh.sendBroadcast(msg);
        }
    }

    taskSendStatus.setInterval(TASK_SECOND * 5);
}

void setup()
{

    //Serial Monitor start with 115200 baudrate.
    Serial.begin(115200);

    pinMode(LedPin, OUTPUT);
    pinMode(ButtonPin, INPUT);

    //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
    mesh.setDebugMsgTypes(ERROR | STARTUP); // set before init() so that you can see startup messages

    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

    userScheduler.addTask(taskSendMessage);
    taskSendMessage.enable();

    userScheduler.addTask(taskSendStatus);
    taskSendStatus.enable();
}

void loop()
{

    ledfunction();
    ButtonFunction();
    // it will run the user scheduler as well
    mesh.update();
}