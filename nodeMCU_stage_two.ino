//nodeMCU (ESP8266)
//wifi: http://henrysbench.capnfatz.com/henrys-bench/arduino-projects-tips-and-more/connect-nodemcu-esp-12e-to-wifi-router-using-arduino-ide/
//wifi:https://github.com/esp8266/Arduino/tree/master/doc/esp8266wifi

//connect to existing AP and then listen on port 80 for GPIO control
//experimentation on AP and server functionalities without using WebServer library functions

#include "FS.h"
#include "ESP8266WiFi.h"

//global variables
String ssid_name[7], ssid_passkey[7], ssID, password; //registered
String available_APs[20];   //real time
int n = 0;  //available wifi n/ws
File ssid;
WiFiServer server(80);  //creating server object of class WiFiServer listening on port 80
String paramNames[4], paramValues[4];

void format()
{
  SPIFFS.begin();
  Serial.println("please wait 30s for SPIFFS to be formatted"); //printed garbage
  SPIFFS.format();
  Serial.println("SPIFFS formatted"); //printed correctly
}

void write_ssid_file()
{
  ssid = SPIFFS.open("/ssid.txt", "w+");
  if(!ssid) Serial.println("file open failed");
  Serial.println("entering 4 ssid credentials into the file..."); //write data into file
  ssid.println("ambience:135");
  ssid.println("IoT:i@m");
  ssid.println("Real:diamonds");
  ssid.println("Quanta:imshaunak");
  ssid.close();
}

void read_ssid_file()  //read creds for registered APs and list them out
{
  int i,j,l=0, colon_index=0;           //loop-counters, length variable, string-delimiter
  ssid = SPIFFS.open("/ssid.txt", "r"); //open file for reading
  if(!ssid) Serial.println("file open failed");
  Serial.println("===========reading from file=======================");
  for(i=0; i<4; i++)
  {
    String s = ssid.readStringUntil('\n');
    Serial.println(s);
    for(j=0; j<s.length(); j++)
    {
      if(s[j] == ':')
      colon_index = j+1;
    }
    ssid_name[i] = s.substring(0,colon_index-1);
    ssid_passkey[i] = s.substring(colon_index);   
  }
  for(j=0; j<4; j++){Serial.print(ssid_name[j]);Serial.print(' ');} Serial.println();
  for(j=0; j<4; j++){Serial.print(ssid_passkey[j]);Serial.print(' ');} Serial.println();
  Serial.println();
}

void server_setup()
{
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("IoT", "12345678"); //AP name and default password set
  server.begin(); //start the server
  IPAddress HTTPS_ServerIP= WiFi.softAPIP(); // Obtain the IP of the Server 
  Serial.print("Server IP is: ");  Serial.println(HTTPS_ServerIP);
}

void setup()
{
  Serial.begin(115200);
  format();             //format any pre-existing files in flash memory
  write_ssid_file();
  read_ssid_file();

  WiFi.persistent(false); //trying to reduce conn delay. not helping
  WiFi.mode(WIFI_AP_STA); //set esp8266 to stationary mode and also as AP for server
  WiFi.disconnect();      //disconnect from any AP if already connected
  delay(100);             //min delay before preceeding
  
  server_setup(); //setup the nodeMCU as a server as well
 
  Serial.println("wifi and server setup done"); Serial.println();
}

void wifi_ap_connect()
{ 
    WiFi.begin(ssID.c_str(), password.c_str());  //convert to str format
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);//trying to reduce conn delay. not helping
    while (WiFi.status() != WL_CONNECTED)        //wait till conn is established
    {
      delay(500);
      Serial.print(".");
    }
    int wifiStatus = WiFi.status();
    if(wifiStatus == WL_CONNECTED)
      {
        Serial.print("Successful connection to IP: "); Serial.println(WiFi.localIP());
        Serial.print("RSSI: "); Serial.println(WiFi.RSSI());
      }
    else
    {
      Serial.println("Unsuccessful connection");  
      delay(1000);
    }
}

void server_actions()
{
  //Serial.println("entered server_actions()"); //printed continuously
  WiFiClient client = server.available();
  if (!client) { return; }
  Serial.println("connection made");

  String request = client.readStringUntil('\n'); //listen to the string sent by the browser and parse
  Serial.print("read string: "); Serial.println(request);
  int l = request.length();

  int sp1, sp2;
  sp1 = request.indexOf(" ", 0);
  sp2 = request.indexOf(" ", sp1+1);
  String reqd = request.substring(sp1+1, sp2);
  Serial.print("reqd: "); Serial.println(reqd);
  l = reqd.length();
  
  if((reqd.indexOf("/operate?", 0) < 0))  Serial.println("invalid URL \n");
  else
  {
    int m;
    int amp1=0, amp2=0;
    String user = "username", pass = "password", gpio = "gpio", state = "state";
    int id_user = reqd.indexOf(user, 0),
        id_pass = reqd.indexOf(pass, 0),
        id_gpio = reqd.indexOf(gpio, 0),
        id_state = reqd.indexOf(state, 0);
    Serial.println(id_user);  Serial.println(id_pass);  Serial.println(id_gpio);
    Serial.println(id_state);
    Serial.println(user.length());  Serial.println(pass.length());  
    Serial.println(gpio.length());  Serial.println(state.length());
    
    paramNames[0] = reqd.substring(id_user, id_user+user.length());
    paramNames[1] = reqd.substring(id_pass, id_pass+pass.length());
    paramNames[2] = reqd.substring(id_gpio, id_gpio+gpio.length());
    paramNames[3] = reqd.substring(id_state, id_state+state.length());

    amp1 = reqd.indexOf("&", 0);
    amp2 = reqd.indexOf("&", amp1+1);

    Serial.print("amp1: "); Serial.println(amp1); 
    Serial.print("amp2: "); Serial.println(amp2); 

    paramValues[0] = reqd.substring(id_user+9, amp1);
    paramValues[1] = reqd.substring(id_pass+9, amp2);
    paramValues[2] = reqd.substring(id_gpio+5, id_gpio+6);
    paramValues[3] = reqd.substring(id_state+6, id_state+7);
    
    for(m=0; m<4; m++)  
    {
      Serial.print(paramNames[m]);  Serial.print('\t');
      Serial.println(paramValues[m]);
    }
    Serial.println();
  }
}

void wifi_ap_scan() //scan for existing APs. works correctly
{
  int conn = 0;
  Serial.println("scan start");
  n = WiFi.scanNetworks();  //returns no. of n/w found
  if(n == 0) Serial.println("no networks found");
  else
  {
    Serial.print(n); Serial.println(" networks found");
    for(int k=0; k<n; k++)
    {
      if(!conn)
      {
        available_APs[k] = WiFi.SSID(k);  //store existing APs in array 
        Serial.println(available_APs[k]); //print the array
        delay(10);
        for(int l=0; l<7; l++)
        {
          if(ssid_name[l] == available_APs[k])
          {
            ssID = ssid_name[l];
            password = ssid_passkey[l];
            wifi_ap_connect();
            server_actions();   //listen on port 80 after successful conn
            conn = 1;
          }
        }
      }      
    }
  }
  Serial.println();
  delay(6000);  //min delay before next scan
}

void loop() 
{
  server_actions(); 
  wifi_ap_scan();   //scan for connection to registered APs
}
