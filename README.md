# test_callback_dbactive_send_realtime
25/05/2020  For test init senddata realtime if DB server Ready
### ทดสอบ เมื่อ db server run (active) จะส่ง 1 กลับไปที่  mconnect board จะส่งข้อมูล realtime  
### ถ้า db server (down) ค่าจะเป็น 0  จะไม่ส่งข้อมูล


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
    
    

task  

void taskChkDB( void * pvParameters ){
   while(1){
      ChkDB();
      dbready = false; 
      client.loop();//mqtt loop 
      vTaskDelay(500 / portTICK_PERIOD_MS);  
   }
}


 TaskHandle_t CheckDBserver;
    xTaskCreatePinnedToCore(
             taskChkDB, 
             "CheckDBserver",   
             5000,     
             NULL,      
             1,        
             &CheckDBserver,    
             0);     
             
             
 
