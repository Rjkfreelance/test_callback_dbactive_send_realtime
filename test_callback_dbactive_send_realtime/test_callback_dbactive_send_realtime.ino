#include <WiFi.h>
#include <PubSubClient.h> 
#include "time.h"
#include <RTClib.h>
#include <string.h>
#include <stdio.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

/*--------------Config Variable---------------------*/
const char* wifi_pwd;
const char* wifi_ssid;
const char* mqtt_server;
int mqttPort;
const char* mqttUser;
const char* mqttPassword;
const char* clientId;
int otatimeout; // OTA Timeout limit 30 sec
const char* sendtopic; // Machine send data Topic
const char* gtopic; //OTA Group Topic 
const char* ctopic; //OTA Sub Companny Topic
const char* stopic; //OTA Self Machine Topic
const char* ackota; //OTA Acknowledge use for Machine confirm received OTA
const char* getconf; // //Topic of this machine subscribe (or listen) for receive command from web socket command getcf(get config)
const char* sendconf; // Topic for Machine send config back to(publish to) web server (use web socket)
const char* dbreply;//Topic for check db active  Server Reply OK if Insert data already  ADD BY RJK 
/*--------------Config Variable---------------------*/

String eachline;// String  Object receiving each line in file conf.txt

char* certssl; // SSL Certification for download firmware OTA Load from certi.txt
String  Certs = "";// String Object for concatination certification from file certi.txt


/* String Object array for keep config each line in file conf.txt in SD Card */
String Line[16];

/*----------TIME NTP Server-------------*/
const char* ntpServer = "time.uni.net.th";
const long  gmtOffset_sec = 7 * 3600;
const int   daylightOffset_sec = 0;

String datareceivedmqtt; // receive OK from DBserv
bool dbready = false;
int checksettime = 0;

WiFiClient FMXClient;
PubSubClient client(FMXClient);
RTC_DS3231 RTC;
#define LEDTIME 500 //Send Data time interval for Testsend
#define Qos  1 //Quality of Service
#define TALK_DB "DB/talk" //add by rjk
#define DB_READY "DB/run" //add by rjk

void wifi_setup()
{ 
  
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid,wifi_pwd); //assign wifi ssid , pass

  while (WiFi.status() != WL_CONNECTED) {//Loop is not connect until connected exit loop
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Connect ok");
}

void sdbegin()
{
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  } else {
    Serial.println("SD Card OK");
  }

}


void assignConfig(fs::FS &fs, const char* path) {
  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open directory");
    return;
  }
  
  Serial.print("Reading file: ");
  Serial.println(path);
    int n =0;
    while (file.available()) {
     eachline = file.readStringUntil('\n');    
     int posi = eachline.indexOf(':');
     String val = eachline.substring(posi+1);
      //Serial.println(val);
      Line[n] = val;
      Line[n].trim();
      n++;
    }
   wifi_ssid = (const char*)Line[0].c_str();
   
   wifi_pwd = (const char*)Line[1].c_str();
  
   mqtt_server = (const char*)Line[2].c_str();
 
   mqttPort = Line[3].toInt();
   
   //Serial.println(mqtt_server);//debug ok
   //Serial.println(mqttPort);//debug ok

   mqttUser = (const char*)Line[4].c_str();

   mqttPassword = (const char*)Line[5].c_str();
   
   clientId = (const char*)Line[6].c_str();
 
   otatimeout = Line[7].toInt();
   sendtopic = (const char*)Line[8].c_str();
   gtopic = (const char*)Line[9].c_str();
   ctopic = (const char*)Line[10].c_str();
   stopic = (const char*)Line[11].c_str();
   ackota = (const char*)Line[12].c_str();
   getconf = (const char*)Line[13].c_str();
   sendconf = (const char*)Line[14].c_str();
   dbreply =  (const char*)Line[15].c_str(); // add by rjk
}
String DateTimeNOW(){
   DateTime now = RTC.now();
   String DMY = String(now.day())+"/"+String(now.month())+"/"+String(now.year())+" "+String(now.hour())+":"+String(now.minute())+":"+String(now.second());
   return DMY;
}

void data_time()
{
  DateTime now = RTC.now();
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  //  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  //    Serial.printf("NTP TIME : %02d/%02d/%04d ",timeinfo.tm_mday,timeinfo.tm_mon + 1,timeinfo.tm_year + 1900);
  //    Serial.printf("%02d:%02d:%02d \r\n",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
  if (checksettime == 0 ) {
    RTC.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
    checksettime = 1;
  }
  else
  {
    if (timeinfo.tm_wday == 0 && timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 && timeinfo.tm_sec <= 60)
    {
      RTC.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
      Serial.println("Update Time Success");
    }
  }
  Serial.printf("%02d", now.day());
  Serial.print('/');
  Serial.printf("%02d", now.month());
  Serial.print('/');
  Serial.printf("%02d", now.year());
  Serial.print(' ');
  Serial.printf("%02d", now.hour());
  Serial.print(':');
  Serial.printf("%02d", now.minute());
  Serial.print(':');
  Serial.printf("%02d", now.second());
  Serial.println();

}

void Callback(char *topic, byte *payload, unsigned int length){
  Serial.println(topic);//Print topic received
  Serial.println((char*)payload);//print payload (or message) in topic received
  
   if (strncmp(DB_READY, topic, strlen(DB_READY)) == 0){
       /*-----check talk to database server reply---*/
       dbready = (bool)(char)payload[0];
       Serial.println("DB Active");
       
   }
   
   if (strncmp(dbreply, topic, strlen(dbreply)) == 0) {
     
     Serial.println((char*)payload);
     
    for (int r = 0; r < length; r++) {
       
        datareceivedmqtt += (char)payload[r];
      
    }

    Serial.println();
  }

}
void mqttconnect() {
  /* Loop until reconnected */
  while (!client.connected()) {
    /* connect now */
    if (client.connect(clientId, mqttUser, mqttPassword)) {
     
      Serial.println("Mqtt....connected");
      
      /* subscribe topic */
       
        client.subscribe(dbreply,Qos);
        client.subscribe(DB_READY,Qos);
        
      }else{
        Serial.print("Can't connect MQTT Cloud!!!");
        Serial.print(client.state());
        delay(1000);
      }
    }
   
}

void ChkDB(){
 
   if(!client.connected()){
        mqttconnect(); 
      }
       client.publish(TALK_DB,"$a");
         
      Serial.println("Run check DBserver Task");
      if(dbready){
        Serial.println(dbready);
        client.publish(sendtopic,"#M testsend if DB active");
        Serial.println("Send realtime ok");
      }else{
        dbready = false;
        Serial.println(dbready);
      }
    
    
}

void taskChkDB( void * pvParameters ){
   while(1){
      ChkDB();
      dbready = false; 
      client.loop();//mqtt loop 
      vTaskDelay(500 / portTICK_PERIOD_MS);  
   }
}


void setup() {
 
  Serial.begin(115200);
  sdbegin();
  assignConfig(SD,"/conf.txt");
  wifi_setup();
  client.setServer(mqtt_server,mqttPort); 
  client.setCallback(Callback);
  mqttconnect();
  
   TaskHandle_t CheckDBserver;
    xTaskCreatePinnedToCore(
             taskChkDB, 
             "CheckDBserver",   
             5000,     
             NULL,      
             1,        
             &CheckDBserver,    
             0);     
   
}

void loop() {
  data_time();
 
 
}
